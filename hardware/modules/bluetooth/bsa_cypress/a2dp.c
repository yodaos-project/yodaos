#include "a2dp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include <bsa_rokid/bsa_api.h>
#include "app_services.h"
#include "app_utils.h"
#include "app_disc.h"
#include "app_xml_utils.h"

#include "mypulse.h"

#ifndef APP_AV_FEAT_MASK
/* Default: Target (receive command) and Controller (send command) enabled and Copy Protection Enabled*/
#define APP_AV_FEAT_MASK (BSA_AV_FEAT_RCTG     | BSA_AV_FEAT_RCCT   | BSA_AV_FEAT_VENDOR | \
                          BSA_AV_FEAT_METADATA | BSA_AV_FEAT_BROWSE | BSA_AV_FEAT_PROTECT)
#endif

#ifndef APP_AV_USE_RC
#define APP_AV_USE_RC TRUE
#endif

#ifndef BSA_AV_DUMP_TX_DATA
#define BSA_AV_DUMP_TX_DATA  FALSE
#endif


/* Capabilities of the local apt-X codec */
static const tBSA_AV_CODEC_INFO aptx_caps =
{
    BSA_AV_CODEC_APTX, /* BSA codec ID, if 0, means that codec is not supported */
    /* Codec capabilities information
     * byte 0 : LOSC (length of codec info not including LOSC: 0x09)
     * byte 1 : media type (0x00 = audio)
     * byte 2 : media codec type (0xFF = non A2DP standard)
     * byte 3 to 6 : Vendor ID in little endian (0x0000004F = APT Licensing ltd.
     * byte 7 to 8 : Vendor specific codec ID (0x0001 = apt-X)
     * byte 9 :
     *   - bit 5 : 44.1kHz sampling frequency supported
     *   - bit 4 : 48kHz sampling frequency supported
     *   - bit 1 : stereo mode supported
     */
    "\x09\x00\xFF\x4F\x00\x00\x00\x01\x00\x32"
};

/* Capabilities of the local SEC codec */
static const tBSA_AV_CODEC_INFO sec_caps =
{
    BSA_AV_CODEC_SEC, /* BSA codec ID, if 0, means that codec is not supported */
    /* Codec capabilities information
     * byte 0 : LOSC (length of codec info not including LOSC: 0x09)
     * byte 1 : media type (0x00 = audio)
     * byte 2 : media codec type (0xFF = non A2DP standard)
     * byte 3 to 6 : Vendor ID in little endian (0x00000075 = Samsung ac ID)
     * byte 7 to 8 : Vendor specific codec ID (0x0001 = SEC)
     * byte 9 :
        *   - bit 6 : 32kHz sampling frequency supported
        *   - bit 5 : 44.1kHz sampling frequency supported
        *   - bit 4 : 48kHz sampling frequency supported
        *   - bit 3 : Mono
        *   - bit 1 : Stereo
     */
    "\x09\x00\xFF\x75\x00\x00\x00\x01\x00\x7A"
};


#define MAX_AV_CONN 2


typedef struct
{
    int index;
    char name[249]; /* Name of peer device. */
    /* Indicate if connection is registered */
    BOOLEAN is_registered;
    /* Handle of the connection */
    tBSA_AV_HNDL handle;
    /* Indicate that connection is open */
    BOOLEAN is_open;
    /* BD ADDRESS of the opened connection */
    BD_ADDR bd_addr;
    /* Indicate that this device has RC buttons */
    BOOLEAN is_rc;
    /* Handle of the remote connection of the device */
    tBSA_AV_RC_HNDL rc_handle;
    /* Latest Delay reported */
    //UINT16 delay;

    //UINT8  uipc_channel;
}tAPP_AV_CONNECTION;

struct A2dpCtx_t
{
    // genaration elements
    void *caller;
    unsigned char   uipc_channel;
    pthread_t       send_tid;
    pthread_mutex_t mutex;
    //pthread_cond_t  cond;
    int abort_request;
    int             thread_running;


    int is_opened;
    int is_playing;
    tAPP_AV_CONNECTION connections[MAX_AV_CONN];
    int conn_index;

    a2dp_callbacks_t listener;
    void *          listener_handle;
    // a2dp
    //BD_ADDR         bd_addr;
    int             is_registered;

    PulseAudio      *pa;

    int label;
    //int stop_pending;
};


extern int rokidbt_disc_read_remote_device_name(void *handle, BTAddr bd_addr, tBSA_TRANSPORT transport, 
                                                            tBSA_DISC_CBACK *user_dis_cback);
static A2dpCtx *_global_ac_ctx = NULL;

static tAPP_AV_CONNECTION *app_av_find_connection_by_handle(tBSA_AV_HNDL handle)
{
    int index;
    for (index = 0; index < MAX_AV_CONN; index++)
    {
        if (_global_ac_ctx->connections[index].handle == handle) {
            return &_global_ac_ctx->connections[index];
        }
    }
    return NULL;
}

static tAPP_AV_CONNECTION *app_av_find_connection_by_bd_addr(BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < MAX_AV_CONN; index++)
    {
        if (bdcmp(_global_ac_ctx->connections[index].bd_addr, bd_addr) == 0)
            return &_global_ac_ctx->connections[index];
    }
    return NULL;
}

static tAPP_AV_CONNECTION *app_av_find_connection_by_status(BOOLEAN registered, BOOLEAN open)
{
    int cnt;
    for (cnt = 0; cnt < MAX_AV_CONN; cnt++)
    {
        if ((_global_ac_ctx->connections[cnt].is_registered == registered) &&
            (_global_ac_ctx->connections[cnt].is_open == open))
            return &_global_ac_ctx->connections[cnt];
    }
    return NULL;
}

static int get_current_rc_handle(A2dpCtx *ac)
{
    int rc_handle = -1;
    if (ac->conn_index >= 0)
    {
        tAPP_AV_CONNECTION *conn = &ac->connections[ac->conn_index];
        if (conn->is_open)
            rc_handle = conn->rc_handle;
    }
    return rc_handle;
}

static int open_uipc_channel(A2dpCtx *ac, int channel)
{
    int status = -1;

    if (channel == UIPC_CH_ID_BAD) {
        BT_LOGE("### invalid parameter");
        return -1;
    }

    /* Open UIPC channel */
    if (UIPC_Open(channel, NULL) == FALSE)
    {
        BT_LOGE("UIPC_Open failed: %d", status);
        assert(0);
        return -1;
    }

    /* In this application we are not feeding the stream real time so we must be blocking */
    if (!UIPC_Ioctl(channel, UIPC_WRITE_BLOCK, NULL))
    {
        BT_LOGE("Failed setting the UIPC non blocking!");
        assert(0);
        return -1;
    }

    BT_LOGV("UIPC opened %d and set write block mode", channel);
    return 0;
}

A2dpCtx *a2dp_create(void *caller)
{
    A2dpCtx *ac = calloc(1, sizeof(*ac));

    ac->uipc_channel = UIPC_CH_ID_BAD;
    ac->caller = caller;
    pthread_mutex_init(&ac->mutex, NULL);
    _global_ac_ctx = ac;
    return ac;
}

int a2dp_destroy(A2dpCtx *ac)
{
    if (ac) {
        //a2dp_disable(ac);
        pthread_mutex_destroy(&ac->mutex);
        free(ac);
    }
    _global_ac_ctx = NULL;
    return 0;
}

int a2dp_register(A2dpCtx *ac)
{
    tBSA_AV_REGISTER register_param;
    int r;

    for (int i =0; i < MAX_AV_CONN; ++i) {
        BSA_AvRegisterInit(&register_param);
        r = BSA_AvRegister(&register_param);
        if (r != 0) {
            BT_LOGE("BSA_AvRegister failed");
            return r;
        }

        BT_LOGV("handle %d\n", register_param.handle);
        ac->connections[i].index = i;
        ac->connections[i].handle = register_param.handle;
        ac->connections[i].is_registered = 1;
    }

    if (ac->uipc_channel == UIPC_CH_ID_BAD) {
        /* Save the UIPC channel */
        open_uipc_channel(ac, register_param.uipc_channel);
        ac->uipc_channel = register_param.uipc_channel;
    }
    ac->is_registered = 1;
    return r;
}

int a2dp_deregister(A2dpCtx *ac)
{
    tBSA_STATUS status = 0;
    tBSA_AV_DEREGISTER deregister_param;

    for (int i = 0; i < MAX_AV_CONN; ++i)
    {
        tAPP_AV_CONNECTION *conn = ac->connections + i;
        if (conn->is_registered)
        {
            /* Deregister AV */
            BSA_AvDeregisterInit(&deregister_param);
            deregister_param.handle = conn->handle;
            status = BSA_AvDeregister(&deregister_param);
            if(status != BSA_SUCCESS) {
                BT_LOGE("BSA_AvDeregister failed status=%d", status);
            }
            conn->is_registered = 0;
            conn->handle = 0;
        }
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_av_build_notification_response
 **
 ** Description      Local function to send notification
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
static int app_av_build_notification_response(UINT8 event_id, tBSA_AV_META_RSP_CMD *p_bsa_av_meta_rsp_cmd)
{
    int status;
    //tBSA_AV_META_ATTRIB *p_pas_attrib;

    p_bsa_av_meta_rsp_cmd->param.notify_status.status = AVRC_STS_NO_ERROR;

    p_bsa_av_meta_rsp_cmd->param.notify_status.event_id = event_id;
    p_bsa_av_meta_rsp_cmd->pdu = BSA_AVRC_PDU_REGISTER_NOTIFICATION;

    BT_LOGV("### event_id %d", event_id);
#if 0
    switch(event_id)
    {
    case AVRC_EVT_PLAY_STATUS_CHANGE:   /* 0x01 */
        // TODO
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.play_status = 0;
        break;

    case AVRC_EVT_TRACK_CHANGE:         /* 0x02 */
        // TODO
        p = p_bsa_av_meta_rsp_cmd->param.notify_status.param.track;
        UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FOLDER_ID); /* the id from the no now playing folder */
        UINT32_TO_BE_STREAM(p, AVRCP_NO_NOW_PLAYING_FILE_ID);   /* the id from the no now playing file */
        break;

    case AVRC_EVT_TRACK_REACHED_END:    /* 0x03 */
    case AVRC_EVT_TRACK_REACHED_START:  /* 0x04 */
        break;

    case AVRC_EVT_PLAY_POS_CHANGED:     /* 0x05 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.play_pos = 0;
        break;
    case AVRC_EVT_BATTERY_STATUS_CHANGE:/* 0x06 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.battery_status = p_bsa_av_cb->meta_info.notif_info.bat_stat;
        break;

    case AVRC_EVT_SYSTEM_STATUS_CHANGE: /* 0x07 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.system_status = p_bsa_av_cb->meta_info.notif_info.sys_stat;
        break;

    case AVRC_EVT_APP_SETTING_CHANGE:   /* 0x08 */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.num_attr = 1;
        p_pas_attrib = &p_bsa_av_cb->meta_info.pas_info.equalizer;
        for (xx=0; xx<p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.num_attr; xx++)
        {
            p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.attr_id[xx] = p_pas_attrib->attrib_id;
            p_bsa_av_meta_rsp_cmd->param.notify_status.param.player_setting.attr_value[xx] = p_pas_attrib->curr_value;
            p_pas_attrib++;
        }
        break;

    case AVRC_EVT_NOW_PLAYING_CHANGE:   /* 0x09 no param */
    case AVRC_EVT_AVAL_PLAYERS_CHANGE:  /* 0x0a no param */
        break;

    case AVRC_EVT_ADDR_PLAYER_CHANGE:   /* 0x0b */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.addr_player.player_id = 0;
        /* UID counter is always 0 for Database Unaware Players */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.addr_player.uid_counter = 0;
        break;

    case AVRC_EVT_UIDS_CHANGE:          /* 0x0c */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.uid_counter = 0;
        break;

    case AVRC_EVT_VOLUME_CHANGE:        /* 0x0d */
        p_bsa_av_meta_rsp_cmd->param.notify_status.param.volume = 0;
        break;

    default:
        BT_LOGV("#### defualt");
        p_bsa_av_meta_rsp_cmd->param.notify_status.status = BTA_AV_RSP_NOT_IMPL;
    }

#endif


    status = BSA_AvMetaRsp(p_bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvMetaRsp failed: %d", status);
        //abort();
        return status;
    }

    BT_LOGD("app_av_build_notification_response %d", event_id);
    return 0;
}
int app_av_rc_register_notifications(int index, tBSA_AV_META_MSG_MSG *pMetaMsg)
{
    UINT16  evt_mask = 1, index_x;

    /* data associated with BSA_AvMetaRsp */
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;

    int status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvMetaRspInit failed: %d", status);
        return status;
    }


#if 0
    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(app_av_cb.connections)))
    {
        BT_LOGE("Connection index out of bounds");
        return -1;
    }
#endif

    if(pMetaMsg == NULL)
    {
        BT_LOGE("Null tBSA_AV_META_MSG_MSG pointer");
        return -1;
    }

    index_x = pMetaMsg->param.reg_notif.event_id - 1;
    evt_mask <<= index_x;

    bsa_av_meta_rsp_cmd.rc_handle = get_current_rc_handle(_global_ac_ctx);
    bsa_av_meta_rsp_cmd.label = _global_ac_ctx->label;

    bsa_av_meta_rsp_cmd.param.notify_status.opcode = pMetaMsg->opcode;
    bsa_av_meta_rsp_cmd.param.notify_status.code = BTA_AV_RSP_INTERIM;

#if 0
    /* Register event to the BSA AV control block  */
    p_bsa_av_cb->meta_info.registered_events.evt_mask |= evt_mask;
    p_bsa_av_cb->meta_info.registered_events.label[index_x] = pMetaMsg->label;
#endif

    return app_av_build_notification_response(pMetaMsg->param.reg_notif.event_id, &bsa_av_meta_rsp_cmd);
}

int app_av_rc_send_play_status_meta_response(int index)
{
    BT_LOGI("app_av_rc_send_play_status_meta_response");
    /* data associated with BSA_AvMetaRsp */
    int status;
    tBSA_AV_META_RSP_CMD bsa_av_meta_rsp_cmd;
    A2dpCtx *ac = _global_ac_ctx;

    /* Sanity check */
    if ((index < 0) || (index >= APP_NUM_ELEMENTS(ac->connections)))
    {
        BT_LOGE("Connection index out of bounds");
        return -1;
    }

    /* Send remote control response */
    status = BSA_AvMetaRspInit(&bsa_av_meta_rsp_cmd);

    bsa_av_meta_rsp_cmd.rc_handle = get_current_rc_handle(ac);
    bsa_av_meta_rsp_cmd.pdu = BSA_AVRC_PDU_GET_PLAY_STATUS;
    bsa_av_meta_rsp_cmd.label = 4;
    bsa_av_meta_rsp_cmd.param.get_play_status.song_len = (UINT32) 255000;    /*  Sample lenth of song 4' 15 seconds */
    bsa_av_meta_rsp_cmd.param.get_play_status.song_pos = (UINT32) 120000;    /*  Sample current position 2' 00 seconds */


    bsa_av_meta_rsp_cmd.param.get_play_status.play_status = ac->is_playing;

    APP_INFO1("Play status = %d", bsa_av_meta_rsp_cmd.param.get_play_status.play_status);

    status = BSA_AvMetaRsp(&bsa_av_meta_rsp_cmd);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvMetaRsp failed: %d", status);
        return status;
    }
    return 0;
}

static void rkd_a2dp_discovery_callback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    A2dpCtx *ac = _global_ac_ctx;
    tAPP_AV_CONNECTION *connection = NULL;

    if (!ac || !p_data) return;
    //BT_LOGV("\n\tevent%d\n", event);
    switch (event)
    {
    case BSA_DISC_NEW_EVT:
        BT_LOGI("\tName:%s", p_data->disc_new.name);
        BT_LOGI("\tServices:0x%08x (%s)",
               (int) p_data->disc_new.services,
               app_service_mask_to_string(p_data->disc_new.services));

        if ((p_data->disc_new.services & BSA_A2DP_SERVICE_MASK) == 0) {
            BT_LOGE("current device is not a a2dp sink device");
            a2dp_close_device(ac, p_data->disc_new.bd_addr);
        }

        break;
    case BSA_DISC_CMPL_EVT:
        break;
    case BSA_DISC_REMOTE_NAME_EVT:
        if (p_data->dev_info.status == BSA_SUCCESS) {
            connection = app_av_find_connection_by_bd_addr(p_data->remote_name.bd_addr);
            if (connection)
                snprintf(connection->name, sizeof(connection->name), "%s", p_data->remote_name.remote_bd_name);
            app_read_xml_remote_devices();

            app_xml_update_name_db(app_xml_remote_devices_db,
                                   APP_NUM_ELEMENTS(app_xml_remote_devices_db), 
                                   p_data->remote_name.bd_addr, p_data->remote_name.remote_bd_name);

            /* Update database => write to disk */
            if (app_write_xml_remote_devices() < 0) {
                BT_LOGE("Failed to store remote devices database");
            }
        }
        break;
    default:
        break;
    }
}

static void a2dp_callback(tBSA_AV_EVT event, tBSA_AV_MSG *p_data)
{
    tAPP_AV_CONNECTION *connection = NULL;
    //int status, i;
    BT_A2DP_MSG msg = {0};

    A2dpCtx *ac = _global_ac_ctx;
    BT_LOGD("\n event %d", event);
    switch (event)
    {
    case BSA_AV_OPEN_EVT:
        BT_LOGI("###BSA_AV_OPEN_EVT status:%d handle:%d cp:%d aptx:%d sec:%d",
                p_data->open.status, p_data->open.handle,
                p_data->open.cp_supported,
                p_data->open.aptx_supported,
                p_data->open.sec_supported);

        BT_LOGI("### BSA_AV_OPEN_EVT address=%x:%x:%x:%x:%x:%x",
                p_data->open.bd_addr[0],
                p_data->open.bd_addr[1],
                p_data->open.bd_addr[2],
                p_data->open.bd_addr[3],
                p_data->open.bd_addr[4],
                p_data->open.bd_addr[5]);

        connection = app_av_find_connection_by_handle(p_data->open.handle);
        if (connection == NULL) {
            BT_LOGE("unknown connection handle %d", p_data->open.handle);
        }
        else {
            bdcpy(msg.open.dev.bd_addr, p_data->open.bd_addr);
            if (rokidbt_find_bonded_device(ac->caller, &msg.open.dev) == 0) {
                rokidbt_find_scaned_device(ac->caller, &msg.open.dev);
            }
            if (p_data->open.status == BSA_SUCCESS) {
                bdcpy(connection->bd_addr, p_data->open.bd_addr);
                connection->is_open = TRUE;
                ac->is_opened = 1;
                ac->conn_index = connection->index;
                snprintf(connection->name, sizeof(connection->name), "%s", msg.open.dev.name);
                a2dp_start(ac);
                rokidbt_disc_read_remote_device_name(ac->caller, p_data->open.bd_addr, BSA_TRANSPORT_BR_EDR,
                                    rkd_a2dp_discovery_callback);
            }
            else {
                connection->is_open = 0;
                if (app_av_find_connection_by_status(1, 1) != NULL) {
                    a2dp_start(ac);
                }
            }

            if (ac->listener)
            {
                msg.open.status = p_data->open.status;
                ac->listener(ac->listener_handle, BT_A2DP_OPEN_EVT, &msg);
            }
        }
        break;

    case BSA_AV_CLOSE_EVT:
        BT_LOGI("### BSA_AV_CLOSE_EVT status:%d handle:%d",
                p_data->close.status, p_data->close.handle);

        connection = app_av_find_connection_by_handle(p_data->close.handle);
        if (connection == NULL) {
            BT_LOGE("unknown connection handle %d", p_data->close.handle);
        }
        else {
            connection->is_open = 0;
            tAPP_AV_CONNECTION *cur_connection = app_av_find_connection_by_status(1, 1);
            if (cur_connection == NULL) {
                ac->conn_index = -1;
                ac->is_opened = 0;
                pthread_mutex_lock(&ac->mutex);
                pulse_pipe_stop(ac->pa);
                pthread_mutex_unlock(&ac->mutex);
            } else {
                ac->conn_index = cur_connection->index;
                if (ac->pa)
                    a2dp_start(ac);
            }

            if (ac->listener)
            {
                bdcpy(msg.close.dev.bd_addr, connection->bd_addr);
                snprintf(msg.close.dev.name, sizeof(msg.close.dev.name), "%s", connection->name);
                msg.close.status = p_data->close.status;
                ac->listener(ac->listener_handle, BT_A2DP_CLOSE_EVT, &msg);
            }
        }
        break;

    case BSA_AV_DELAY_RPT_EVT:
        BT_LOGD("### BSA_AV_DELAY_RPT_EVT channel:%d handle:%d delay: %d",
                  p_data->delay.channel, p_data->delay.handle, p_data->delay.delay);
        break;

    case BSA_AV_PENDING_EVT:

        BT_LOGI("### BSA_AV_PENDING_EVT address=%x:%x:%x:%x:%x:%x\n",p_data->pend.bd_addr[0],
            p_data->pend.bd_addr[1],p_data->pend.bd_addr[2],p_data->pend.bd_addr[3],
            p_data->pend.bd_addr[4],p_data->pend.bd_addr[5]);
        a2dp_open(ac, p_data->pend.bd_addr);
        break;

    case BSA_AV_START_EVT:
        BT_LOGI("BSA_AV_START_EVT status:%d channel:%x UIPC:%d SCMS:%d,%d",
                p_data->start.status, p_data->start.channel, p_data->start.uipc_channel,
                p_data->start.cp_enabled, p_data->start.cp_flag);
        /* If this is a Point to Point AV connection */
        ac->is_playing = BSA_AVRC_PLAYSTATE_PLAYING;
        if (ac->listener)
        {
            msg.start.status = p_data->start.status;
            ac->listener(ac->listener_handle, BT_A2DP_START_EVT, &msg);
        }
        if (p_data->start.channel == BSA_AV_CHNL_AUDIO)
        {
#if 0
            if (p_data->start.status == BSA_SUCCESS)
            {
                switch (app_av_cb.media_feeding.format)
                {
                case bsa_av_codec_pcm:
                    BT_LOGI("start pcm feeding: freq=%d / channels=%d / bits=%d",
                            app_av_cb.media_feeding.cfg.pcm.sampling_freq,
                            app_av_cb.media_feeding.cfg.pcm.num_channel,
                            app_av_cb.media_feeding.cfg.pcm.bit_per_sample);
                    break;
                case bsa_av_codec_aptx:
                    BT_LOGI("    start apt-x feeding: freq=%d / mode=%d",
                            app_av_cb.media_feeding.cfg.aptx.sampling_freq,
                            app_av_cb.media_feeding.cfg.aptx.ch_mode);
                    break;
                case bsa_av_codec_sec:
                    BT_LOGI("    start sec feeding: freq=%d / mode=%d",
                            app_av_cb.media_feeding.cfg.sec.sampling_freq,
                            app_av_cb.media_feeding.cfg.sec.ch_mode);
                    break;

                default:
                    BT_LOGI("unsupported feeding format code: %d", app_av_cb.media_feeding.format);
                    break;
                }

                BT_LOGI("    scms-t: cp_enabled = %s, cp_flag = %d",
                        p_data->start.cp_enabled ? "true" : "false",
                                p_data->start.cp_flag);

                if (app_dm_get_dual_stack_mode() == bsa_dm_dual_stack_mode_bsa)
                {
                    /* reconfigure the uipc to adapt to the feeding mode */
                    app_av_uipc_reconfig();

                    /* unlock the tx thread if it was locked */
                    status = app_unlock_mutex(&app_av_cb.app_stream_tx_mutex);
                    if (status < 0)
                    {
                        BT_LOGE("app_unlock_mutex failed:%d", status);
                    }
                }
                else
                {
                    BT_LOGI("stack is not in bsa mode. do not start av thread");
                }
            }
            else
            {
                /* in case we are playing a list, finish */
                app_av_cb.play_list = false;
            }
            app_av_cb.last_start_status = p_data->start.status;
#endif
        }
        /* Else, this is a Broadcast channel */
        else
        {
#ifdef APP_AV_BCST_INCLUDED
            //app_av_bcst_start_event_hdlr(&p_data->start);
#endif
        }
        break;

    case BSA_AV_STOP_EVT:
        BT_LOGI("BSA_AV_STOP_EVT pause:%d Channel:%d UIPC:%d",
                p_data->stop.pause, p_data->stop.channel, p_data->stop.uipc_channel);
        //ac->stop_pending = 0;
        if (p_data->stop.pause)
            ac->is_playing = BSA_AVRC_PLAYSTATE_PAUSED;
        else
            ac->is_playing = BSA_AVRC_PLAYSTATE_STOPPED;

        if (ac->listener)
        {
            msg.stop.status = p_data->stop.status;
            msg.stop.pause = p_data->stop.pause;
            ac->listener(ac->listener_handle, BT_A2DP_STOP_EVT, &msg);
        }
        break;

    case BSA_AV_RC_OPEN_EVT:
        BT_LOGI("BSA_AV_RC_OPEN_EVT status:%d handle:%d", p_data->rc_open.status, p_data->rc_open.rc_handle);

        connection = app_av_find_connection_by_bd_addr(p_data->rc_open.peer_addr);
        if (connection == NULL)
        {
            BT_LOGE("unknown connection bd addr for %02X:%02X:%02X:%02X:%02X:%02X",
                p_data->rc_open.peer_addr[0], p_data->rc_open.peer_addr[1], p_data->rc_open.peer_addr[2],
                p_data->rc_open.peer_addr[3], p_data->rc_open.peer_addr[4], p_data->rc_open.peer_addr[5]);
        }
        else if (p_data->rc_open.status == BSA_SUCCESS)
        {
            connection->is_rc = TRUE;
            connection->rc_handle = p_data->rc_open.rc_handle;
        }

        if (ac->listener)
        {
            msg.rc_open.status = p_data->rc_open.status;
            ac->listener(ac->listener_handle, BT_A2DP_RC_OPEN_EVT, &msg);
        }
        break;
    case BSA_AV_RC_CLOSE_EVT:
        BT_LOGI("BSA_AV_RC_CLOSE_EVT handle:%d bdaddr %02X:%02X:%02X:%02X:%02X:%02X", p_data->rc_close.rc_handle,
                p_data->rc_close.peer_addr[0], p_data->rc_close.peer_addr[1], p_data->rc_close.peer_addr[2],
                p_data->rc_close.peer_addr[3], p_data->rc_close.peer_addr[4], p_data->rc_close.peer_addr[5]);
        connection = app_av_find_connection_by_bd_addr(p_data->rc_close.peer_addr);
        if (connection == NULL)
        {
            BT_LOGE("unknown connection bd addr for %02X:%02X:%02X:%02X:%02X:%02X",
                p_data->rc_close.peer_addr[0], p_data->rc_close.peer_addr[1], p_data->rc_close.peer_addr[2],
                p_data->rc_close.peer_addr[3], p_data->rc_close.peer_addr[4], p_data->rc_close.peer_addr[5]);
        }
        else
        {
            connection->is_rc = FALSE;
        }
        if (ac->listener)
        {
            msg.rc_close.status = 0;
            ac->listener(ac->listener_handle, BT_A2DP_RC_CLOSE_EVT, &msg);
        }

        break;

    case BSA_AV_REMOTE_CMD_EVT:
        BT_LOGD("BSA_AV_REMOTE_CMD_EVT handle:%d", p_data->remote_cmd.rc_handle);

        switch(p_data->remote_cmd.rc_id)
        {
        case BSA_AV_RC_PLAY:
            BT_LOGD("Play key");
            break;
        case BSA_AV_RC_STOP:
            BT_LOGD("Stop key");
            break;
        case BSA_AV_RC_PAUSE:
            BT_LOGD("Pause key");
            break;
        case BSA_AV_RC_FORWARD:
            BT_LOGD("Forward key");
            break;
        case BSA_AV_RC_BACKWARD:
            BT_LOGD("Backward key");
            break;
        default:
            BT_LOGD("unknow key:0x%x", p_data->remote_cmd.rc_id);
            break;
        }
        if (ac->listener) {
            msg.remote_cmd.rc_id = p_data->remote_cmd.rc_id;
            ac->listener(ac->listener_handle, BT_A2DP_REMOTE_CMD_EVT, &msg);
        }
        break;

    case BSA_AV_REMOTE_RSP_EVT:
        BT_LOGD("BSA_AV_REMOTE_RSP_EVT handle:%d", p_data->remote_rsp.rc_handle);
        if (ac->listener) {
            msg.remote_rsp.rc_id = p_data->remote_rsp.rc_id;
            ac->listener(ac->listener_handle, BT_A2DP_REMOTE_RSP_EVT, &msg);
        }
        break;

    case BSA_AV_VENDOR_CMD_EVT:
        BT_LOGD("    len=%d, label=%d, code=%d, company_id=0x%x",
             p_data->vendor_cmd.len, p_data->vendor_cmd.label, p_data->vendor_cmd.code,
             p_data->vendor_cmd.company_id);
        break;

    case BSA_AV_VENDOR_RSP_EVT:
        BT_LOGD("BSA_AV_VENDOR_RSP_EVT handle:%d", p_data->vendor_rsp.rc_handle);
        BT_LOGD("    len=%d, label=%d, code=%d, company_id=0x%x",
             p_data->vendor_rsp.len, p_data->vendor_rsp.label, p_data->vendor_rsp.code,
             p_data->vendor_rsp.company_id);
        break;

    case BSA_AV_META_MSG_EVT:
        BT_LOGD("BSA_AV_META_MSG_EVT handle:%d", p_data->meta_msg.rc_handle);

        BT_LOGD("    label=%d, pdu=%d, company_id=0x%x",
            p_data->meta_msg.label, p_data->meta_msg.pdu, p_data->meta_msg.company_id);

        switch(p_data->meta_msg.pdu)
        {
        case BSA_AVRC_PDU_GET_ELEMENT_ATTR:
            break;

        case BSA_AVRC_PDU_GET_PLAY_STATUS:
            app_av_rc_send_play_status_meta_response(0);
            break;

        case BSA_AVRC_PDU_SET_ADDRESSED_PLAYER:
            break;

        case BSA_AVRC_PDU_GET_FOLDER_ITEMS:
            break;

        case BSA_AVRC_PDU_REGISTER_NOTIFICATION:
            //app_av_rc_register_notifications(0, &p_data->meta_msg);
            break;

        case BSA_AVRC_PDU_SET_BROWSED_PLAYER:
            break;

        case BSA_AVRC_PDU_CHANGE_PATH:
            break;

        case BSA_AVRC_PDU_GET_ITEM_ATTRIBUTES:
            break;

        case BSA_AVRC_PDU_PLAY_ITEM:
            break;
        }

        break;

    case BSA_AV_FEAT_EVT:
        BT_LOGD("BSA_AV_FEAT_EVT Peer feature:%x", p_data->rc_feat.peer_features);
        break;

    default:
        BT_LOGD("Unknown msg %d", event);
        break;
    }
}

int a2dp_enable(A2dpCtx *ac)
{
    int status;

    tBSA_AV_ENABLE enable_param;
    tBSA_AV_DISABLE disable_param;

    if (ac->is_registered)
        return 0;

    if (!ac->pa)
        ac->pa = pulse_create();

    BSA_AvEnableInit(&enable_param);
    enable_param.p_cback = a2dp_callback;
    /* Set apt-X capabilities */
    enable_param.aptx_caps = aptx_caps;
    /* Set SEC capabilities */
    enable_param.sec_caps = sec_caps;
    /* Set the Features mask needed */
    enable_param.features = APP_AV_FEAT_MASK;

    status = BSA_AvEnable(&enable_param);
    if (status != BSA_SUCCESS) {
        BT_LOGE("BSA_AvEnable failed %d", status);
        goto enable_fail;
    }

    status =  a2dp_register(ac);
    if (status != BSA_SUCCESS) {
        BT_LOGE("BSA_AvRegister failed %d", status);
        goto register_fail;
    }
    return 0;
register_fail:
    BSA_AvDisableInit(&disable_param);
    BSA_AvDisable(&disable_param);
enable_fail:
    pulse_destroy(ac->pa);
    ac->pa = NULL;
    return status;
}

int a2dp_open(A2dpCtx *ac, BD_ADDR bd_addr)
{
    int status;
    tBSA_AV_OPEN open_param;
    tAPP_AV_CONNECTION *conn = NULL;


    // find an not opened av handle
    conn = app_av_find_connection_by_status(TRUE, FALSE);
    if (!conn)
    {
        BT_LOGE("error: cant alloc connections anymore!");
        return -1;
    }

#if 0
    if (ac->is_opened) {
        BT_LOGI("##### pause current channel");
        if ( 0 == a2dp_stop(ac))
        {
            ac->stop_pending = 1;
            while (ac->stop_pending)
            {
                usleep(50);
            }
        }
    }
#endif
    BSA_AvOpenInit(&open_param);
    bdcpy(open_param.bd_addr, bd_addr);
    open_param.handle = conn->handle;
    /* Indicate if AVRC must be used */
    open_param.use_rc = TRUE;
    BT_LOGI("### a2dp_open address=%2x:%2x:%2x:%2x:%2x:%2x",
            bd_addr[0],
            bd_addr[1],
            bd_addr[2],
            bd_addr[3],
            bd_addr[4],
            bd_addr[5]);
    BT_LOGI("### Connecting to AV device handle %d", conn->handle);
    status = BSA_AvOpen(&open_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvOpen(%02X:%02X:%02X:%02X:%02X:%02X) failed: %d",
                open_param.bd_addr[0], open_param.bd_addr[1], open_param.bd_addr[2],
                open_param.bd_addr[3], open_param.bd_addr[4], open_param.bd_addr[5], status);
        return -1;
    }

    return 0;
}

static void *send_thread(void *arg)
{
    A2dpCtx *ac = arg;
    int error;
    unsigned char buffer[512];
    int count = 0;

    int fd = open("/tmp/audio", O_RDONLY);
    if (fd < 0)
    {
        BT_LOGE("### Open pa output failed :%s", strerror(errno));
        goto exit;
    }
    while (!ac->abort_request) {
        int r = read(fd, buffer, sizeof(buffer));
        if (r <= 0) {
            BT_LOGE("### Read pa output failed :%s", strerror(errno));
            break;
        }
        if (ac->abort_request || (ac->uipc_channel == UIPC_CH_ID_BAD) || (!ac->is_opened))
            break;

        count += 1;
        if (count % 1000 == 0)
            BT_LOGD("read data %d count %d", r, count);

        if (!UIPC_Send(ac->uipc_channel, 0, buffer, r)) {
            if (UIPC_Ioctl(ac->uipc_channel, UIPC_READ_ERROR, &error)) {
                if (error == UIPC_ENOMEM)
                {
                    BT_LOGI("UIPC buffer full! Pause playing");
                    sleep(3);

                    // TOOD pause --> resume
                    BT_LOGI("### UIPC buffer full! Resume playing");
                    //app_av_resume();
                }
                else
                {
                    BT_LOGE("UIPC_Send failed");
                }
            }
        }
    }

    close(fd);
exit:
    BT_LOGI("a2dp send thread exited");
    pthread_mutex_lock(&ac->mutex);
    ac->thread_running = 0;
    pthread_mutex_unlock(&ac->mutex);
    pthread_exit(NULL);
    return NULL;
}

int a2dp_pause_stop(A2dpCtx *ac, int pause)
{
    int status;
    tBSA_AV_STOP stop_param;

    /* stopping state before calling BSA api, call back happens asynchronously */
    /* TODO : protect critical section */
    //app_av_rc_change_play_status(BSA_AVRC_PLAYSTATE_STOPPED);

    /* Stop the AV stream */
    status = BSA_AvStopInit(&stop_param);
    stop_param.pause = pause;
    status = BSA_AvStop(&stop_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvStop failed: %d", status);
        return status;
    }

    return 0;
}

int a2dp_pause(A2dpCtx *ac)
{
    pthread_mutex_lock(&ac->mutex);
    pulse_pipe_stop(ac->pa);
    pthread_mutex_unlock(&ac->mutex);
    return a2dp_pause_stop(ac, 1);
}

int a2dp_stop(A2dpCtx *ac)
{
    pthread_mutex_lock(&ac->mutex);
    pulse_pipe_stop(ac->pa);
    pthread_mutex_unlock(&ac->mutex);
    return a2dp_pause_stop(ac, 0);
}


int a2dp_creat_thread(A2dpCtx *ac)
{
    pthread_attr_t     attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    ac->abort_request = 0;
    pthread_mutex_lock(&ac->mutex);
    if (ac->thread_running == 0) {
        if (0 == pthread_create(&ac->send_tid, &attr, send_thread, ac)) {
            ac->thread_running = 1;
        } else {
            ac->abort_request = 1;
            pthread_mutex_unlock(&ac->mutex);
            BT_LOGE("failed to creat thread");
            return -1;
        }
    }
    pthread_mutex_unlock(&ac->mutex);
    return 0;
}


int a2dp_start_with_aptx(A2dpCtx *ac)
{
    int status = 0;
    tBSA_AV_START start_param;

    BSA_AvStartInit(&start_param);

    //aptx codec;
    start_param.media_feeding.format = BSA_AV_CODEC_APTX;
    start_param.media_feeding.cfg.aptx.sampling_freq = A2DP_SIMPLE_RATE;
    start_param.media_feeding.cfg.aptx.ch_mode = BSA_AV_CHANNEL_MODE_STEREO;
    start_param.feeding_mode = 1 ? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = 20; /* convert us to ms, synchronous feeding mode only*/

    /* Content Protection */
    start_param.cp_id = 0;
    start_param.scmst_flag = 0x0;

    status = BSA_AvStart(&start_param);
    if (status == BSA_ERROR_SRV_AV_FEEDING_NOT_SUPPORTED)
    {
        BT_LOGE("BSA_AvStart failed because file encoding is not supported by remote devices");
       return status;
    }
    else if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvStart failed: %d", status);
        return status;
    }

    pthread_mutex_lock(&ac->mutex);
    pulse_pipe_init(ac->pa);
    pthread_mutex_unlock(&ac->mutex);

    return a2dp_creat_thread(ac);
}

int a2dp_start(A2dpCtx *ac)
{
    int status;
    tBSA_AV_START start_param;

    BSA_AvStartInit(&start_param);

    // only use pcm codec
    start_param.media_feeding.format = 0x5; // Raw PCM codec;
    start_param.media_feeding.cfg.pcm.sampling_freq = A2DP_SIMPLE_RATE;
    start_param.media_feeding.cfg.pcm.num_channel = 2;
    start_param.media_feeding.cfg.pcm.bit_per_sample = 16;
    start_param.feeding_mode = 1 ? BSA_AV_FEEDING_ASYNCHRONOUS: BSA_AV_FEEDING_SYNCHRONOUS;
    start_param.latency = 20; /* convert us to ms, synchronous feeding mode only*/

    /* Content Protection */
    start_param.cp_id = 0;
    start_param.scmst_flag = 0x0;

    status = BSA_AvStart(&start_param);
    if (status == BSA_ERROR_SRV_AV_FEEDING_NOT_SUPPORTED)
    {
        BT_LOGE("BSA_AvStart failed because file encoding is not supported by remote devices");
        return status;
    }
    else if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvStart failed: %d", status);
        return status;
    }

    pthread_mutex_lock(&ac->mutex);
    pulse_pipe_init(ac->pa);
    pthread_mutex_unlock(&ac->mutex);
    return a2dp_creat_thread(ac);
}

int a2dp_close_device(A2dpCtx *ac, BD_ADDR bd_addr)
{
    int status;
    tBSA_AV_CLOSE close_param;

    tAPP_AV_CONNECTION *conn = app_av_find_connection_by_bd_addr(bd_addr);
    if (!conn || !conn->is_open)
    {
        BT_LOGW("device not opened");
        return -1;
    }
    /* Close av connection */
    status = BSA_AvCloseInit(&close_param);
    close_param.handle = conn->handle;
    //conn->is_open = 0;
    status = BSA_AvClose(&close_param);

    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_AvClose failed status = %d", status);
    }
    return status;
}

int a2dp_close(A2dpCtx *ac)
{
    tBSA_AV_CLOSE close_param;
    int status;

    /* Close av connection */
    for (int i = 0; i < MAX_AV_CONN; ++i)
    {
        tAPP_AV_CONNECTION *conn = ac->connections + i;
        if (conn->is_open)
        {
            status = BSA_AvCloseInit(&close_param);
            close_param.handle = conn->handle;
            status = BSA_AvClose(&close_param);

            if (status != BSA_SUCCESS)
            {
                BT_LOGE("BSA_AvClose failed status = %d", status);
            }
            //conn->is_open = 0;
        }
    }
    //ac->is_opened = 0;
    return 0;
}

int a2dp_disable(A2dpCtx *ac)
{
    tBSA_AV_DISABLE disable_param;
    tAPP_AV_CONNECTION *conn = NULL;

    BT_CHECK_HANDLE(ac);

    ac->abort_request = 1;
    if (ac->is_registered) {
        a2dp_stop(ac);
        pulse_destroy(ac->pa);
        ac->pa = NULL;
        conn = app_av_find_connection_by_status(TRUE, TRUE);
        if (conn) {
            int times = 10;
            a2dp_close(ac);
            while (times-- && app_av_find_connection_by_status(TRUE, TRUE)) {
                usleep(200*1000);
            }
        }

        if (ac->uipc_channel != UIPC_CH_ID_BAD) {
            UIPC_Close(ac->uipc_channel);
            ac->uipc_channel = UIPC_CH_ID_BAD;
        }

        a2dp_deregister(ac);

        BSA_AvDisableInit(&disable_param);
        BSA_AvDisable(&disable_param);
    }
    while (ac->thread_running) {
        BT_LOGW("###wait send thread exit");
        usleep(50*1000);
    }
    ac->is_registered = 0;
    return 0;
}

int a2dp_is_enabled(A2dpCtx *ac)
{
    return ac ? ac->is_registered : 0;
}

int a2dp_is_opened(A2dpCtx *ac)
{
    return ac ? ac->is_opened : 0;
}

int a2dp_set_listener(A2dpCtx *ac, a2dp_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(ac);
    ac->listener = listener;
    ac->listener_handle = listener_handle;
    return 0;
}

int a2dp_send_rc_command(A2dpCtx *ac, uint8_t cmd)
{
    int status;
    tBSA_AV_REM_CMD bsa_av_rc_cmd;

    /* Send remote control command */
    status = BSA_AvRemoteCmdInit(&bsa_av_rc_cmd);
    bsa_av_rc_cmd.rc_handle = get_current_rc_handle(ac);
    bsa_av_rc_cmd.key_state = BSA_AV_STATE_PRESS;
    bsa_av_rc_cmd.rc_id = (tBSA_AV_RC)cmd;
    bsa_av_rc_cmd.label = ac->label++; /* Just used to distinguish commands */
    if (ac->label > 80000)
        ac->label = 0;
    status = BSA_AvRemoteCmd(&bsa_av_rc_cmd);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("### BSA_AvRemoteCmd failed: %d", status);
        return status;
    }
    return 0;
}

int a2dp_send_rc_inc_volume(A2dpCtx *ac)
{
   return a2dp_send_rc_command(ac, BSA_AV_RC_VOL_UP);
}

int a2dp_send_rc_dec_volume(A2dpCtx *ac)
{
    return a2dp_send_rc_command(ac, BSA_AV_RC_VOL_DOWN);
}

int a2dp_get_connected_devices(A2dpCtx *ac, BTDevice *dev_array, int arr_len)
{
    int r = 0;

    for (int i = 0; i < MAX_AV_CONN; ++i)
    {
        if (r >= arr_len)
            break;

        if (ac->connections[i].is_open)
        {
            BTDevice *dev = dev_array + r;
            snprintf(dev->name, sizeof(dev->name), "%s", ac->connections[i].name);
            bdcpy(dev->bd_addr, ac->connections[i].bd_addr);
            r += 1;
        }
    }

    return r;
}

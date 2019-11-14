#include "a2dp_sink.h"

#include <bsa_rokid/bsa_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#ifdef PCM_ALSA
#include "alsa/asoundlib.h"
#define APP_AVK_ASLA_DEV "default"
#endif

#include <cutils/properties.h>
#include <utils/Timers.h>
#include "app_manager.h"
#include "app_services.h"
#include "app_utils.h"
#include "app_disc.h"
#include "aac_latm_decoder.h"
#include "app_xml_utils.h"


#ifndef BSA_AVK_SECURITY
#define BSA_AVK_SECURITY    BSA_SEC_AUTHORIZATION
#endif
#ifndef BSA_AVK_FEATURES
#define BSA_AVK_FEATURES    (BSA_AVK_FEAT_DELAY_RPT)
#endif
#ifndef BSA_RC_FEATURES
#if (defined(AVRC_COVER_ART_INCLUDED) && AVRC_COVER_ART_INCLUDED == TRUE)
#define BSA_RC_FEATURES    (BSA_RC_FEAT_RCCT|BSA_RC_FEAT_RCTG|\
                             BSA_RC_FEAT_VENDOR|BSA_RC_FEAT_METADATA|\
                             BSA_RC_FEAT_BROWSE|BSA_RC_FEAT_COVER_ART)
#else
#define BSA_RC_FEATURES    (BSA_RC_FEAT_RCCT|BSA_RC_FEAT_RCTG|\
                             BSA_RC_FEAT_VENDOR|BSA_RC_FEAT_METADATA|\
                             BSA_RC_FEAT_BROWSE)
#endif
#endif

#ifndef BSA_AVK_DUMP_RX_DATA
#define BSA_AVK_DUMP_RX_DATA FALSE
#endif

#ifndef BSA_AVK_AAC_SUPPORTED
#define BSA_AVK_AAC_SUPPORTED FALSE
#endif

/* bitmask of events that BSA client is interested in registering for notifications */
tBSA_AVK_REG_NOTIFICATIONS reg_notifications =
        (1 << (BSA_AVK_RC_EVT_PLAY_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_REACHED_END - 1)) |
        (1 << (BSA_AVK_RC_EVT_TRACK_REACHED_START - 1)) |
        (1 << (BSA_AVK_RC_EVT_PLAY_POS_CHANGED - 1)) |
        (1 << (BSA_AVK_RC_EVT_BATTERY_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_SYSTEM_STATUS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_APP_SETTING_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_NOW_PLAYING_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_AVAL_PLAYERS_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_ADDR_PLAYER_CHANGE - 1)) |
        (1 << (BSA_AVK_RC_EVT_UIDS_CHANGE - 1));

enum {
    A2DPK_STATE_UNREGISTER,
    A2DPK_STATE_REGISTER,
    A2DPK_STATE_ENABLED,
    A2DPK_STATE_OPENING,
    A2DPK_STATE_OPENED,
    A2DPK_STATE_STARTING,
    A2DPK_STATE_STARTED,
    A2DPK_STATE_CLOSING,
    A2DPK_STATE_CLOSED,
};

extern int rokidbt_disc_read_remote_device_name(void *handle, BTAddr bd_addr, tBSA_TRANSPORT transport, 
                                                            tBSA_DISC_CBACK *user_dis_cback);

static int a2dpk_reg_notfn_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label, UINT8 event, tBSA_AVK_CODE code);
static int a2dpk_set_abs_vol_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label);
static A2dpSink *_a2dpk;
static void a2dpk_uipc_callback(BT_HDR *p_msg);

static int get_command_label(A2dpSink *as)
{
    if (!as)
        return 0;
    if (as->transaction_label >= 15)
        as->transaction_label = 0;
    return as->transaction_label ++;
}
static int a2dpk_creat_aac_decoder(A2dpSink *as, int channel, int sample, int bit)
{
    int latency = 600;
    int ret = 0;

    BT_CHECK_HANDLE(as);
    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;
    BTConnection *connection = as->connections + as->conn_index;

    //check pas have to free if hav new format
    if ((connection->sample_rate != sample)
            || (connection->num_channel != channel)
            || (connection->bit_per_sample != bit)) {
        pthread_mutex_lock(&as->mutex);
        if (as->aac_decoder) {
            aac_decode_close(as->aac_decoder);
            as->aac_decoder = NULL;
        }
        if (as->pas) {
            pa_simple_free(as->pas);
            as->pas = NULL;
        }
        pthread_mutex_unlock(&as->mutex);
    }
    connection->sample_rate = sample;
    connection->num_channel = channel;
    connection->bit_per_sample = bit;
    BT_LOGI("Sampling rate:%d, number of channel:%d bit per sample:%d",
            connection->sample_rate, connection->num_channel,
            connection->bit_per_sample);

    pthread_mutex_lock(&as->mutex);
    if (!as->aac_decoder)
        as->aac_decoder = aac_decoder_create(sample, channel, bit);
    if (as->aac_decoder) {
        int error;
        pa_sample_format_t format;

        if (bit == 8)
            format = PA_SAMPLE_U8;
        else
            format = PA_SAMPLE_S16LE;
        /* The Sample format to use */
        pa_sample_spec ss = {
                .format = format,
                .rate = sample,
                .channels = channel,
        };
        pa_buffer_attr attr = {0};

        attr.maxlength = (uint32_t) -1;
        attr.tlength = (uint32_t) -1;
        attr.minreq = (uint32_t) -1;
        attr.prebuf = ss.rate * ss.channels * (bit / 8) * latency /1000;
        attr.fragsize = attr.tlength;
        /* Create a new playback stream */
        if (!as->pas) {
            if (!(as->pas = pa_simple_new(NULL, "BT-A2DP-SINK", PA_STREAM_PLAYBACK, NULL,
                                "playback", &ss, NULL, &attr, &error))) {
                BT_LOGE("pa_simple_new() failed: %s", pa_strerror(error));
                as->pas = NULL;
                ret = -1;
            }
        }
    } else {
        BT_LOGE("failed to aac_decoder_create");
        ret = -1;
    }
    pthread_mutex_unlock(&as->mutex);

    return ret;
}

static int a2dpk_start_ready(A2dpSink *as, tBSA_AVK_MSG *p_data)
{
    char *avk_format_display[12] = {"SBC", "M12", "M24", "??", "ATRAC", "PCM", "APT-X", "SEC"};
    int ret = 0;
    int latency = 600; // 600 ms

    BT_LOGD("AVK start: format %s", avk_format_display[p_data->start_streaming.media_receiving.format]);
    if (p_data->start_streaming.media_receiving.format < (sizeof(avk_format_display)/sizeof(avk_format_display[0]))) {
        BT_LOGI("AVK start: format %s", avk_format_display[p_data->start_streaming.media_receiving.format]);
    } else {
        BT_LOGI("AVK start: format code (%u) unknown", p_data->start_streaming.media_receiving.format);
    }

    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;
    BTConnection *connection = as->connections + as->conn_index;
    connection->format = p_data->start_streaming.media_receiving.format;
    if (connection->format == BSA_AVK_CODEC_PCM) {

        //check pas have to free if hav new format 
        if (as->pas) {
            if ((connection->sample_rate != p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq)
                || (connection->num_channel != p_data->start_streaming.media_receiving.cfg.pcm.num_channel)
                || (connection->bit_per_sample != p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample)) {
                pthread_mutex_lock(&as->mutex);
                pa_simple_free(as->pas);
                as->pas = NULL;
                pthread_mutex_unlock(&as->mutex);
            }
        }
        connection->sample_rate = p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq;
        connection->num_channel = p_data->start_streaming.media_receiving.cfg.pcm.num_channel;
        connection->bit_per_sample = p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample;
        BT_LOGI("Sampling rate:%d, number of channel:%d bit per sample:%d",
            p_data->start_streaming.media_receiving.cfg.pcm.sampling_freq,
            p_data->start_streaming.media_receiving.cfg.pcm.num_channel,
            p_data->start_streaming.media_receiving.cfg.pcm.bit_per_sample);

        pa_sample_format_t format;
        int error;
        if (connection->bit_per_sample == 8)
            format = PA_SAMPLE_U8;
        else
            format = PA_SAMPLE_S16LE;

        /* The Sample format to use */
        pa_sample_spec ss = {
            .format = format,
            .rate = connection->sample_rate,
            .channels = connection->num_channel
        };
        pa_buffer_attr attr = {0};

        attr.maxlength = (uint32_t) -1;//pa_usec_to_bytes(latency,&ss);
        attr.tlength = (uint32_t) -1;
        attr.minreq = (uint32_t) -1;
        attr.prebuf = connection->sample_rate * connection->num_channel * (connection->bit_per_sample / 8) * latency /1000;
        attr.fragsize = (uint32_t) -1;

#ifdef A2DP_DUMP_DATA
        if (!as->fp)
            as->fp = fopen("/data/bt_a2dp_sink.pcm", "wb");
#endif
        //TODO open new pulseaudio
        //
        /* Create a new playback stream */
        pthread_mutex_lock(&as->mutex);
        if (!as->pas) {
            if (!(as->pas = pa_simple_new(NULL, "BT-A2DP-SINK", PA_STREAM_PLAYBACK, NULL,
                            "playback", &ss, NULL, &attr, &error))) {
                BT_LOGE("pa_simple_new() failed: %s", pa_strerror(error));
                as->pas = NULL;
                ret = -1;
            }
        }
        pthread_mutex_unlock(&as->mutex);
    }
    else if (connection->format == BSA_AVK_CODEC_M24) {
        BT_LOGI("aac decode");
        connection->aac.obj_type = p_data->start_streaming.media_receiving.cfg.aac.obj_type;
        connection->aac.samp_freq = p_data->start_streaming.media_receiving.cfg.aac.samp_freq;
        connection->aac.chnl = p_data->start_streaming.media_receiving.cfg.aac.chnl;
        connection->aac.vbr = p_data->start_streaming.media_receiving.cfg.aac.vbr;
        connection->aac.bitrate = p_data->start_streaming.media_receiving.cfg.aac.bitrate;
        BT_LOGI("obj_type %d,  samp_freq:%d, chnl :%d, vbr:%d, bitrate:%d",
            connection->aac.obj_type, connection->aac.samp_freq, connection->aac.chnl,
            connection->aac.vbr, connection->aac.bitrate);
    #ifdef A2DP_DUMP_DATA
        if (!as->fp)
            as->fp = fopen("/data/bt_a2dp_sink.aac", "wb");
    #endif
        as->need_parse_aac_config =1;
    }
    else {
        BT_LOGE("Unsupport format");
        ret = -1;
    }
    return ret;
}

static void on_stop(A2dpSink *as)
{
    pthread_mutex_lock(&as->mutex);
    as->is_started = 0;
    as->is_playing = 0;
    if (as->aac_decoder) {
        aac_decode_close(as->aac_decoder);
        as->aac_decoder = NULL;
    }
    as->aac_remain.len = 0;
    as->aac_remain.need_len = 0;
    if (as->pas) {
        pa_simple_free(as->pas);
        as->pas = NULL;
    }
#ifdef A2DP_DUMP_DATA
    if (as->fp) {
        fclose(as->fp);
        as->fp = NULL;
    }
#endif
    pthread_mutex_unlock(&as->mutex);
}

void _reset_connection(BTConnection *connection)
{
    memset(connection, 0, sizeof(*connection));
    connection->in_use = FALSE;
}

void reset_connection(BTConnection *connections, int count, BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < count; index++)
    {
        if (bdcmp(connections[index].bda_connected, bd_addr) == 0)
        {
            _reset_connection(connections + index);
        }
    }
}

BTConnection *find_connection_by_addr(BTConnection *connections, int count, BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < count; index++)
    {
        if (bdcmp(connections[index].bda_connected, bd_addr) == 0 &&
                (connections[index].in_use))
        {
            return &connections[index];
        }
    }
    return NULL;
}

bool get_avail_connection(A2dpSink *as)
{
    int index;
    for (index = 0; index < MAX_CONNECTIONS; index++) {
        if (as->connections[index].in_use == 0) {
            return true;
        }
    }

    return false;
}

BTConnection *add_connection(BTConnection *connections, int count, BD_ADDR bd_addr)
{
    int index;
    for (index = 0; index < count; index++)
    {
        if (bdcmp(connections[index].bda_connected, bd_addr) == 0)
        {
            connections[index].in_use = TRUE;
            return &connections[index];
        }
    }

    for (index = 0; index < count; index++)
    {
        if (connections[index].in_use == FALSE)
        {
            bdcpy(connections[index].bda_connected, bd_addr);
            connections[index].in_use = TRUE;
            connections[index].index = index;
            return &connections[index];
        }
    }
    return NULL;
}

BTConnection *find_connection_by_av_handle(BTConnection *connections, int count, UINT8 handle)
{
    int index;
    for (index = 0; index < count; index++)
    {
        if (connections[index].ccb_handle == handle)
            return &connections[index];
    }
    return NULL;
}

BTConnection *find_connection_by_rc_handle(BTConnection *connections, int count, UINT8 rc_handle)
{
    int index;
    for (index = 0; index < count; index++)
    {
        if (connections[index].rc_handle == rc_handle)
            return &connections[index];
    }
    return NULL;
}

int a2dpk_get_element_attr_command(A2dpSink *as)
{
    int status;
    int rc_handle;
    BTConnection *connection;
    tBSA_AVK_GET_ELEMENT_ATTR bsa_avk_get_elem_attr_cmd;

    BT_CHECK_HANDLE(as);
    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;
    connection = &as->connections[as->conn_index];

    rc_handle = connection->rc_handle;
    UINT8 attrs[]  = {AVRC_MEDIA_ATTR_ID_TITLE,
                      AVRC_MEDIA_ATTR_ID_ARTIST,
                      AVRC_MEDIA_ATTR_ID_ALBUM,
                      AVRC_MEDIA_ATTR_ID_TRACK_NUM,
                      AVRC_MEDIA_ATTR_ID_NUM_TRACKS,
                      AVRC_MEDIA_ATTR_ID_GENRE,
                      AVRC_MEDIA_ATTR_ID_PLAYING_TIME};
    UINT8 num_attr  = 7;

    /* Send command */
    status = BSA_AvkGetElementAttrCmdInit(&bsa_avk_get_elem_attr_cmd);
    bsa_avk_get_elem_attr_cmd.rc_handle = rc_handle;
    bsa_avk_get_elem_attr_cmd.label = get_command_label(as); /*used to distinguish commands*/

    bsa_avk_get_elem_attr_cmd.num_attr = num_attr;
    memcpy(bsa_avk_get_elem_attr_cmd.attrs, attrs, sizeof(attrs));

    status = BSA_AvkGetElementAttrCmd(&bsa_avk_get_elem_attr_cmd);
    if (status != BSA_SUCCESS) {
        BT_LOGE("Unable to Send app_avk_rc_get_element_attr_command %d", status);
    }
    return status;
}

static void rkd_a2dpk_discovery_callback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data)
{
    A2dpSink *as = _a2dpk;

    if (!as || !p_data) return;
    //BT_LOGV("\n\tevent%d\n", event);
    switch (event)
    {
    case BSA_DISC_NEW_EVT:
        BT_LOGI("\tName:%s", p_data->disc_new.name);
        BT_LOGI("\tServices:0x%08x (%s)",
               (int) p_data->disc_new.services,
               app_service_mask_to_string(p_data->disc_new.services));

        if ((p_data->disc_new.services & BSA_A2DP_SRC_SERVICE_MASK) == 0) {
            BT_LOGE("current device is not a a2dp src device");
            a2dpk_close_device(as, p_data->disc_new.bd_addr);
        }

        break;
    case BSA_DISC_CMPL_EVT:
        break;
    case BSA_DISC_REMOTE_NAME_EVT:
        if (p_data->dev_info.status == BSA_SUCCESS) {
            BTConnection *connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, p_data->remote_name.bd_addr);
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

static void a2dpk_callback(tBSA_AVK_EVT event, tBSA_AVK_MSG *p_data)
{
    BTConnection *connection = NULL;
    int ret;
    tBSA_AVK_REM_RSP RemRsp;
    A2dpSink *as = _a2dpk;
    BT_AVK_MSG msg = {0};

    if (!as || !p_data) return;

    BT_LOGD("\n event %d", event);
    switch (event)
    {
    case BSA_AVK_REGISTER_EVT:
        if(p_data->reg_evt.status == BSA_SUCCESS && as->uipc_channel == UIPC_CH_ID_BAD)
        {
            /* Save UIPC channel */
            as->uipc_channel = p_data->reg_evt.uipc_channel;
            /* open the UIPC channel to receive the pcm */
            if (UIPC_Open(p_data->reg_evt.uipc_channel, a2dpk_uipc_callback) == FALSE)
            {
                BT_LOGE("Unable to open UIPC channel");
                break;
            }
        }
        break;
    case BSA_AVK_OPEN_EVT:
        BT_LOGI("BSA_AVK_OPEN_EVT status 0x%x ccb_handle %d", p_data->sig_chnl_open.status, p_data->sig_chnl_open.ccb_handle);

        BTDevice dev = {0};

        bdcpy(dev.bd_addr, p_data->sig_chnl_open.bd_addr);
        if (rokidbt_find_bonded_device(as->caller, &dev) == 1) {
            snprintf((char *)msg.sig_chnl_open.name, sizeof(msg.sig_chnl_open.name), "%s", dev.name);
        } else if (rokidbt_find_scaned_device(as->caller, &dev) == 1) {
            snprintf((char *)msg.sig_chnl_open.name, sizeof(msg.sig_chnl_open.name), "%s", dev.name);
        }
        if (p_data->sig_chnl_open.status == BSA_SUCCESS) {
            connection = add_connection(as->connections, MAX_CONNECTIONS, p_data->sig_chnl_open.bd_addr);
            if (!connection) {
                BT_LOGE("### BSA_AVK_OPEN_EVT cannot allocate connection");
                break;
            }
            connection->ccb_handle = p_data->sig_chnl_open.ccb_handle;
            connection->is_open = TRUE;
            connection->is_streaming_chl_open = FALSE;
            as->is_opened = 1;
            as->conn_index = connection->index;
            snprintf(connection->name, sizeof(connection->name), "%s", dev.name);
            rokidbt_disc_read_remote_device_name(as->caller, p_data->sig_chnl_open.bd_addr, BSA_TRANSPORT_BR_EDR,
                                    rkd_a2dpk_discovery_callback);

            BT_LOGI("AVK connected index %d to device %02X:%02X:%02X:%02X:%02X:%02X",
                    connection->index,
                    p_data->sig_chnl_open.bd_addr[0],
                    p_data->sig_chnl_open.bd_addr[1],
                    p_data->sig_chnl_open.bd_addr[2],
                    p_data->sig_chnl_open.bd_addr[3],
                    p_data->sig_chnl_open.bd_addr[4],
                    p_data->sig_chnl_open.bd_addr[5]);
        }

        if (as->listener) {
            msg.sig_chnl_open.status = p_data->sig_chnl_open.status;
            msg.sig_chnl_open.idx = as->conn_index;
            bdcpy(msg.sig_chnl_open.bd_addr, p_data->sig_chnl_open.bd_addr);
            as->listener(as->listener_handle, BT_AVK_OPEN_EVT, &msg);
        }
        memset(as->open_pending_addr, 0x0, sizeof(as->open_pending_addr));
        as->open_pending = FALSE;
        break;
    case BSA_AVK_CLOSE_EVT:
        /* Close event, reason BTA_AVK_CLOSE_STR_CLOSED or BTA_AVK_CLOSE_CONN_LOSS*/
        BT_LOGI("BSA_AVK_CLOSE_EVT status 0x%x, handle %d", p_data->sig_chnl_close.status, p_data->sig_chnl_close.ccb_handle);

        connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, p_data->sig_chnl_close.bd_addr);
        if (!connection) {
            BT_LOGE("### BSA_AVK_CLOSE_EVT unknown handle %d", p_data->sig_chnl_close.ccb_handle);
            break;
        }

        memset(as->open_pending_addr, 0x0, sizeof(as->open_pending_addr));
        as->open_pending = FALSE;
        connection->is_started_streaming = FALSE;

        if (as->listener) {
            msg.sig_chnl_close.status = p_data->sig_chnl_close.status;
            msg.sig_chnl_close.idx = connection->index;
            snprintf(msg.sig_chnl_close.name, sizeof(msg.sig_chnl_close.name), "%s", connection->name);
            bdcpy(msg.sig_chnl_close.bd_addr, p_data->sig_chnl_close.bd_addr);
            as->listener(as->listener_handle, BT_AVK_CLOSE_EVT, &msg);
        }
        as->is_opened = 0;
        _reset_connection(connection);
        break;
    case BSA_AVK_STR_OPEN_EVT:
        BT_LOGI("BSA_AVK_STR_OPEN_EVT status 0x%x", p_data->stream_chnl_open.status);
        connection = find_connection_by_av_handle(as->connections, MAX_CONNECTIONS,  p_data->stream_chnl_open.ccb_handle);
        if(connection == NULL) {
            break;
        }
        connection->is_streaming_chl_open = TRUE;
        if ((as->listener) && (as->is_opened == 1)) {
            msg.stream_chnl_open.status = p_data->stream_chnl_open.status;
            msg.stream_chnl_open.idx = connection->index;
            snprintf(msg.stream_chnl_open.name, sizeof(msg.stream_chnl_open.name), "%s", connection->name);
            bdcpy(msg.stream_chnl_open.bd_addr, p_data->stream_chnl_open.bd_addr);
            as->listener(as->listener_handle, BT_AVK_STR_OPEN_EVT, &msg);
        }
        break;

    case BSA_AVK_STR_CLOSE_EVT:
        BT_LOGI("BSA_AVK_STR_CLOSE_EVT streaming chn closed handle: %d", p_data->stream_chnl_close.ccb_handle);

        connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, p_data->stream_chnl_close.bd_addr);
        if(connection == NULL) {
            BT_LOGE("### BSA_AVK_STR_CLOSE_EVT unknown handle %d", p_data->sig_chnl_close.ccb_handle);
            break;
        }
        if ((as->listener) && (as->is_opened == 1)) {
            msg.stream_chnl_close.status = p_data->stream_chnl_close.status;
            msg.stream_chnl_close.idx = connection->index;
            snprintf(msg.stream_chnl_close.name, sizeof(msg.stream_chnl_close.name), "%s", connection->name);
            bdcpy(msg.stream_chnl_close.bd_addr, p_data->stream_chnl_close.bd_addr);
            as->listener(as->listener_handle, BT_AVK_STR_CLOSE_EVT, &msg);
        }
        connection->is_streaming_chl_open = FALSE;
        break;

        case BSA_AVK_DELAY_RPT_RSP_EVT:
            BT_LOGD("BSA_AVK_DELAY_RPT_RSP_EVT status 0x%x", p_data->delay_rpt_rsp.status);
            BT_LOGD("device %02X:%02X:%02X:%02X:%02X:%02X\n",
                p_data->delay_rpt_rsp.bd_addr[0], p_data->delay_rpt_rsp.bd_addr[1],p_data->delay_rpt_rsp.bd_addr[2],
                p_data->delay_rpt_rsp.bd_addr[3], p_data->delay_rpt_rsp.bd_addr[4],p_data->delay_rpt_rsp.bd_addr[5]);
        break;
    case BSA_AVK_START_EVT:
        BT_LOGI("BSA_AVK_START_EVT status 0x%x discarded %d",
                p_data->start_streaming.status, p_data->start_streaming.discarded);

        /* We got START_EVT for a new device that is streaming but server discards the data
            because another stream is already active */
        if(p_data->start_streaming.discarded) {
            BT_LOGI("### start discarded");
            connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, p_data->start_streaming.bd_addr);
            if (connection) {
                connection->is_started_streaming = TRUE;
                connection->is_streaming_chl_open = TRUE;
            }

            break;
        }
        /* got Start event and device is streaming */
        else {
            connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, p_data->start_streaming.bd_addr);
            if ((connection == NULL) || connection->is_started_streaming) {
                break;
            }

            if (connection->is_streaming_chl_open == TRUE) {
               BT_LOGI("playing playing");
                ret = a2dpk_start_ready(as, p_data);
                if (!ret) {
                    as->is_started = 1;
                    connection->is_started_streaming = TRUE;
                    as->conn_index = connection->index;
                    if ((as->listener) && (as->is_opened == 1)) {
                        msg.start_streaming.status = p_data->start_streaming.status;
                        msg.start_streaming.idx = connection->index;
                        bdcpy(msg.start_streaming.bd_addr, p_data->start_streaming.bd_addr);
                        as->listener(as->listener_handle, BT_AVK_START_EVT, &msg);
                    }
                }
            }
        }
        break;

    case BSA_AVK_STOP_EVT:
        BT_LOGI("BSA_AVK_STOP_EVT handle: %d  Suspended: %d", p_data->stop_streaming.ccb_handle, p_data->stop_streaming.suspended);

        connection = find_connection_by_av_handle(as->connections, MAX_CONNECTIONS, p_data->stop_streaming.ccb_handle);
        if(connection == NULL) {
            break;
        }
        as->is_started = 0;
        as->is_playing = 0;
        connection->is_started_streaming = FALSE;
        /* Stream was suspended */
        if(p_data->stop_streaming.suspended) {
        }
        /* stream was closed */
        else {
            on_stop(as);
        }
        if ((as->listener) && (as->is_opened == 1)) {
            msg.stop_streaming.status = p_data->stop_streaming.status;
            msg.stop_streaming.idx = connection->index;
            bdcpy(msg.stop_streaming.bd_addr, p_data->stop_streaming.bd_addr);
            msg.stop_streaming.suspended = p_data->stop_streaming.suspended;
            as->listener(as->listener_handle, BT_AVK_STOP_EVT, &msg);
        }
        break;
    case BSA_AVK_RC_OPEN_EVT:
        BT_LOGI("BSA_AVK_RC_OPEN_EVT handle: %d", p_data->rc_open.rc_handle);
        if(p_data->rc_open.status == BSA_SUCCESS) {
            if(as->open_pending && bdcmp(as->open_pending_addr, p_data->rc_open.bd_addr) != 0 && A2DP_SINK_OPEN_PENDING_LINK_UP) {
                tBSA_STATUS status;
                tBSA_AVK_CLOSE bsa_avk_close_param;
                BT_LOGW("we already link devices yet, not want another connect!!!");

                BSA_AvkCloseInit(&bsa_avk_close_param);
                bsa_avk_close_param.rc_handle = p_data->rc_open.rc_handle;

                status = BSA_AvkClose(&bsa_avk_close_param);
                if (status != BSA_SUCCESS) {
                    BT_LOGE("###Unable to Close AVK connection with status %d", status);
                }
                break;
            }
            connection = add_connection(as->connections, MAX_CONNECTIONS, p_data->rc_open.bd_addr);
            if(connection == NULL) {
                BT_LOGE("BSA_AVK_RC_OPEN_EVT could not allocate connection");
                break;
            }

            BT_LOGD("BSA_AVK_RC_OPEN_EVT connection index %d peer_feature=0x%x rc_handle=%d",
                connection->index, p_data->rc_open.peer_features, p_data->rc_open.rc_handle);
            connection->rc_handle = p_data->rc_open.rc_handle;
            connection->peer_features =  p_data->rc_open.peer_features;
            connection->peer_version = p_data->rc_open.peer_version;
            connection->is_rc_open = TRUE;
        }
        if ((as->listener) && (as->is_opened == 1)) {
            msg.rc_open.status = p_data->rc_open.status;
            bdcpy(msg.rc_open.bd_addr, p_data->rc_open.bd_addr);
            msg.rc_open.idx = connection ? connection->index : as->conn_index;
            as->listener(as->listener_handle, BT_AVK_RC_OPEN_EVT, &msg);
        }
        break;
    case BSA_AVK_RC_PEER_OPEN_EVT:
        BT_LOGD("BSA_AVK_RC_PEER_OPEN_EVT peer_feature=0x%x rc_handle=%d",
            p_data->rc_open.peer_features, p_data->rc_open.rc_handle);

        connection = find_connection_by_rc_handle(as->connections, MAX_CONNECTIONS, p_data->rc_open.rc_handle);
        if(connection == NULL) {
            BT_LOGE("BSA_AVK_RC_PEER_OPEN_EVT could not find connection handle %d", p_data->rc_open.rc_handle);
            break;
        }

        BT_LOGD("BSA_AVK_RC_PEER_OPEN_EVT peer_feature=0x%x rc_handle=%d",
            p_data->rc_open.peer_features, p_data->rc_open.rc_handle);

        connection->peer_features =  p_data->rc_open.peer_features;
        connection->peer_version = p_data->rc_open.peer_version;
        connection->is_rc_open = TRUE;
        if ((as->listener) && (as->is_opened == 1)) {
            msg.rc_open.status = p_data->rc_open.status;
            bdcpy(msg.rc_open.bd_addr, p_data->rc_open.bd_addr);
            msg.rc_open.idx = connection ? connection->index : as->conn_index;
            as->listener(as->listener_handle, BT_AVK_RC_OPEN_EVT, &msg);
        }
        break;

    case BSA_AVK_RC_CLOSE_EVT:
        BT_LOGI("BSA_AVK_RC_CLOSE_EVT");
        connection = find_connection_by_rc_handle(as->connections, MAX_CONNECTIONS, p_data->rc_close.rc_handle);
        if (!connection)
            break;

        connection->is_rc_open = FALSE;
        if ((as->listener) && (as->is_opened == 1)) {
            msg.rc_close.status = p_data->rc_close.status;
            msg.rc_close.idx = connection->index;
            as->listener(as->listener_handle, BT_AVK_RC_CLOSE_EVT, &msg);
        }
        break;
    case BSA_AVK_REMOTE_RSP_EVT:
        BT_LOGD("BSA_AVK_REMOTE_RSP_EVT");
        break;
    case BSA_AVK_REMOTE_CMD_EVT:
        BT_LOGD("BSA_AVK_REMOTE_CMD_EVT");
        BT_LOGD(" label:0x%x", p_data->remote_cmd.label);
        BT_LOGD(" op_id:0x%x", p_data->remote_cmd.op_id);
        BT_LOGD(" label:0x%x", p_data->remote_cmd.label);
        BT_LOGD(" avrc header");
        BT_LOGD("   ctype:0x%x", p_data->remote_cmd.hdr.ctype);
        BT_LOGD("   subunit_type:0x%x", p_data->remote_cmd.hdr.subunit_type);
        BT_LOGD("   subunit_id:0x%x", p_data->remote_cmd.hdr.subunit_id);
        BT_LOGD("   opcode:0x%x", p_data->remote_cmd.hdr.opcode);
        BT_LOGD(" len:0x%x", p_data->remote_cmd.len);
#if 1
        //APP_DUMP("data", p_data->remote_cmd.data, p_data->remote_cmd.len);

        BSA_AvkRemoteRspInit(&RemRsp);
        RemRsp.avrc_rsp = BSA_AVK_RSP_ACCEPT;
        RemRsp.label = p_data->remote_cmd.label;
        RemRsp.op_id = p_data->remote_cmd.op_id;
        RemRsp.key_state = p_data->remote_cmd.key_state;
        RemRsp.handle = p_data->remote_cmd.rc_handle;
        RemRsp.length = 0;
        BSA_AvkRemoteRsp(&RemRsp);
#endif
        break;

    case BSA_AVK_VENDOR_CMD_EVT:
        BT_LOGD("BSA_AVK_VENDOR_CMD_EVT");
        BT_LOGD(" code:0x%x", p_data->vendor_cmd.code);
        BT_LOGD(" company_id:0x%x", p_data->vendor_cmd.company_id);
        BT_LOGD(" label:0x%x", p_data->vendor_cmd.label);
        BT_LOGD(" len:0x%x", p_data->vendor_cmd.len);
#if (defined(BSA_AVK_DUMP_RX_DATA) && (BSA_AVK_DUMP_RX_DATA == TRUE))
        APP_DUMP("A2DP Data", p_data->vendor_cmd.data, p_data->vendor_cmd.len);
#endif
        break;

    case BSA_AVK_VENDOR_RSP_EVT:
        BT_LOGD("BSA_AVK_VENDOR_RSP_EVT");
        BT_LOGD(" code:0x%x", p_data->vendor_rsp.code);
        BT_LOGD(" company_id:0x%x", p_data->vendor_rsp.company_id);
        BT_LOGD(" label:0x%x", p_data->vendor_rsp.label);
        BT_LOGD(" len:0x%x", p_data->vendor_rsp.len);
        break;

    case BSA_AVK_CP_INFO_EVT:
        BT_LOGD("BSA_AVK_CP_INFO_EVT");
        if(p_data->cp_info.id == BSA_AVK_CP_SCMS_T_ID) {
            switch(p_data->cp_info.info.scmst_flag)
            {
                case BSA_AVK_CP_SCMS_COPY_NEVER:
                    BT_LOGD(" content protection:0x%x - COPY NEVER", p_data->cp_info.info.scmst_flag);
                    break;
                case BSA_AVK_CP_SCMS_COPY_ONCE:
                    BT_LOGD(" content protection:0x%x - COPY ONCE", p_data->cp_info.info.scmst_flag);
                    break;
                case BSA_AVK_CP_SCMS_COPY_FREE:
                case (BSA_AVK_CP_SCMS_COPY_FREE+1):
                    BT_LOGD(" content protection:0x%x - COPY FREE", p_data->cp_info.info.scmst_flag);
                    break;
                default:
                    BT_LOGD(" content protection:0x%x - UNKNOWN VALUE", p_data->cp_info.info.scmst_flag);
                    break;
            }
        }
        else {
            BT_LOGD("No content protection");
        }
        break;
    case BSA_AVK_REGISTER_NOTIFICATION_EVT:
        BT_LOGD("BSA_AVK_REGISTER_NOTIFICATION_EVT");
        if (p_data->reg_notif_cmd.reg_notif_cmd.event_id == AVRC_EVT_PLAY_STATUS_CHANGE) {
            BT_LOGI("play_status %d", p_data->reg_notif_rsp.rsp.param.play_status);
            msg.reg_notif.rsp.event_id = BT_AVRC_EVT_PLAY_STATUS_CHANGE;
            msg.reg_notif.rsp.param.play_status = p_data->reg_notif_rsp.rsp.param.play_status;
            switch(msg.reg_notif.rsp.param.play_status) {
                case AVRC_PLAYSTATE_PLAYING:
                    as->is_started = 1;
                    as->is_playing = 1;
                    if ((as->listener) && (as->is_opened == 1)) {
                        as->listener(as->listener_handle, BT_AVK_REGISTER_NOTIFICATION_EVT, &msg);
                    }
                    break;
                case AVRC_PLAYSTATE_STOPPED:
                case AVRC_PLAYSTATE_PAUSED:
                    as->is_started = 0;
                    as->is_playing = 0;
                    if ((as->listener) && (as->is_opened == 1)) {
                        as->listener(as->listener_handle, BT_AVK_REGISTER_NOTIFICATION_EVT, &msg);
                    }
                    break;
                default:
                    break;
            }
        }
        break;
    case BSA_AVK_LIST_PLAYER_APP_ATTR_EVT:
        BT_LOGD("BSA_AVK_LIST_PLAYER_APP_ATTR_EVT");
        break;
    case BSA_AVK_LIST_PLAYER_APP_VALUES_EVT:
        BT_LOGD("BSA_AVK_LIST_PLAYER_APP_VALUES_EVT");
        break;
    case BSA_AVK_SET_PLAYER_APP_VALUE_EVT:
        BT_LOGD("BSA_AVK_SET_PLAYER_APP_VALUE_EVT");
        break;
    case BSA_AVK_GET_PLAYER_APP_VALUE_EVT:
        BT_LOGD("BSA_AVK_GET_PLAYER_APP_VALUE_EVT");
        break;
    case BSA_AVK_GET_ELEM_ATTR_EVT:
        BT_LOGD("BSA_AVK_GET_ELEM_ATTR_EVT status %d", p_data->elem_attr.status);
        {
            int i;
            int n = p_data->elem_attr.num_attr;
            msg.elem_attr.num_attr = p_data->elem_attr.num_attr;
            for (i = 0; i < n; i++)
            {
                tBSA_AVK_ATTR_ENTRY *attr_entry = &p_data->elem_attr.attr_entry[i];
                BT_LOGI("id %d name:%s", attr_entry->attr_id, attr_entry->name.data);
                msg.elem_attr.attr_entry[i].attr_id = attr_entry->attr_id;
                snprintf(msg.elem_attr.attr_entry[i].data, sizeof(msg.elem_attr.attr_entry[i].data), "%s", attr_entry->name.data);
            }
        }
        if ((as->listener) && (as->is_opened == 1)) {
            as->listener(as->listener_handle, BT_AVK_GET_ELEM_ATTR_EVT, &msg);
        }
        break;
    case BSA_AVK_GET_PLAY_STATUS_EVT:
        BT_LOGI("BSA_AVK_GET_PLAY_STATUS_EVT status :: %d", p_data->get_play_status.rsp.play_status);
        switch(p_data->get_play_status.rsp.play_status) {
            case AVRC_PLAYSTATE_PLAYING:
                as->is_started = 1;
                as->is_playing = 1;
                break;
            case AVRC_PLAYSTATE_PAUSED:
            case AVRC_PLAYSTATE_STOPPED:
                as->is_started = 0;
                as->is_playing = 0;
                break;
            default:
                break;
        }
        if ((as->listener) && (as->is_opened == 1)) {
            msg.get_play_status.play_status = p_data->get_play_status.rsp.play_status;
            as->listener(as->listener_handle, BT_AVK_GET_PLAY_STATUS_EVT, &msg);
        }
        break;
    case BSA_AVK_SET_ADDRESSED_PLAYER_EVT:
        BT_LOGD("BSA_AVK_SET_ADDRESSED_PLAYER_EVT");
        break;
    case BSA_AVK_SET_BROWSED_PLAYER_EVT:
        BT_LOGD("BSA_AVK_SET_BROWSED_PLAYER_EVT");
        break;
    case BSA_AVK_GET_FOLDER_ITEMS_EVT:
        BT_LOGD("BSA_AVK_GET_FOLDER_ITEMS_EVT");
        break;
    case BSA_AVK_CHANGE_PATH_EVT:
        BT_LOGD("BSA_AVK_CHANGE_PATH_EVT");
        break;
    case BSA_AVK_GET_ITEM_ATTR_EVT:
        BT_LOGD("BSA_AVK_GET_ITEM_ATTR_EVT");
        break;
    case BSA_AVK_PLAY_ITEM_EVT:
        BT_LOGD("BSA_AVK_PLAY_ITEM_EVT");
        break;
    case BSA_AVK_ADD_TO_NOW_PLAYING_EVT:
        BT_LOGD("BSA_AVK_ADD_TO_NOW_PLAYING_EVT");
        break;

    case BSA_AVK_SET_ABS_VOL_CMD_EVT:
        BT_LOGI("BSA_AVK_SET_ABS_VOL_CMD_EVT volume :: %d", p_data->abs_volume.abs_volume_cmd.volume);
        BT_LOGI("BSA_AVK_SET_ABS_VOL_CMD_EVT opcode :: %d", p_data->abs_volume.abs_volume_cmd.opcode);
        BT_LOGI("BSA_AVK_SET_ABS_VOL_CMD_EVT pdu :: %d", p_data->abs_volume.abs_volume_cmd.pdu);

        if (p_data->abs_volume.abs_volume_cmd.volume > A2DP_MAX_ABS_VOLUME)
            break;
        if ((as->listener) && (as->is_opened == 1)) {
            //msg.abs_volume.opcode = p_data->abs_volume.abs_volume_cmd.opcode;
            msg.abs_volume.volume = p_data->abs_volume.abs_volume_cmd.volume;
            as->listener(as->listener_handle,  BT_AVK_SET_ABS_VOL_CMD_EVT, &msg);
        }
        a2dpk_set_abs_vol_rsp(p_data->abs_volume.abs_volume_cmd.volume, p_data->abs_volume.handle,
                            p_data->abs_volume.label);
        break;
    case BSA_AVK_REG_NOTIFICATION_CMD_EVT:
        BT_LOGD("BSA_AVK_REG_NOTIFICATION_CMD_EVT");
        if (p_data->reg_notif_cmd.reg_notif_cmd.event_id == AVRC_EVT_VOLUME_CHANGE)
        {
            if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
                break;
            connection = &as->connections[as->conn_index];
            if (connection->in_use && connection->is_open && connection->is_rc_open) {
                connection->m_bAbsVolumeSupported = true;
                connection->volChangeLabel = p_data->reg_notif_cmd.label;

                /* Peer requested registration for vol change event. Send response with current system volume. BSA is TG role in AVK */
                a2dpk_reg_notfn_rsp(as->volume,
                                      p_data->reg_notif_cmd.handle,
                                      p_data->reg_notif_cmd.label,
                                      p_data->reg_notif_cmd.reg_notif_cmd.event_id,
                                      BSA_AVK_RSP_INTERIM);
            }
        }
        break;
    default:
        BT_LOGD("Unsupported event %d", event);
        break;
    }
}

static int acc_decode_play(void *caller_handle, void * buffer, int size)
{
    A2dpSink *as = (A2dpSink *)caller_handle;
    int error;

    if(!as)
        return -1;
    //BT_LOGV("size %d", size);
    if (as->pas != NULL && buffer && size > 0 && as->is_started && !as->mute) {
        if((pa_simple_write(as->pas, buffer, size, &error)) < 0) {
            BT_LOGE("pa_simple_write failed %s", pa_strerror(error));
        }
    }
    return 0;
}

static int acc_latm_decode_play_frame(A2dpSink *as, UINT8 *packet, int packetLen)
{
    int aac_len;

    if (!as) return -1;
    if (as->mute) return 0;
    aac_len = packetLen + LATM_SYNC_SIZE;
    as->aac_buff = malloc(aac_len);
    if (!as->aac_buff) {
        return -1;
    }

    addLATMSyncStreamtoPacket(as->aac_buff, packetLen);
    memcpy(as->aac_buff + LATM_SYNC_SIZE, packet, packetLen);
    pthread_mutex_lock(&as->mutex);
    if (as->aac_decoder) {
        aac_decode_frame(as->aac_decoder, as->aac_buff, aac_len, acc_decode_play, as);
    }
    pthread_mutex_unlock(&as->mutex);
    free(as->aac_buff);
    as->aac_buff = NULL;

    return 0;
}

// uipc audio call back function
// param: BT_HDR  pointer on a buffer containing PCM sample
static void a2dpk_uipc_callback(BT_HDR *p_msg)
{
    UINT8 *p_buffer;
    int error;
    BTConnection *connection = NULL;
#ifdef A2DP_DEBUG_TIME
    nsecs_t now1, now2;
#endif

    A2dpSink *as = _a2dpk;

    if (p_msg == NULL)
        return;
    if (!as || (p_msg->len == 0))
        goto exit;
    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        goto exit;

    p_buffer = ((UINT8 *)(p_msg + 1)) + p_msg->offset;
    connection = &as->connections[as->conn_index];
    if(as->uipc_channel == UIPC_CH_ID_BAD) {
        BT_LOGE("Unable to find connection!!!!!!");
        goto exit;
    }
    if (as->is_started) as->is_playing = 1;

#ifdef A2DP_DUMP_DATA
    //BT_LOGD("len %d\n", p_msg->len);
    char value[PROP_VALUE_MAX] = {0};
    property_get("rokid.dump.bt", value, "0");
    if (as->fp && (atoi(value) > 0))
        fwrite(p_buffer, p_msg->len, 1, as->fp);
#endif
#ifdef A2DP_DEBUG_TIME
    now1 = systemTime(CLOCK_MONOTONIC);
#endif

    if (connection->format == BSA_AVK_CODEC_M24) {
        UINT8 *src = p_buffer;
        int len = p_msg->len;
        int frame_length;

        if (as->need_parse_aac_config) {
            if (get_latm_frame_config(&as->latmctx, src, len)) {
                BT_LOGE("failed to get_latm_frame_config!!");
                goto exit;
            }
            if (a2dpk_creat_aac_decoder(as, as->latmctx.m4ac.channels, as->latmctx.m4ac.sample_rate, 16)) {
                BT_LOGE("failed to a2dpk_creat_aac_decoder!!");
                goto exit;
            }
            save_latm_mux_header(src, len);
            as->need_parse_aac_config = 0;
        }
        if (as->aac_remain.need_len > 0) {
            BT_LOGD("need_len %d !!!", as->aac_remain.need_len);
            if (len >= as->aac_remain.need_len) {
                memcpy(as->aac_remain.buffer + as->aac_remain.len, src, as->aac_remain.need_len);
                as->aac_remain.len += as->aac_remain.need_len;
                src += as->aac_remain.need_len;
                len -= as->aac_remain.need_len;
                acc_latm_decode_play_frame(as, as->aac_remain.buffer, as->aac_remain.len);
                as->aac_remain.len = 0;
                as->aac_remain.need_len = 0;
            } else if (len < as->aac_remain.need_len) {//need more data
                memcpy(as->aac_remain.buffer + as->aac_remain.len, src, len);
                as->aac_remain.len += len;
                as->aac_remain.need_len -= len;
                len = 0;
                goto exit;
            }
        }
        if (as->aac_remain.len > 0) {
            BT_LOGD("remain len %d !!", as->aac_remain.len);
            if (as->aac_remain.need_len != 0) {
                BT_LOGE("failed: remain len %d, need_len %d, need len should be 0", as->aac_remain.len, as->aac_remain.need_len);
                as->aac_remain.len = 0;
                as->aac_remain.need_len = 0;
                goto exit;
            }
            if (as->aac_remain.len >= (CHECK_MUX_HEADER_MAX + MUX_SLOT_LENGTH_SIZE)) {
                BT_LOGE("failed:remain len %d, should less 19 !!", as->aac_remain.len);
                as->aac_remain.len = 0;
                as->aac_remain.need_len = 0;
                goto exit;
            }
            memcpy(as->aac_remain.buffer + as->aac_remain.len, src, len);
            as->aac_remain.len += len;
            len = 0;

            while (as->aac_remain.len >= (CHECK_MUX_HEADER_MAX + MUX_SLOT_LENGTH_SIZE)) {
                frame_length = get_aac_latm_length(&as->latmctx, as->aac_remain.buffer, as->aac_remain.len);
                BT_LOGD("aac latm frame %d from remain", frame_length);
                if (frame_length < 0) {
                    BT_LOGE("failed to parse aac latm frame %d", frame_length);
                    as->aac_remain.len = 0;
                    as->aac_remain.need_len = 0;
                    goto exit;
                }
                if (frame_length > as->aac_remain.len) { //need more data
                    as->aac_remain.need_len = frame_length - as->aac_remain.len;
                    goto exit;
                } else {
                    acc_latm_decode_play_frame(as, as->aac_remain.buffer, frame_length);
                    as->aac_remain.len -= frame_length;
                }
                if (as->aac_remain.len > 0) {
                    memcpy(as->aac_remain.buffer, as->aac_remain.buffer + frame_length, as->aac_remain.len);
                }
            }
        }
        if (len > 0) {
            while (len >= (CHECK_MUX_HEADER_MAX + MUX_SLOT_LENGTH_SIZE)) {
                frame_length = get_aac_latm_length(&as->latmctx, src, len);
                BT_LOGV("aac latm frame %d from src", frame_length);
                if (frame_length < 0) {
                    BT_LOGE("failed to parse aac latm length %d", frame_length);
                    as->aac_remain.len = 0;
                    as->aac_remain.need_len = 0;
                    goto exit;
                }
                if (frame_length > len) { //need more data
                    memcpy(as->aac_remain.buffer, src, len);
                    as->aac_remain.len = len;
                    as->aac_remain.need_len = frame_length - len;
                    len = 0;
                    goto exit;
                } else {
                    acc_latm_decode_play_frame(as, src, frame_length);
                    len -= frame_length;
                    src += frame_length;
                }
            }
            if (len > 0) {//need full config header
                memcpy(as->aac_remain.buffer, src, len);
                as->aac_remain.len = len;
                len = 0;
            }
        }
    } else if (connection->format == BSA_AVK_CODEC_PCM) {
        pthread_mutex_lock(&as->mutex);
        if (as->pas != NULL && p_buffer && as->is_started  && !as->mute) {
            if((pa_simple_write(as->pas, p_buffer, p_msg->len, &error)) < 0) {
                BT_LOGE("pa_simple_write failed %s", pa_strerror(error));
            }
        } else {
            BT_LOGD("playback stream NULL or stop");
        }
        pthread_mutex_unlock(&as->mutex);
    }
#ifdef A2DP_DEBUG_TIME
    now2 = systemTime(CLOCK_MONOTONIC);
    now1 = (now2 - now1)/1000000;
    if (now1 >= 5)
        BT_LOGD("sweet test a2dp sink times %ld, end!!!!", now1);
#endif
exit:
    GKI_freebuf(p_msg);
    return;
}


static int a2dpk_register(A2dpSink *as)
{
    tBSA_STATUS status;
    tBSA_AVK_REGISTER bsa_avk_register_param;

    /* register an audio AVK end point */
    BSA_AvkRegisterInit(&bsa_avk_register_param);

    bsa_avk_register_param.media_sup_format.audio.pcm_supported = TRUE;
    bsa_avk_register_param.media_sup_format.audio.sec_supported = TRUE;
#if (defined(BSA_AVK_AAC_SUPPORTED) && (BSA_AVK_AAC_SUPPORTED==TRUE))
    /* Enable AAC support in the app */
    bsa_avk_register_param.media_sup_format.audio.aac_supported = TRUE;
#endif
    strncpy(bsa_avk_register_param.service_name, "BSA Audio Service",
        BSA_AVK_SERVICE_NAME_LEN_MAX - 1);

    bsa_avk_register_param.reg_notifications = reg_notifications;

    status = BSA_AvkRegister(&bsa_avk_register_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to register an AV sink with status %d", status);

        if(BSA_ERROR_SRV_AVK_AV_REGISTERED == status)
        {
            BT_LOGE("AV was registered before AVK, AVK should be registered before AV");
        }
        return status;
    }
#if 0
    /* Save UIPC channel */
    as->uipc_channel = bsa_avk_register_param.uipc_channel;

    /* open the UIPC channel to receive the pcm */
    if (UIPC_Open(bsa_avk_register_param.uipc_channel, a2dpk_uipc_callback) == FALSE)
    {
        BT_LOGE("Unable to open UIPC channel");
        return -1;
    }
#endif
    return 0;
}

static BTConnection *a2dpk_check_all_connect(A2dpSink *as)
{
    int index;

    if(!as) return NULL;

    for (index = 0; index < MAX_CONNECTIONS; index++)
    {
        if (as->connections[index].in_use)
            return &as->connections[index];
    }
    return NULL;
}

int a2dpk_enable(A2dpSink *as)
{
    tBSA_AVK_DISABLE disable_param;
    tBSA_AVK_ENABLE bsa_avk_enable_param;
    tBSA_STATUS status;

    BT_CHECK_HANDLE(as);
    if (as->is_enabled) {
        BT_LOGW("#### a2dp sink is already enabled");
        return 0;
    }

    as->uipc_channel = UIPC_CH_ID_BAD;
    /* set sytem vol at 50% */
    as->volume = (UINT8)((A2DP_MAX_ABS_VOLUME - A2DP_MIN_ABS_VOLUME)>>1);

    /* get hold on the AVK resource, synchronous mode */
    BSA_AvkEnableInit(&bsa_avk_enable_param);

    bsa_avk_enable_param.sec_mask = BSA_AVK_SECURITY;
    bsa_avk_enable_param.features = BSA_AVK_FEATURES;
    bsa_avk_enable_param.rc_features = BSA_RC_FEATURES;
    bsa_avk_enable_param.p_cback = a2dpk_callback;

    status = BSA_AvkEnable(&bsa_avk_enable_param);
    if (status != BSA_SUCCESS) {
        BT_LOGE("Unable to enable an AV sink with status %d", status);
        if(BSA_ERROR_SRV_AVK_AV_REGISTERED == status) {
            BT_LOGE("AV was registered before AVK, AVK should be registered before AV");
        }
        return status;
    }

    status = a2dpk_register(as);
    if (status) goto register_err;

    as->is_enabled = (status == 0);

    return status;
register_err:
    /* disable avk */
    BSA_AvkDisableInit(&disable_param);
    BSA_AvkDisable(&disable_param);

    return status;
}

static int a2dpk_reset_stauts(A2dpSink *as)
{
    as->is_opened = 0;
    as->is_started = 0;
    as->is_playing = 0;
    as->transaction_label = 0;
    memset(as->connections, 0, sizeof(as->connections));
    as->mute = false;
    A2DP_SINK_OPEN_PENDING_LINK_UP = 0;
    return 0;
}

int a2dpk_disable(A2dpSink *as)
{
    tBSA_AVK_DISABLE disable_param;
    tBSA_AVK_DEREGISTER deregister_param;
    int times = 20;

    BT_CHECK_HANDLE(as);
    if (!as->is_enabled) {
        BT_LOGW("###no enabled yet");
        return 0;
    }

    times = 30;
    while (times-- && as->open_pending) {//connecting, wait for connect over
        usleep(100*1000);
    }

    times = 20;
    a2dpk_close(as);
    while (times -- && a2dpk_check_all_connect(as)) {
        usleep(200*1000);
    }

    on_stop(as);
    if (as->uipc_channel != UIPC_CH_ID_BAD) {
        UIPC_Close(as->uipc_channel);
        as->uipc_channel = UIPC_CH_ID_BAD;
    }
    /* deregister avk */
    BSA_AvkDeregisterInit(&deregister_param);
    BSA_AvkDeregister(&deregister_param);

    /* disable avk */
    BSA_AvkDisableInit(&disable_param);
    BSA_AvkDisable(&disable_param);

    a2dpk_reset_stauts(as);
    as->is_enabled = 0;
    return 0;
}

int a2dpk_open(A2dpSink *as, BD_ADDR bd_addr)
{
    tBSA_STATUS status;
    tBSA_AVK_OPEN open_param;

    BT_CHECK_HANDLE(as);
    if (!as->is_enabled) {
        BT_LOGW("###no enabled yet");
        return -1;
    }
    if (get_avail_connection(as) == false) {
        BT_LOGW("Connect full!! If you really want to connect PLS disconnect devices before");
        return -1;
    }

    as->open_pending = TRUE;
    BSA_AvkOpenInit(&open_param);
    bdcpy(open_param.bd_addr, bd_addr);
    bdcpy(as->open_pending_addr, bd_addr);

    open_param.sec_mask = BSA_SEC_NONE;
    status = BSA_AvkOpen(&open_param);
    if (status != BSA_SUCCESS) {
        BT_LOGE("BSA_AvkOpen failed %d", status);
        memset(as->open_pending_addr, 0x0, sizeof(as->open_pending_addr));
        as->open_pending = FALSE;
    }

    return status;
}

int a2dpk_close_device(A2dpSink *as, BD_ADDR bd_addr)
{
    BTConnection *connection;
    tBSA_STATUS status;
    tBSA_AVK_CLOSE bsa_avk_close_param;

    BT_CHECK_HANDLE(as);
    /* Close AVK connection */
    connection = find_connection_by_addr(as->connections, MAX_CONNECTIONS, bd_addr);

    if(connection == NULL) {
        BT_LOGE("Unable to Close AVK connection , invalid BDA");
        return 0;
    }
    BSA_AvkCloseInit(&bsa_avk_close_param);

    bsa_avk_close_param.ccb_handle = connection->ccb_handle;
    bsa_avk_close_param.rc_handle = connection->rc_handle;
    BT_LOGI("close device:%02x%02x%02x%02x%02x%02x ccb_handle %d rc_handle %d",
           bd_addr[0], bd_addr[1], bd_addr[2],
           bd_addr[3], bd_addr[4], bd_addr[5],
           connection->ccb_handle, connection->rc_handle);

    status = BSA_AvkClose(&bsa_avk_close_param);
    if (status != BSA_SUCCESS) {
        BT_LOGE("###Unable to Close AVK connection with status %d", status);
    }
    return status;
}

int a2dpk_close(A2dpSink *as)
{
    int i;
    BD_ADDR bd_addr;
    int ret = 0;

    BT_CHECK_HANDLE(as);
    for (i = 0; i < MAX_CONNECTIONS; ++i) {
        if (as->connections[i].in_use) {
            bdcpy(bd_addr, as->connections[i].bda_connected);
            ret = a2dpk_close_device(as, bd_addr);
        }
    }
    return ret;
}

A2dpSink *a2dpk_create(void *caller)
{
    A2dpSink *as = calloc(1, sizeof(*as));

    as->caller = caller;
    pthread_mutex_init(&as->mutex, NULL);

    _a2dpk = as;
    return as;
}

int a2dpk_destroy(A2dpSink *as)
{
    if (as) {
        //a2dpk_disable(as);
        pthread_mutex_destroy(&as->mutex);
        free(as);
    }
    _a2dpk = NULL;
    return 0;
}

int a2dpk_set_listener(A2dpSink *as, a2dp_sink_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(as);
    as->listener = listener;
    as->listener_handle = listener_handle;
    return 0;
}

static int a2dpk_rc_send_command(A2dpSink *as, UINT8 key_state, UINT8 command, UINT8 rc_handle)
{
    int status;
    tBSA_AVK_REM_CMD bsa_avk_rc_cmd;

    BT_CHECK_HANDLE(as);
    /* Send remote control command */
    status = BSA_AvkRemoteCmdInit(&bsa_avk_rc_cmd);
    bsa_avk_rc_cmd.rc_handle = rc_handle;
    bsa_avk_rc_cmd.key_state = (tBSA_AVK_STATE)key_state;
    bsa_avk_rc_cmd.rc_id = (tBSA_AVK_RC)command;
    bsa_avk_rc_cmd.label = get_command_label(as);

    status = BSA_AvkRemoteCmd(&bsa_avk_rc_cmd);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to Send AV RC Command %d", status);
    }
    return status;
}

static int a2dpk_rc_send_click(A2dpSink *as, UINT8 command)
{
    int status;
    BT_CHECK_HANDLE(as);

    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;
    BTConnection *connection = &as->connections[as->conn_index];
    status = a2dpk_rc_send_command(as, BSA_AV_STATE_PRESS, command, connection->rc_handle);
    GKI_delay(300);
    status = a2dpk_rc_send_command(as, BSA_AV_STATE_RELEASE, command, connection->rc_handle);

    return status;
}

int a2dpk_rc_send_getstatus(A2dpSink *as)
{
    int status;
    tBSA_AVK_GET_PLAY_STATUS bsa_avk_play_status_cmd;
    BTConnection *connection = NULL;

    BT_CHECK_HANDLE(as);
    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;

    connection = &as->connections[as->conn_index];
    /* Send command */
    status = BSA_AvkGetPlayStatusCmdInit(&bsa_avk_play_status_cmd);
    bsa_avk_play_status_cmd.rc_handle = connection->rc_handle;
    bsa_avk_play_status_cmd.label = get_command_label(as); /* Just used to distinguish commands */

    status = BSA_AvkGetPlayStatusCmd(&bsa_avk_play_status_cmd);
    if (status != BSA_SUCCESS) {
        BT_LOGE("Unable to Send app_avk_rc_get_play_status_command %d", status);
    }

    return status;
}

int a2dpk_rc_send_play(A2dpSink *as)
{
    return a2dpk_rc_send_click(as, BSA_AVK_RC_PLAY);
}

int a2dpk_rc_send_stop(A2dpSink *as)
{
    return a2dpk_rc_send_click(as, BSA_AVK_RC_STOP);
}

int a2dpk_rc_send_pause(A2dpSink *as)
{
    return a2dpk_rc_send_click(as, BSA_AVK_RC_PAUSE);
}

int a2dpk_rc_send_forward(A2dpSink *as)
{
    return a2dpk_rc_send_click(as, BSA_AVK_RC_FORWARD);
}

int a2dpk_rc_send_backward(A2dpSink *as)
{
    return a2dpk_rc_send_click(as, BSA_AVK_RC_BACKWARD);
}

int a2dpk_rc_send_cmd(A2dpSink *as, UINT8 command)
{
    BT_LOGI("cmd %d", command);
    return a2dpk_rc_send_click(as, command);
}


static int a2dpk_set_abs_vol_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label)
{
    int status;
    tBSA_AVK_SET_ABS_VOLUME_RSP abs_vol;
    BSA_AvkSetAbsVolumeRspInit(&abs_vol);

    abs_vol.label = label;
    abs_vol.rc_handle = rc_handle;
    abs_vol.volume = volume;
    status = BSA_AvkSetAbsVolumeRsp(&abs_vol);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to send app_avk_set_abs_vol_rsp %d", status);
    }
    return status;
}

static int a2dpk_reg_notfn_rsp(UINT8 volume, UINT8 rc_handle, UINT8 label, UINT8 event, tBSA_AVK_CODE code)
{
    int status;
    tBSA_AVK_REG_NOTIF_RSP reg_notf;
    BSA_AvkRegNotifRspInit(&reg_notf);

    reg_notf.reg_notf.param.volume  = volume;
    reg_notf.reg_notf.event_id      = event;
    reg_notf.rc_handle              = rc_handle;
    reg_notf.label                  = label;
    reg_notf.code                   = code;

    status = BSA_AvkRegNotifRsp(&reg_notf);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to send a2dpk_reg_notfn_rsp %d", status);
    }
    return status;
}

int a2dpk_set_abs_vol(A2dpSink *as, UINT8 volume)
{
    int status = -1;
    int label;
    BT_CHECK_HANDLE(as);

    if (volume > A2DP_MAX_ABS_VOLUME) return -1;
    if (as->conn_index < 0 || as->conn_index >= MAX_CONNECTIONS)
        return -1;
    BTConnection *connection = &as->connections[as->conn_index];
    if (connection->in_use && connection->is_open && connection->is_rc_open && connection->m_bAbsVolumeSupported) {
        label = connection->volChangeLabel;
        BT_LOGI("abs vol %d", volume);
        status = a2dpk_reg_notfn_rsp(volume, connection->rc_handle, label,  AVRC_EVT_VOLUME_CHANGE, BSA_AVK_RSP_CHANGED);
        if (status == BSA_SUCCESS)
            as->volume = volume;
    }

    return status;
}

int a2dpk_get_connected_devices(A2dpSink *as, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;

    BT_CHECK_HANDLE(as);
    if (!a2dpk_get_opened(as)) return 0;
    //pthread_mutex_lock(&as->mutex);
    for (i = 0; i< APP_NUM_ELEMENTS(as->connections); ++i) {
        BTConnection *conn = as->connections + i;

        BT_LOGD("connection %d in use %d is_open %d is_rc_open %d",
                i, conn->in_use, conn->is_open, conn->is_rc_open);
        if (conn->in_use && conn->is_open) {
            if (count < arr_len && dev_array) {
                BTDevice *dev = dev_array + count;
                snprintf(dev->name, sizeof(dev->name), "%s", conn->name);
                bdcpy(dev->bd_addr, conn->bda_connected);
                count ++;
            }
        }
        if (count >= arr_len) break;
    }
    //pthread_mutex_unlock(&as->mutex);
    return count;
}

int a2dpk_get_enabled(A2dpSink *as)
{
    return as ? as->is_enabled : 0;
}

int a2dpk_get_opened(A2dpSink *as)
{
    return as ? as->is_opened : 0;
}

int a2dpk_get_playing(A2dpSink *as)
{
    return as ? as->is_playing : 0;
}

int a2dpk_get_open_pending_addr(A2dpSink *as, BD_ADDR bd_addr)
{
    BT_CHECK_HANDLE(as);
    if (as->open_pending)
        bdcpy(bd_addr, as->open_pending_addr);
    return as->open_pending;
}

int a2dpk_set_mute(A2dpSink *as, bool mute)
{
    BT_CHECK_HANDLE(as);
    as->mute = mute;
    return 0;
}


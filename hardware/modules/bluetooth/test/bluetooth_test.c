
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/time.h>
#include <pthread.h>
#include <cutils/properties.h>

#include <hardware/bluetooth.h>


#define MKDIR_FACROKID_COMMAND "mkdir -p /data/factoryRokid"
#define RESULT_FILE_PATH "/data/factoryRokid/bt_test_result"
#define RM_RESULT_FILE  "rm -rf /data/factoryRokid/bt_test_result"

#define SAVE_RES_TO_FILE(...) \
    do{\
        if (SAVE_FILE) {\
            FILE *pFdRes = fopen(RESULT_FILE_PATH, "a+");\
            if(NULL != pFdRes){\
                char *pCh = (char*)malloc(sizeof(char) * 4*1024);\
                if(NULL != pCh){\
                    sprintf(pCh, __VA_ARGS__);\
                    fputs(pCh, pFdRes);\
                    free(pCh);\
                    pCh = NULL;\
                    fclose(pFdRes);}}\
            else{\
                printf("Fail to write /data/factoryRokid/bt_test_result\n");\
            }\
        }\
        else\
            printf(__VA_ARGS__);\
    }while(0)

#define DELETE_RES_FILE() \
    do { \
        system(RM_RESULT_FILE);\
    }while(0)


static BTDevice devices[BT_DISC_NB_DEVICES];
static int dev_count;
static int _scanning = 0;
static int SAVE_FILE = 0;

static struct bluetooth_module_t *BT_MOD = NULL;
static struct bluetooth_device_t *BT_DEV = NULL;

static inline int bluetooth_device_open (const hw_module_t *module, struct bluetooth_device_t **device) {
    return module->methods->open (module, BLUETOOTH_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}

static int get_bluetooth_device(void)
{
    if (hw_get_module (BLUETOOTH_HARDWARE_MODULE_ID, (const struct hw_module_t **) &BT_MOD) == 0) {
        //open bluetooth
        if (0 != bluetooth_device_open(&BT_MOD->common, &BT_DEV)) {
            printf("failed to hw module: %s  device open\n", BLUETOOTH_HARDWARE_MODULE_ID);
            return -1;
        }
    } else {
        printf("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
        return -1;
    }
    return 0;
}
static int app_get_choice(const char *querystring)
{
    int neg, value, c, base;
    int count = 0;

    base = 10;
    neg = 1;
    printf("%s => ", querystring);
    value = 0;
    do
    {
        c = getchar();

        if ((count == 0) && (c == '\n'))
        {
            return -1;
        }
        count ++;

        if ((c >= '0') && (c <= '9'))
        {
            value = (value * base) + (c - '0');
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            value = (value * base) + (c - 'a' + 10);
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            value = (value * base) + (c - 'A' + 10);
        }
        else if (c == '-')
        {
            neg *= -1;
        }
        else if (c == 'x')
        {
            base = 16;
        }

    } while ((c != EOF) && (c != '\n'));

    return value * neg;
}

static int display_scan_result(struct rk_bluetooth *bt)
{
    int index;

    memset(devices, 0, sizeof(devices));
    dev_count = bt->get_disc_devices(bt, devices, BT_DISC_NB_DEVICES);

    for (index = 0; index < dev_count; ++index) {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
        printf("\tRSSI:%d\n", devices[index].rssi);
    }

    return 0;
}

static void discovery_callback(void *caller, const char *bt_name, BTAddr bt_addr, int is_completed, void *data)
{
    if (is_completed)
        _scanning = 0;
}

static int demo_discovery(struct rk_bluetooth *bt, enum bt_profile_type type)
{
    int ret;
    int times = 20;// 20s

    _scanning = 1;
     ret = bt->start_discovery(bt, type);
    while (_scanning &&  times--) {
        usleep( 1000 * 1000);
    }
    if (_scanning) {
        ret = bt->cancel_discovery(bt);
        _scanning = 0;
    }
    display_scan_result(bt);
    return ret;
}

static int demo_connect(struct rk_bluetooth *bt, enum bt_profile_type type)
{
    int ret = -1;
    int choice, index;

    printf("    0 bonded devices\n");
    printf("    1 scaned devices\n");
    choice = app_get_choice("select devices");
    switch (choice) {
        case 0:
            memset(devices, 0, sizeof(devices));
            dev_count = bt->get_bonded_devices(bt, devices, BT_DISC_NB_DEVICES);
            break;
        case 1:
            memset(devices, 0, sizeof(devices));
            dev_count = bt->get_disc_devices(bt, devices, BT_DISC_NB_DEVICES);
            break;
        default:
            printf("invalid choice %d\n", choice);
            return  ret;
            break;
    }
    for (index = 0; index < dev_count; ++index) {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }

    choice = app_get_choice("select device");
    if (choice < 0 || choice >= dev_count) {
        printf("invalid index %d [0 ... %d]\n", choice, dev_count);
        return ret;
    }
    BTDevice *dev = devices + choice;
    switch (type) {
        case BT_A2DP_SOURCE:
            ret = bt->a2dp.connect(bt, dev->bd_addr);
            break;
        case BT_A2DP_SINK:
            ret = bt->a2dp_sink.connect(bt, dev->bd_addr);
            break;
        case BT_BLE:
            ret = bt->blec.connect(bt, dev->bd_addr);
            break;
        case BT_HS:
            ret = bt->hfp.connect(bt, dev->bd_addr);
            break;
        case BT_HH:
            ret = bt->hh.connect(bt, dev->bd_addr, 0);
            break;
        default:
            printf("not suport con type %d\n", type);
            break;
    }
    return ret;
}


static void demo_a2dp_disconnect(struct rk_bluetooth *bt)
{
    int index, choice;
    printf("connections ===\n");
    memset(devices, 0, sizeof(devices));
    dev_count = bt->a2dp.get_connected_devices(bt, devices, 10);
    for (index = 0; index < dev_count; ++index)
    {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }
    choice = app_get_choice("select disconnect device index");
    if (choice < 0 || choice >= dev_count)
    {
        printf("invalid index %d [0 ... %d]\n", choice, dev_count);
        return;
    }
    BTDevice *dev = devices + choice;
    bt->a2dp.disconnect(bt, dev->bd_addr);
}

static void demo_a2dp_sink_display_connections(struct rk_bluetooth *bt)
{
    int index;
    printf("connections\n");
    memset(devices, 0, sizeof(devices));
    dev_count = bt->a2dp_sink.get_connected_devices(bt, devices, BT_DISC_NB_DEVICES);
    for (index = 0; index < dev_count; ++index)
    {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }
}

static void demo_a2dp_sink_disconnect(struct rk_bluetooth *bt)
{
    int index, choice;
    printf("connections ===\n");
    memset(devices, 0, sizeof(devices));
    dev_count = bt->a2dp_sink.get_connected_devices(bt, devices, BT_DISC_NB_DEVICES);
    for (index = 0; index < dev_count; ++index)
    {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }
    choice = app_get_choice("select disconnect device index");
    if (choice < 0 || choice >= dev_count)
    {
        printf("invalid index %d [0 ... %d]\n", choice, dev_count);
        return;
    }
    BTDevice *dev = devices + choice;
    bt->a2dp_sink.disconnect(bt, dev->bd_addr);
}


static void demo_ble_client_disconnect(struct rk_bluetooth *bt)
{
    int index, choice;
    printf("connections ===\n");
    memset(devices, 0, sizeof(devices));
    dev_count = bt->blec.get_connected_devices(bt, devices, BT_DISC_NB_DEVICES);
    for (index = 0; index < dev_count; ++index)
    {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }
    choice = app_get_choice("select disconnect device index");
    if (choice < 0 || choice >= dev_count)
    {
        printf("invalid index %d [0 ... %d]\n", choice, dev_count);
        return;
    }
    BTDevice *dev = devices + choice;
    bt->blec.disconnect(bt, dev->bd_addr);
}

static void demo_hh_disconnect(struct rk_bluetooth *bt)
{
    int index, choice;
    printf("connections ===\n");
    memset(devices, 0, sizeof(devices));
    dev_count = bt->hh.get_connected_devices(bt, devices, BT_DISC_NB_DEVICES);
    for (index = 0; index < dev_count; ++index)
    {
        printf("dev:%d\n", index);
        printf("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        printf("\tname:%s\n", devices[index].name);
    }
    choice = app_get_choice("select disconnect device index");
    if (choice < 0 || choice >= dev_count)
    {
        printf("invalid index %d [0 ... %d]\n", choice, dev_count);
        return;
    }
    BTDevice *dev = devices + choice;
    bt->hh.disconnect(bt, dev->bd_addr);
}

void manage_listener(void *caller, BT_MGR_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_MGT_MSG  *msg = data;

    switch (event) {
        case BT_EVENT_MGR_CONNECT:
            if (msg)
                printf("BT_EVENT_MGR_CONNECT : enable= %d\n", msg->connect.enable);
            break;
        case BT_EVENT_MGR_DISCONNECT:
            if (msg)
                printf("BT_EVENT_MGR_DISCONNECT : reason= %d\n", msg->disconnect.reason);
            printf("bt depend service abnormal  exit!!!!!, you must reboot bt service!!");
            exit(2);
            break;
        default:
            break;
    }
    return;
}

void ble_listener (void *caller, BT_BLE_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_BLE_MSG *msg = data;

    switch (event) {
        case BT_BLE_SE_WRITE_EVT:
            if (msg) {
                char buf[255] = {0};
                int len = msg->ser_write.len > 255 ? 255 : msg->ser_write.len;
                memcpy(buf, msg->ser_write.value + msg->ser_write.offset , len);
                buf[254] = '\0';
                printf("BT_BLE_SE_WRITE_EVT : uuid 0x%x, status %d, len: %d, buf: %s\n",
                        msg->ser_write.uuid, msg->ser_write.status, msg->ser_write.len, buf);
            }
            break;
        case BT_BLE_SE_OPEN_EVT:
            if (msg)
                printf("BT_BLE_SE_OPEN_EVT : reason= %d, conn_id %d\n", msg->ser_open.reason, msg->ser_open.conn_id);
            break;
        case BT_BLE_SE_CLOSE_EVT:
            if (msg)
                printf("BT_BLE_SE_CLOSE_EVT : reason= %d, conn_id %d\n", msg->ser_close.reason, msg->ser_close.conn_id);
            break;
        default:
            break;
    }
    return;
}

static void a2dp_listener (void *caller, BT_A2DP_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_A2DP_MSG *msg = data;

    switch (event) {
        case BT_A2DP_OPEN_EVT:
            if (msg) {
                printf("BT_A2DP_OPEN_EVT : status= %d, name: %s , addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                    msg->open.status, msg->open.dev.name,
                    msg->open.dev.bd_addr[0], msg->open.dev.bd_addr[1],
                    msg->open.dev.bd_addr[2], msg->open.dev.bd_addr[3],
                    msg->open.dev.bd_addr[4], msg->open.dev.bd_addr[5]);
            }
            break;
        case BT_A2DP_CLOSE_EVT:
            if (msg)
                printf("BT_A2DP_CLOSE_EVT : status= %d\n", msg->close.status);
            break;
        case BT_A2DP_RC_OPEN_EVT:
            if (msg)
                printf("BT_A2DP_RC_OPEN_EVT : status= %d\n", msg->rc_open.status);
            break;
        case BT_A2DP_RC_CLOSE_EVT:
            if (msg)
                printf("BT_A2DP_RC_CLOSE_EVT : status= %d\n", msg->rc_close.status);
            break;
        case BT_A2DP_START_EVT:
            if (msg)
                printf("BT_A2DP_START_EVT : status= %d\n", msg->start.status);
            break;
        case BT_A2DP_STOP_EVT:
            if (msg)
                printf("BT_A2DP_STOP_EVT : pause= %d\n", msg->stop.pause);
            break;
        case BT_A2DP_REMOTE_CMD_EVT:
            if (msg) {
                printf("BT_A2DP_REMOTE_CMD_EVT : rc_cmd= 0x%x\n", msg->remote_cmd.rc_id);
                switch(msg->remote_cmd.rc_id) {
                    case BT_AVRC_PLAY:
                        printf("Play key\n");
                        break;
                    case BT_AVRC_STOP:
                        printf("Stop key\n");
                        break;
                    case BT_AVRC_PAUSE:
                        printf("Pause key\n");
                        break;
                    case BT_AVRC_FORWARD:
                        printf("Forward key\n");
                        break;
                    case BT_AVRC_BACKWARD:
                        printf("Backward key\n");
                        break;
                    default:
                        break;
                }
            }
            break;
        case BT_A2DP_REMOTE_RSP_EVT:
            if (msg)
                printf("BT_A2DP_REMOTE_RSP_EVT : rc_cmd= 0x%x\n", msg->remote_rsp.rc_id);
            break;
        default:
            break;
    }
    return;
}

void a2dp_sink_listener (void *caller, BT_A2DP_SINK_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_AVK_MSG *msg = data;

    switch (event) {
        case BT_AVK_OPEN_EVT:
            if (msg) {
                printf("BT_AVK_OPEN_EVT : status= %d, idx %d, name: %s , addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                    msg->sig_chnl_open.status, msg->sig_chnl_open.idx, msg->sig_chnl_open.name,
                    msg->sig_chnl_open.bd_addr[0], msg->sig_chnl_open.bd_addr[1],
                    msg->sig_chnl_open.bd_addr[2], msg->sig_chnl_open.bd_addr[3],
                    msg->sig_chnl_open.bd_addr[4], msg->sig_chnl_open.bd_addr[5]);
            }
            break;
        case BT_AVK_CLOSE_EVT:
            if (msg)
                printf("BT_AVK_CLOSE_EVT : status= %d, idx %d\n", msg->sig_chnl_close.status, msg->sig_chnl_close.idx);
            break;
        case BT_AVK_STR_OPEN_EVT:
            if (msg) {
                printf("BT_AVK_STR_OPEN_EVT : status= %d, idx %d, name: %s , addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                    msg->stream_chnl_open.status, msg->stream_chnl_open.idx, msg->stream_chnl_open.name,
                    msg->stream_chnl_open.bd_addr[0], msg->stream_chnl_open.bd_addr[1],
                    msg->stream_chnl_open.bd_addr[2], msg->stream_chnl_open.bd_addr[3],
                    msg->stream_chnl_open.bd_addr[4], msg->stream_chnl_open.bd_addr[5]);
            }
            break;
        case BT_AVK_STR_CLOSE_EVT:
            if (msg)
                printf("BT_AVK_STR_CLOSE_EVT : status= %d, idx %d\n", msg->stream_chnl_close.status, msg->stream_chnl_close.idx);
            break;
        case BT_AVK_START_EVT:
            if (msg)
                printf("BT_AVK_START_EVT : status= %d, idx %d\n", msg->start_streaming.status, msg->start_streaming.idx);
            break;
        case BT_AVK_STOP_EVT:
            if (msg)
                printf("BT_AVK_STOP_EVT : status= %d, idx %d, suspended=%d\n", msg->stop_streaming.status, msg->stop_streaming.idx, msg->stop_streaming.suspended);
            break;
        case BT_AVK_RC_OPEN_EVT:
            if (msg)
                printf("BT_AVK_RC_OPEN_EVT : status= %d, idx %d\n", msg->rc_open.status, msg->rc_open.idx);
            break;
        case BT_AVK_RC_CLOSE_EVT:
            if (msg)
                printf("BT_AVK_RC_CLOSE_EVT : status= %d, idx %d\n", msg->rc_close.status, msg->rc_close.idx);
            break;
        case BT_AVK_GET_PLAY_STATUS_EVT:
            if (msg)
                printf("BT_AVK_GET_PLAY_STATUS_EVT : play_status= %d\n", msg->get_play_status.play_status);
            break;
        case BT_AVK_SET_ABS_VOL_CMD_EVT:
            if (msg)
                printf("BT_AVK_SET_ABS_VOL_CMD_EVT : volume= %d\n", msg->abs_volume.volume);
            break;
        default:
            break;
    }
    return;
}

void hfp_listener (void *caller, BT_HS_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_HFP_MSG *msg = data;

    switch (event) {
        case BT_HS_CONN_EVT:
            if (msg) {
                printf("BT_HS_CONN_EVT : status= %d, idx %d, name: %s, addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                    msg->conn.status, msg->conn.idx, msg->conn.name,
                    msg->conn.bd_addr[0], msg->conn.bd_addr[1],
                    msg->conn.bd_addr[2], msg->conn.bd_addr[3],
                    msg->conn.bd_addr[4], msg->conn.bd_addr[5]);
            }
            break;
        case BT_HS_CLOSE_EVT:
            if (msg)
                printf("BT_HS_CLOSE_EVT : status= %d, idx %d\n", msg->close.status, msg->close.idx);
            break;
        case BT_HS_AUDIO_OPEN_EVT:
            printf("BT_HS_AUDIO_OPEN_EVT\n");
            break;
        case BT_HS_AUDIO_CLOSE_EVT:
            printf("BT_HS_AUDIO_CLOSE_EVT\n");
            break;
        case BT_HS_FEED_MIC_EVT:
            if (msg) {
                //printf("BT_HS_FEED_MIC_EVT : need len=%d, bits=%d, channel =%d, rate=%d\n",
                //    msg->feed_mic.need_len, msg->feed_mic.bits, msg->feed_mic.channel, msg->feed_mic.rate);
                /*todo*/
            }
            break;
        case BT_HS_CIND_EVT:
            printf("BT_HS_CIND_EVT\n");
            if (msg) {
                printf("call_ind_id %d, call_setup_ind_id %d,service_ind_id %d, battery_ind_id %d, callheld_ind_id %d, signal_strength_ind_id %d, roam_ind_id %d\n",
                    msg->cind.status.call_ind_id, msg->cind.status.call_setup_ind_id, msg->cind.status.service_ind_id, msg->cind.status.battery_ind_id,
                    msg->cind.status.callheld_ind_id, msg->cind.status.signal_strength_ind_id, msg->cind.status.roam_ind_id);
                printf("curr_call_ind %d, curr_call_setup_ind %d,curr_service_ind %d, curr_battery_ind %d, curr_callheld_ind %d, curr_signal_strength_ind %d, curr_roam_ind %d\n",
                    msg->cind.status.curr_call_ind, msg->cind.status.curr_call_setup_ind, msg->cind.status.curr_service_ind, msg->cind.status.curr_battery_ind,
                    msg->cind.status.curr_callheld_ind, msg->cind.status.curr_signal_strength_ind, msg->cind.status.curr_roam_ind);
            }
            break;
        case BT_HS_CIEV_EVT:
            if (msg)
                printf("BT_HS_CIEV_EVT : CIEV= %d\n", msg->ciev.status);
            break;
        case BT_HS_RING_EVT:
            printf("BT_HS_RING_EVT\n");
            break;
        case BT_HS_CHLD_EVT:
            printf("BT_HS_CHLD_EVT\n");
            break;
        case BT_HS_OK_EVT:
            printf("BT_HS_OK_EVT\n");
            break;
        case BT_HS_ERROR_EVT:
            printf("BT_HS_ERROR_EVT\n");
            break;
        default:
            break;
    }
    return;
}

static void ble_server_operation(struct rk_bluetooth *bt)
{
    int choice = 0;
    char buf[36] = {0};
    int len = 0;

    do {
        printf("Bluetooth ble server CMD menu:\n");
        printf("    0 write\n");
        printf("    1 discovery ble devices\n");
        printf("    99 disabel ble and exit ble\n");

        choice = app_get_choice("Select choice");

        switch(choice) {
        case 0:
            memset(buf, 0, sizeof(buf));
            len = strlen("rokid test");
            memcpy(buf, "rokid test", len);

            bt->ble.send_buf(bt, 0x2a06, buf, len);

            memset(buf, 0, sizeof(buf));
            len = strlen("test rokid");
            memcpy(buf, "test rokid", len);

            bt->ble.send_buf(bt, 0x2a07, buf, len);
            break;
        case 1:
            demo_discovery(bt, BT_BLE);
            break;
        case 99:
            bt->ble.disable(bt);
            break;
        default:
            printf("Unsupported choice\n");
            break;
        }


    } while(choice != 99);
}

#define ATVV_SERVICE_UUID		"AB5E0001-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_TX_CHAR_UUID		"AB5E0002-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_RX_CHAR_UUID		"AB5E0003-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_CTL_CHAR_UUID		"AB5E0004-5A21-4F05-BC7D-AF01F617B664"

#define ATVV_GET_CAPS (0x0A)
#define ATVV_MIC_OPEN (0x0C)
#define ATVV_MIC_CLOSE (0x0D)

#define ATVV_CTL_AUDIO_STOP  (0x00)
#define ATVV_CTL_AUDIO_START  (0x04)
#define ATVV_CTL_START_SEARCH  (0x08)
#define ATVV_CTL_AUDIO_SYNC  (0x0A)
#define ATVV_CTL_GET_CAPS_RESP  (0x0B)
#define ATVV_CTL_MIC_OPEN_ERROR  (0x0C)

static void ble_client_operation(struct rk_bluetooth *bt)
{
    int choice;

    do
    {
        printf("Bluetooth ble_client CMD menu:\n");
        printf("    0 discovery\n");
        printf("    1 connect\n");
        printf("    2 disconnect\n");
        printf("    3 search service\n");
        printf("    4 register_notification\n");
        printf("    5 write\n");
        printf("    6 read\n");
        printf("    7 disable\n");

        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
            case 0:
                demo_discovery(bt, BT_BLE);
                break;
            case 1:
                demo_connect(bt, BT_BLE);
                break;
            case 2:
                demo_ble_client_disconnect(bt);
                break;
            case 3: {
                bt->blec.search_service(bt, ATVV_SERVICE_UUID);
                break;
            }
            case 4: {
                bt->blec.register_notification(bt, ATVV_SERVICE_UUID, ATVV_RX_CHAR_UUID);
                bt->blec.register_notification(bt, ATVV_SERVICE_UUID, ATVV_CTL_CHAR_UUID);
                break;
            }
            case 5: {
                char value[255];
                int len = 5;
                memset(value, 0, sizeof(value));
                value[0] = ATVV_GET_CAPS;
                value[1] = 0;
                value[2] = 04;
                value[3] = 00;
                value[4] = 02;

                bt->blec.write(bt, ATVV_SERVICE_UUID, ATVV_TX_CHAR_UUID, value, len);
                break;
            }
            case 6: {
                bt->blec.read(bt, ATVV_SERVICE_UUID, ATVV_CTL_CHAR_UUID);
                break;
            }
            case 7:
                bt->blec.disable(bt);
                break;
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);
}

static void a2dp_operate(struct rk_bluetooth *bt)
{
    int choice;

    do
    {
        printf("Bluetooth a2dp CMD menu:\n");
        printf("    1 discovery\n");
        printf("    2 connect\n");
        printf("    3 disconnect\n");
        printf("    4 start\n");
        printf("    5 volume +\n");
        printf("    6 volume -\n");

        printf("    7 disable\n");


        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
                break;
            case 1:
                demo_discovery(bt, BT_A2DP_SINK);//we need to connect sink devices
                break;
            case 2:
                demo_connect(bt, BT_A2DP_SOURCE);
                break;
            case 3:
                demo_a2dp_disconnect(bt);
                break;
            case 4:
                bt->a2dp.start(bt, BT_A2DP_CODEC_TYPE_PCM);
                break;
            case 5:
                bt->a2dp.send_avrc_cmd(bt, BT_AVRC_VOL_UP);
                break;
            case 6:
                bt->a2dp.send_avrc_cmd(bt, BT_AVRC_VOL_DOWN);
                break;
            case 7:
                 bt->a2dp.disable(bt);
                break;
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);

}

static void a2dp_sink_operation(struct rk_bluetooth *bt)
{
    int choice;
    int ret = 0;

    do
    {
        printf("Bluetooth a2dp sink CMD menu:\n");
        printf("    0 play\n");
        printf("    1 stop\n");
        printf("    2 pause\n");
        printf("    3 forward\n");
        printf("    4 backward\n");

        printf("    5 discovery\n");
        printf("    6 connect\n");
        printf("    7 display connections\n");
        printf("    8 disconnect\n");
        printf("    9 disable\n");
        printf("    10 get element atrributes\n");
        printf("    11 send get playstatus\n");
        printf("    12 get playing\n");
        printf("    13 set mute\n");
        printf("    14 set unmute\n");
        printf("    15 set abs volume\n");


        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
            case 0:
                bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_PLAY);
                break;
            case 1:
                bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_STOP);
                break;
            case 2:
                bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_PAUSE);
                break;
            case 3:
                bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_FORWARD);
                break;
            case 4:
                bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_BACKWARD);
                break;
            case 5:
                demo_discovery(bt, BT_A2DP_SOURCE);
                break;
            case 6:
                demo_connect(bt, BT_A2DP_SINK);
                break;
            case 7:
                demo_a2dp_sink_display_connections(bt);
                break;
            case 8:
                demo_a2dp_sink_disconnect(bt);
                break;
            case 9:
                bt->a2dp_sink.disable(bt);
                break;
            case 10:
                bt->a2dp_sink.get_element_attrs(bt);
                break;
            case 11:
                ret = bt->a2dp_sink.send_get_playstatus(bt);
                break;
            case 12:
                ret = bt->a2dp_sink.get_playing(bt);
                printf("get current playing %d\n", ret);
                break;
            case 13:
                ret = bt->a2dp_sink.set_mute(bt, true);
                break;
            case 14:
                ret = bt->a2dp_sink.set_mute(bt, false);
                break;
            case 15:
            {
                int vol;
                vol = app_get_choice("input volume(0-127)");
                ret = bt->a2dp_sink.set_abs_vol(bt, vol);
                break;
            }
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);
}

static void hs_operation(struct rk_bluetooth *bt)
{
    int choice;

    do
    {
        printf("Bluetooth hs_operation CMD menu:\n");
        printf("    0 discovery\n");
        printf("    1 connect\n");
        printf("    2 close\n");
        printf("    3 disable\n");
        printf("    4 answercall\n");
        printf("    5 hangup call\n");
        printf("    6 dial num\n");
        printf("    7 last num dial\n");
        printf("    8 send dtmf\n");
        printf("    9 set volume\n");
        printf("    10 mute unmute mic\n");

        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
            case 0:
                demo_discovery(bt, BT_HS);
                break;
            case 1:
                demo_connect(bt, BT_HS);
                break;
            case 2:
                bt->hfp.disconnect(bt);
                break;
            case 3:
                bt->hfp.disable(bt);
                break;
            case 4:
                bt->hfp.answercall(bt);
                break;
            case 5:
                bt->hfp.hangup(bt);
                break;
            case 6:
            {
                int  i = 0;
                char c = 0;
                char num[255] = {0};
                printf("input dial numbers\n");
                do
                {
                    c = getchar();
                    num[i] = c;
                    i ++;
                } while ((c != EOF) && (c != '\n') && i < 255);
                printf("your input: %s\n", num);
                bt->hfp.dial_num(bt, num);
            }
                break;
            case 7:
                bt->hfp.dial_last_num(bt);
                break;
            case 8:
            {
                char dtmf = 0;
                printf("input dtmf char\n");
                scanf("%c", &dtmf);
                printf("your dtmf char: %c (%d)\n", dtmf, dtmf);
                bt->hfp.send_dtmf(bt, dtmf);
            }
                break;
            case 9:
            {
                int volume;
                int type;
                printf("    %d for speaker set volume \n", BT_HS_VOL_SPK);
                printf("    %d for mic set volume \n", BT_HS_VOL_MIC);
                type = app_get_choice("select types");
                if (type == BT_HS_VOL_SPK) {
                    printf(" input volume(0 -- 14) \n");
                    volume = app_get_choice("select volume");
                    if (volume <= 14 && volume >=0)
                        bt->hfp.set_volume(bt, BT_HS_VOL_SPK, volume);
                } else if (type == BT_HS_VOL_MIC) {
                    printf(" input volume(0 -- 14) \n");
                    volume = app_get_choice("select volume");
                    if (volume <= 14 && volume >=0)
                        bt->hfp.set_volume(bt, BT_HS_VOL_MIC, volume);
                }
            }
                break;
            case 10:
            {
                int mute;
                printf(" 1  mute microphone \n");
                printf(" 0  unmute microphone \n");
                mute = app_get_choice("select devices");
                mute = !!mute;
                bt->hfp.mute_mic(bt, mute);
            }
                break;
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);
}

static void hh_operation(struct rk_bluetooth *bt)
{
    int choice;

    do
    {
        printf("Bluetooth hh_operation CMD menu:\n");
        printf("    0 discovery\n");
        printf("    1 connect\n");
        printf("    2 disconnect\n");
        printf("    3 disable\n");

        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice)
        {
            case 0: {
                int type;
                type = app_get_choice("Select type");
                demo_discovery(bt, type);
            }
                break;
            case 1:
                demo_connect(bt, BT_HH);
                break;
            case 2:
                demo_hh_disconnect(bt);
                break;
            case 3:
                bt->hh.disable(bt);
                break;
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);
}

static void memory_leak_test_operation(void)
{
    int choice;
    int ret = 0;
    int times = 0;
    struct rk_bluetooth *bt = NULL;

    ret = BT_DEV->creat(&bt);
    if (ret || !bt) {
        printf("failed to creat bt\n");
       goto exit;
    }

    bt->set_discovery_listener(bt, discovery_callback, bt);
    bt->set_manage_listener(bt, manage_listener, bt);
    ret = bt->open(bt, NULL);
    if (ret) {
        printf("failed to open bt\n");
       goto exit;
    }

    do {
        printf("Bluetooth memory leak CMD menu:\n");
        printf("    0 test all\n");
        printf("    1 test a2dp sink\n");
        printf("    2 test ble\n");
        printf("    3 test a2dp source\n");
        printf("    4 test hfp\n");
        printf("    5 test creat/destroy\n");

        printf("    99 break\n");

        choice = app_get_choice("Select source");

        switch(choice) {
            case 0:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    BT_DEV->destroy(bt);
                    bt = NULL;
                    usleep(100*1000);
                    ret = BT_DEV->creat(&bt);
                    if (ret || !bt) {
                        printf("failed to creat bt\n");
                        goto exit;
                    }

                    bt->set_discovery_listener(bt, discovery_callback, bt);
                    bt->set_manage_listener(bt, manage_listener, bt);
                    ret = bt->open(bt, NULL);
                    if (ret) {
                        printf("failed to open bt\n");
                        goto exit;
                    }
                    usleep(100*1000);

                    bt->ble.set_listener(bt, ble_listener, bt);
                    if (0 != bt->ble.enable(bt)) {
                        printf("enable ble failed\n");
                        goto exit;
                    }
                    bt->ble.set_ble_visibility(bt, true, true);
                    usleep(500*1000);
                    bt->ble.disable(bt);

                    bt->a2dp_sink.set_listener(bt, a2dp_sink_listener, bt);
                    if (bt->a2dp_sink.enable(bt)) {
                        printf("enable a2dp sink failed!\n");
                        goto exit;
                    }
                    bt->set_visibility(bt, true, true);
                    usleep(500*1000);
                    bt->a2dp_sink.disable(bt);
                }
                break;
            case 1:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    if (bt->a2dp_sink.enable(bt)) {
                        printf("enable a2dp sink failed!\n");
                        goto exit;
                    }
                    usleep(500*1000);
                    bt->a2dp_sink.disable(bt);
                    usleep(500*1000);
                }
                break;
            case 2:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    if (0 != bt->ble.enable(bt)) {
                        printf("enable ble failed\n");
                        goto exit;
                    }
                    usleep(500*1000);
                    bt->ble.disable(bt);
                    usleep(500*1000);
                }
                break;
            case 3:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    if (bt->a2dp.enable(bt)) {
                        printf("enable a2dp sink failed!\n");
                        goto exit;
                    }
                    usleep(500*1000);
                    bt->a2dp.disable(bt);
                    usleep(500*1000);
                }
                break;
            case 4:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    bt->hfp.set_listener(bt, hfp_listener, bt);
                    if (bt->hfp.enable(bt)) {
                        printf("enable hfp failed!\n");
                        goto exit;
                    }
                    usleep(100*1000);
                    bt->hfp.disable(bt);
                    usleep(100*1000);
                }
                break;
            case 5:
                times = app_get_choice("Select test times");
                while (times-- > 0) {
                    BT_DEV->destroy(bt);
                    bt = NULL;
                    usleep(200*1000);
                    ret = BT_DEV->creat(&bt);
                    if (ret || !bt) {
                        printf("failed to creat bt\n");
                        goto exit;
                    }

                    bt->set_discovery_listener(bt, discovery_callback, bt);
                    bt->set_manage_listener(bt, manage_listener, bt);
                    ret = bt->open(bt, NULL);
                    if (ret) {
                        printf("failed to open bt\n");
                        goto exit;
                    }
                    usleep(500*1000);
                }
                break;
            default:
                printf("Unsupported choice\n");
            break;
        }
    } while (choice != 99);

exit:
    if (bt) {
        BT_DEV->destroy(bt);
        bt = NULL;
    }
    return;
}

int main_loop(void)
{
    int ret;
    int choice;
    char name[128] = {0};
    BTAddr bd_addr = {0};
    struct rk_bluetooth *bt = NULL;

    ret = get_bluetooth_device();
    if (ret) {
        printf("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
        return -1;
    }
    ret = BT_DEV->creat(&bt);
    if (ret || !bt) {
        printf("failed to creat bt\n");
        goto exit;
    }

    bt->set_discovery_listener(bt, discovery_callback, bt);
    bt->set_manage_listener(bt, manage_listener, bt);
    ret = bt->open(bt, NULL);
    if (ret) {
        printf("failed to open bt\n");
        goto exit;
    }
    do {
        printf("Bluetooth test CMD menu:\n");
        printf("    1 a2dp sink\n");
        printf("    2 ble service\n");
        printf("    3 a2dp\n");
        printf("    4 a2dp sink command\n");
        printf("    5 discovery command\n");
        printf("    6 hpf hs\n");
        printf("    7 hpf hs  command\n");
        printf("    8 test destroy/creat memory leak\n");
        printf("    9 open\n");
        printf("    10 close\n");
        printf("    11 hid host\n");
        printf("    12 ble client\n");

        printf("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice)
        {
            case 1:
                bt->a2dp_sink.disable(bt);
                usleep(100*1000);
                bt->get_module_addr(bt, bd_addr);
                snprintf(name, sizeof(name), "rokidsink_%02x:%02x:%02x:%02x:%02x:%02x",
                            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
                bt->set_bt_name(bt, name);
                bt->a2dp_sink.set_listener(bt, a2dp_sink_listener, bt);
                if (bt->a2dp_sink.enable(bt)) {
                    printf("enable a2dp sink failed!\n");
                    break;
                }
                bt->set_visibility(bt, true, true);
                a2dp_sink_operation(bt);
                break;
            case 2:
                bt->ble.disable(bt);
                usleep(100*1000);
                bt->ble.set_listener(bt, ble_listener, bt);
                bt->set_bt_name(bt, "Rokid-Me-666666");
                if (0 != bt->ble.enable(bt)) {
                    printf("enable ble failed\n");
                    goto exit;
                }
                bt->ble.set_ble_visibility(bt, true, true);
                ble_server_operation(bt);
                break;
            case 3:
                bt->a2dp.disable(bt);
                usleep(100*1000);
                bt->get_module_addr(bt, bd_addr);
                snprintf(name, sizeof(name), "rokidsource_%02x:%02x:%02x:%02x:%02x:%02x",
                            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
                bt->set_bt_name(bt, name);
                bt->a2dp.set_listener(bt, a2dp_listener, bt);
                if (bt->a2dp.enable(bt)) {
                    printf("enable a2dp failed!\n");
                    break;
                }
                bt->set_visibility(bt, true, true);
                a2dp_operate(bt);
                break;
            case 4:
                a2dp_sink_operation(bt);
                break;
            case 5:
                demo_discovery(bt, BT_BR_EDR);
                break;
            case 6:
                bt->hfp.disable(bt);
                usleep(100*1000);
                bt->get_module_addr(bt, bd_addr);
                snprintf(name, sizeof(name), "rokidsink_%02x:%02x:%02x:%02x:%02x:%02x",
                            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
                bt->set_bt_name(bt, name);
                bt->hfp.set_listener(bt, hfp_listener, bt);
                if (bt->hfp.enable(bt)) {
                    printf("enable hfp failed!\n");
                    break;
                }
                bt->set_visibility(bt, true, true);
                hs_operation(bt);
                break;
            case 7:
                hs_operation(bt);
                break;
            case 8:
                /*before test  we should destroy old bt*/
                BT_DEV->destroy(bt);
                bt = NULL;
                usleep(100*1000);
                memory_leak_test_operation();
                /*after test creat bt for continue use other test option*/
                usleep(100*1000);
                ret = BT_DEV->creat(&bt);
                if (ret || !bt) {
                    printf("failed to creat bt\n");
                    goto exit;
                }

                bt->set_discovery_listener(bt, discovery_callback, bt);
                bt->set_manage_listener(bt, manage_listener, bt);
                ret = bt->open(bt, NULL);
                if (ret) {
                    printf("failed to open bt\n");
                    goto exit;
                }
                break;
            case 9:
                ret = bt->open(bt, NULL);
                if (ret) {
                    printf("failed to open bt\n");
                    goto exit;
                }
                break;
            case 10:
                bt->close(bt);
                break;
            case 11:
                if (bt->hh.enable(bt)) {
                    printf("enable hid host failed!\n");
                    break;
                }
                hh_operation(bt);
                break;
            case 12:
                if (bt->blec.enable(bt)) {
                    printf("enable blec client failed!\n");
                    break;
                }
                ble_client_operation(bt);
                break;
            default:
                break;
        }
    } while (choice != 99);

exit:
    if (BT_DEV) {
        if (bt) {
            bt->close(bt);
            BT_DEV->destroy(bt);
            bt = NULL;
        }
        BT_DEV->common.close((struct hw_device_t *)BT_DEV);
        BT_DEV = NULL;
    }

    return ret;
}

static int cmd_test_scan(void)
{
    int ret;
    int index;
    struct rk_bluetooth *bt = NULL;

    ret = get_bluetooth_device();
    if (ret) {
        printf("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
        return -1;
    }
    ret = BT_DEV->creat(&bt);
    if (ret || !bt) {
        printf("failed to creat bt\n");
        goto exit;
    }

    bt->set_discovery_listener(bt, discovery_callback, bt);
    bt->set_manage_listener(bt, manage_listener, bt);
    ret = bt->open(bt, NULL);
    if (ret) {
        printf("failed to open bt\n");
        goto exit;
    }
    usleep(100*1000);
    ret = demo_discovery(bt, BT_BR_EDR);
exit:
    if (BT_DEV) {
        if (bt) {
            BT_DEV->destroy(bt);
            bt = NULL;
        }
        BT_DEV->common.close((struct hw_device_t *)BT_DEV);
        BT_DEV = NULL;
    }

    SAVE_RES_TO_FILE("bt scan result: %d\n", ret);
    for (index = 0; index < dev_count; ++index) {
        SAVE_RES_TO_FILE("dev:%d\n", index);
        SAVE_RES_TO_FILE("\tbdaddr:%02X:%02X:%02X:%02X:%02X:%02X\n",
                devices[index].bd_addr[0],
                devices[index].bd_addr[1],
                devices[index].bd_addr[2],
                devices[index].bd_addr[3],
                devices[index].bd_addr[4],
                devices[index].bd_addr[5]);
        SAVE_RES_TO_FILE("\tname:%s\n", devices[index].name);
        SAVE_RES_TO_FILE("\tRSSI:%d\n", devices[index].rssi);
    }
    return ret;
}

static int cmd_test_get_bt_addr(void)
{
    int ret;
    BTAddr bd_addr = {0};
    struct rk_bluetooth *bt = NULL;

    ret = get_bluetooth_device();
    if (ret) {
        printf("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
        return -1;
    }
    ret = BT_DEV->creat(&bt);
    if (ret || !bt) {
        printf("failed to creat bt\n");
        goto exit;
    }

    bt->set_discovery_listener(bt, discovery_callback, bt);
    bt->set_manage_listener(bt, manage_listener, bt);
    ret = bt->open(bt, NULL);
    if (ret) {
        printf("failed to open bt\n");
        goto exit;
    }
    usleep(100 * 1000);
    ret = bt->get_module_addr(bt, bd_addr);
exit:
    if (BT_DEV) {
        if (bt) {
            BT_DEV->destroy(bt);
            bt = NULL;
        }
        BT_DEV->common.close((struct hw_device_t *)BT_DEV);
        BT_DEV = NULL;
    }
    SAVE_RES_TO_FILE("bt addr result: %d\n", ret);
    if (ret) {
        SAVE_RES_TO_FILE("failed to get BT MAC addr\n");
    } else {
        SAVE_RES_TO_FILE("BT MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",
                bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        SAVE_RES_TO_FILE("BT NAME:rokidftm_%02x:%02x:%02x:%02x:%02x:%02x\n",
                bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
    }
    return ret;
}

static int cmd_test_a2dp_sink(bool enable)
{
    int ret = 0;
    char value[PROP_VALUE_MAX];
    struct rk_bluetooth *bt = NULL;

    if (enable) {
        char name[128] = {0};
        BTAddr bd_addr = {0};

        ret = get_bluetooth_device();
        if (ret) {
            printf("failed to fetch hw module: %s\n", BLUETOOTH_HARDWARE_MODULE_ID);
            return -1;
        }
        ret = BT_DEV->creat(&bt);
        if (ret || !bt) {
            printf("failed to creat bt\n");
            goto exit;
        }

        bt->set_discovery_listener(bt, discovery_callback, bt);
        bt->set_manage_listener(bt, manage_listener, bt);
        ret = bt->open(bt, NULL);
        if (ret) {
            printf("failed to open bt\n");
            goto exit;
        }
        usleep(100 * 1000);
        bt->get_module_addr(bt, bd_addr);
        snprintf(name, sizeof(name), "rokidftm_%02x:%02x:%02x:%02x:%02x:%02x",
                bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
        bt->a2dp_sink.set_listener(bt, a2dp_sink_listener, bt);
        bt->set_bt_name(bt, name);
        if (bt->a2dp_sink.enable(bt)) {
            printf("enable a2dp sink failed!\n");
            goto exit;
        }
        bt->set_visibility(bt, true, true);
        property_set("rokid.ftm.bt.sink", "1");
        do {
            property_get("rokid.ftm.bt.sink", value, "0");
            if (atoi(value) == 0) break;
            usleep(1000 * 1000);
        } while(1);
        bt->a2dp_sink.disable(bt);
    }
    else
        property_set("rokid.ftm.bt.sink", "0");

exit:
    if (BT_DEV) {
        if (bt) {
            BT_DEV->destroy(bt);
            bt = NULL;
        }
        BT_DEV->common.close((struct hw_device_t *)BT_DEV);
        BT_DEV = NULL;
    }
    return ret;
}

static void show_help(void)
{
    fprintf(stdout, "usage: bt_demo <command> <data>\n");
    fprintf(stdout, "<command> : scan /a2dpsink on / a2dpsink off / addr/ help\n");
}

int main(int argc, char **argv)
{
    int ret = -1;
    char **parse = argv;

    if (argc == 1)
        ret = main_loop();
    else {
        /* in test mode */
        /* Creat dir /data/factoryRokid */
	system(MKDIR_FACROKID_COMMAND);
        system(RM_RESULT_FILE);

        /* parse command line arguments */
        parse += 2;
        while (*parse) {
            if (strcmp(*parse, "-f") == 0) {
                SAVE_FILE = 1;
            }
            parse++;
        }
        if (!strcmp(argv[1], "scan")) {
            cmd_test_scan();
        }
        else if (!strcmp(argv[1], "a2dpsink")) {
            if (argv[2] && (!strcmp(argv[2], "on")))
                cmd_test_a2dp_sink(true);
            else if (argv[2] && (!strcmp(argv[2], "off")))
                cmd_test_a2dp_sink(false);
        }
        else if (!strcmp(argv[1], "addr")) {
            cmd_test_get_bt_addr();
        }
        else
            show_help();
    }
    //printf("test rokid bluetooth exit\n");
    return ret;
}


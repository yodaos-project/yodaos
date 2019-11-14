#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <json-c/json.h>

#include "common.h"

struct a2dpsource_handle *g_bt_a2dpsource = NULL;

void broadcast_a2dpsource_state(void *reply) {
    json_object *root = json_object_new_object();
    struct a2dpsource_state *a2dpsource_state = &g_bt_a2dpsource->state;
    struct bt_autoconnect_config *a2dpsource_config = &g_bt_a2dpsource->config;
    char *a2dpsource = NULL;
    char *conn_state = NULL;
    char *conn_name = NULL;
    char conn_addr[32] = {0};
    char *broadcast_state = NULL;
    char *re_state = NULL;
    char *action = "stateupdate";

    switch (a2dpsource_state->open_state) {
    case (A2DP_SOURCE_STATE_OPEN): {
        a2dpsource = "open";
        break;
    }
    case (A2DP_SOURCE_STATE_OPENED): {
        a2dpsource = "opened";
        break;
    }
    case (A2DP_SOURCE_STATE_OPENFAILED): {
        a2dpsource = "open failed";
        break;
    }
    case (A2DP_SOURCE_STATE_CLOSE): {
        a2dpsource = "close";
        break;
    }
    case (A2DP_SOURCE_STATE_CLOSING): {
        a2dpsource = "closing";
        break;
    }
    case (A2DP_SOURCE_STATE_CLOSED): {
        a2dpsource = "closed";
        break;
    }
    default:
        a2dpsource = "invalid";
        break;
    }

    switch (a2dpsource_state->connect_state) {
    case (A2DP_SOURCE_STATE_CONNECTED): {
        conn_state = "connected";
        conn_name = a2dpsource_config->dev[0].name;

        sprintf(conn_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                a2dpsource_config->dev[0].addr[0],
                a2dpsource_config->dev[0].addr[1],
                a2dpsource_config->dev[0].addr[2],
                a2dpsource_config->dev[0].addr[3],
                a2dpsource_config->dev[0].addr[4],
                a2dpsource_config->dev[0].addr[5]);
        break;
    }
    case (A2DP_SOURCE_STATE_CONNECTEFAILED): {
        conn_state = "connect failed";
        break;
    }
    case (A2DP_SOURCE_STATE_DISCONNECTED): {
        conn_state = "disconnected";
        break;
    }
    case (A2DP_SOURCE_STATE_CONNECTEOVER): {
        conn_state = "connect over";
        conn_name = a2dpsource_config->dev[0].name;
        sprintf(conn_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                a2dpsource_config->dev[0].addr[0],
                a2dpsource_config->dev[0].addr[1],
                a2dpsource_config->dev[0].addr[2],
                a2dpsource_config->dev[0].addr[3],
                a2dpsource_config->dev[0].addr[4],
                a2dpsource_config->dev[0].addr[5]);
        break;
    }
    default:
        conn_state = "invalid";
        break;
    }

    switch (a2dpsource_state->broadcast_state) {
    case (A2DP_SOURCE_BROADCAST_OPENED): {
        broadcast_state = "opened";
        break;
    }
    case (A2DP_SOURCE_BROADCAST_CLOSED): {
        broadcast_state = "closed";
        break;
    }
    default:
        break;
    }

    json_object_object_add(root, "linknum", json_object_new_int(a2dpsource_config->autoconnect_linknum));

    if (a2dpsource) {
        json_object_object_add(root, "a2dpstate", json_object_new_string(a2dpsource));
    }

    if (conn_state) {
        json_object_object_add(root, "connect_state", json_object_new_string(conn_state));
        if (conn_name) {
            json_object_object_add(root, "connect_name", json_object_new_string(conn_name));
            json_object_object_add(root, "connect_address", json_object_new_string(conn_addr));
        }
    }

    if (broadcast_state) {
        json_object_object_add(root, "broadcast_state", json_object_new_string(broadcast_state));
    }

    json_object_object_add(root, "action", json_object_new_string(action));

    re_state = (char *)json_object_to_json_string(root);
    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_A2DPSOURCE, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSOURCE, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}
static int get_a2dpsource_connected_device_num() {
    BTDevice device;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    memset(&device, 0, sizeof(device));

    return bt->a2dp.get_connected_devices(bt, &device, 1);
}

static void a2dp_source_listener (void *caller, BT_A2DP_EVT event, void *data) {
    //struct rk_bluetooth *bt = caller;
    BT_A2DP_MSG *msg = data;
    int index = 0;
    struct rk_bluetooth *bt = caller;
    struct bt_autoconnect_device dev ;
    struct a2dpsource_state *a2dpsource_state = &g_bt_a2dpsource->state;
    struct a2dpsource_timer *a2dpsource_timer = &g_bt_a2dpsource->timer;
    struct bt_autoconnect_config *a2dpsource_config = &g_bt_a2dpsource->config;

    memset(&dev, 0, sizeof(struct bt_autoconnect_device));

    switch (event) {
    case BT_A2DP_OPEN_EVT:
        if (msg) {
            if (msg->open.status == 0) {
                BT_LOGI("BT_A2DP_OPEN_EVT : status= %d, name: %s , addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                       msg->open.status, msg->open.dev.name,
                       msg->open.dev.bd_addr[0], msg->open.dev.bd_addr[1],
                       msg->open.dev.bd_addr[2], msg->open.dev.bd_addr[3],
                       msg->open.dev.bd_addr[4], msg->open.dev.bd_addr[5]);
                memcpy(dev.addr, msg->open.dev.bd_addr, sizeof(dev.addr));

                index = bt_autoconnect_get_index(a2dpsource_config, &dev);
                if (index >= 0) {
                    strncpy(dev.name, a2dpsource_config->dev[index].name, strlen(a2dpsource_config->dev[index].name));
                }
                BT_LOGV("old name :: %s\n", dev.name);

                if (strlen(msg->open.dev.name) != 0) {
                    BT_LOGV("connect name :; %s\n", msg->open.dev.name);
                    memset(dev.name, 0, sizeof(dev.name));
                    strncpy(dev.name, msg->open.dev.name, strlen(msg->open.dev.name));
                }

                eloop_timer_stop(a2dpsource_timer->e_connect_over_id);

                bt_autoconnect_update(a2dpsource_config, &dev);
                bt_autoconnect_sync(a2dpsource_config);

                if (bt->set_visibility(bt, false, false)) {
                    BT_LOGI("set visibility error\n");
                }

                if (a2dpsource_state->open_state != A2DP_SOURCE_STATE_CLOSED) {
                    a2dpsource_state->connect_state = A2DP_SOURCE_STATE_CONNECTED;
                    a2dpsource_state->broadcast_state = A2DP_SOURCE_BROADCAST_CLOSED;
                    broadcast_a2dpsource_state(NULL);
                }

            } else {
                int i;
                if (a2dpsource_config->autoconnect_flag == AUTOCONNECT_BY_HISTORY_WORK) {
                    eloop_timer_start(a2dpsource_timer->e_auto_connect_id);
                    for (i = 0; i < a2dpsource_config->autoconnect_num; i++) {
                        if (memcmp(a2dpsource_config->dev[i].addr, msg->open.dev.bd_addr, sizeof(msg->open.dev.bd_addr)) == 0) {
                            return ;
                        }
                    }
                }
                if (a2dpsource_config->autoconnect_flag == AUTOCONNECT_BY_HISTORY_OVER)  {
                    if (a2dpsource_state->open_state != A2DP_SOURCE_STATE_CLOSED) {
                        a2dpsource_state->connect_state = A2DP_SOURCE_STATE_CONNECTEFAILED;
                        a2dpsource_state->broadcast_state = A2DP_SOURCE_BROADCAST_OPENED;
                        broadcast_a2dpsource_state(NULL);
                    }
                }
            }
        }
        break;
    case BT_A2DP_CLOSE_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_CLOSE_EVT : status= %d\n", msg->close.status);

        a2dpsource_state->connect_state = A2DP_SOURCE_STATE_DISCONNECTED;

        if (bt->set_visibility(bt, true, true)) {
            BT_LOGI("set visibility error\n");
        }

        if (a2dpsource_state->open_state < A2DP_SOURCE_STATE_CLOSED) {
            a2dpsource_state->broadcast_state = A2DP_SOURCE_BROADCAST_OPENED;
            broadcast_a2dpsource_state(NULL);
        }

        if ((a2dpsource_state->open_state < A2DP_SOURCE_STATE_CLOSE) &&
                (a2dpsource_state->open_state >= A2DP_SOURCE_STATE_OPENED)) {
            eloop_timer_start(a2dpsource_timer->e_connect_over_id);
        }

        break;
    case BT_A2DP_RC_OPEN_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_RC_OPEN_EVT : status= %d\n", msg->rc_open.status);
        break;
    case BT_A2DP_RC_CLOSE_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_RC_CLOSE_EVT : status= %d\n", msg->rc_close.status);
        break;
    case BT_A2DP_START_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_START_EVT : status= %d\n", msg->start.status);
        break;
    case BT_A2DP_STOP_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_STOP_EVT : pause= %d\n", msg->stop.pause);
        break;
    case BT_A2DP_REMOTE_CMD_EVT:
        if (msg) {
            BT_LOGI("BT_A2DP_REMOTE_CMD_EVT : rc_cmd= 0x%x\n", msg->remote_cmd.rc_id);
            switch(msg->remote_cmd.rc_id) {
            case BT_AVRC_PLAY:
                BT_LOGI("Play key\n");
                break;
            case BT_AVRC_STOP:
                BT_LOGI("Stop key\n");
                break;
            case BT_AVRC_PAUSE:
                BT_LOGI("Pause key\n");
                break;
            case BT_AVRC_FORWARD:
                BT_LOGI("Forward key\n");
                break;
            case BT_AVRC_BACKWARD:
                BT_LOGI("Backward key\n");
                break;
            default:
                break;
            }
        }
        break;
    case BT_A2DP_REMOTE_RSP_EVT:
        if (msg)
            BT_LOGI("BT_A2DP_REMOTE_RSP_EVT : rc_cmd= 0x%x\n", msg->remote_rsp.rc_id);
        break;
    default:
        break;
    }

    return;
}

static int bt_a2dpsource_enable(struct rk_bluetooth *bt, char *name) {
    int ret = 0;
    struct a2dpsource_state *a2dpsource_state = &g_bt_a2dpsource->state;

    bt->a2dp.disable(bt);
    ret = bt->a2dp.set_listener(bt, a2dp_source_listener, bt);
    if (ret) {
        BT_LOGE("set ble listener error :: %d\n", ret);
        return -2;
    }

    bt->set_bt_name(bt, name);

    if (bt->a2dp.enable(bt)) {
        BT_LOGE("enable a2dp source failed!\n");
        return -2;
    }

    ret = bt->set_visibility(bt, true, true);
    if (ret) {
        BT_LOGE("set visibility error\n");
        return -3;
    }

    a2dpsource_state->broadcast_state = A2DP_SOURCE_BROADCAST_OPENED;

    return 0;
}

static void a2dpsource_bluetooth_autoconnect() {
    struct bt_autoconnect_config *a2dpsource_config = &g_bt_a2dpsource->config;
    struct a2dpsource_timer *a2dpsource_timer = &g_bt_a2dpsource->timer;

    if (get_a2dpsource_connected_device_num()) {
        a2dpsource_config->autoconnect_index = 0;
        return;
    }
    a2dpsource_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_WORK;
    a2dpsource_config->autoconnect_index = 0;
    eloop_timer_start(a2dpsource_timer->e_auto_connect_id);
    BT_LOGI("begin autoconnect\n");
}

static void auto_connect_a2dpsource_by_history_index(void *eloop_ctx) {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsource_timer *a2dpsource_timer = &g_bt_a2dpsource->timer;
    struct bt_autoconnect_config *a2dpsource_config = &g_bt_a2dpsource->config;
    struct a2dpsource_state *a2dpsource_state = &g_bt_a2dpsource->state;
    char zero_addr[6] = {0};
    int ret;


    if (get_a2dpsource_connected_device_num()) {
        a2dpsource_config->autoconnect_index = 0;
        a2dpsource_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_OVER;
        eloop_timer_stop(a2dpsource_timer->e_auto_connect_id);
        return;
    }
    if (a2dpsource_config->autoconnect_flag != AUTOCONNECT_BY_HISTORY_WORK) {
        a2dpsource_config->autoconnect_index = 0;
        return ;
    }

    BT_LOGI("autoconnect index ;:; %d\n", a2dpsource_config->autoconnect_index);
    if (a2dpsource_config->autoconnect_index >= 0) {
        if (a2dpsource_config->autoconnect_index < a2dpsource_config->autoconnect_num) {
            BT_LOGI("autoconnect  %02X:%02X:%02X:%02X:%02X:%02X\n",
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[0],
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[1],
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[2],
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[3],
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[4],
                     a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr[5]);
            BT_LOGI("autoconnect name :: %s\n", a2dpsource_config->dev[a2dpsource_config->autoconnect_index].name);
            if (memcmp(a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr, zero_addr, sizeof(zero_addr)) != 0) {
                ret = bt->a2dp.connect(bt, a2dpsource_config->dev[a2dpsource_config->autoconnect_index].addr);
                if (ret) {
                    BT_LOGI("autoconnect failed :: %d\n", ret);//busy  auto connect next
                    a2dpsource_config->autoconnect_index ++;
                    eloop_timer_start(a2dpsource_timer->e_auto_connect_id);
                    return;
                } else
                    a2dpsource_state->connect_state = A2DP_SOURCE_STATE_CONNECTING;
            }
        } else {
            a2dpsource_config->autoconnect_index = 0;
            a2dpsource_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_OVER;
            if ((a2dpsource_state->connect_state != A2DP_SOURCE_STATE_CONNECTED) &&
                (a2dpsource_config->autoconnect_linknum > 0)) {
                a2dpsource_state->connect_state = A2DP_SOURCE_STATE_CONNECTEOVER;
                broadcast_a2dpsource_state(NULL);
            }
            eloop_timer_stop(a2dpsource_timer->e_auto_connect_id);
            return ;
        }
    }

    a2dpsource_config->autoconnect_index++;
}

static void a2dpsource_autoconnect_over(void *eloop_ctx) {
    if (get_a2dpsource_connected_device_num()) {
        return;
    } else {
        bt_a2dpsource_off(g_bt_handle->bt);
    }
}

int bt_a2dpsource_timer_init() {
    struct a2dpsource_timer *timer = &g_bt_a2dpsource->timer;

    timer->e_auto_connect_id = eloop_timer_add(auto_connect_a2dpsource_by_history_index, NULL, 1 * 1000, 0);
    timer->e_connect_over_id = eloop_timer_add(a2dpsource_autoconnect_over, NULL, BLUETOOTH_A2DPSOURCE_AUTOCONNECT_LIMIT_TIME, 0);
    //timer->e_discovery_id = eloop_timer_add(a2dpsource_upload_scan_results, NULL, BLUETOOTH_A2DPSOURCE_DISCOVERY_LIMIT_TIME, BLUETOOTH_A2DPSOURCE_DISCOVERY_LIMIT_TIME);

    if ((timer->e_auto_connect_id <= 0) ||
        (timer->e_connect_over_id <= 0)) {
        return -1;
    }

    return 0;
}

void bt_a2dpsource_timer_uninit() {
    struct a2dpsource_timer *timer = &g_bt_a2dpsource->timer;

    eloop_timer_stop(timer->e_auto_connect_id);
    eloop_timer_stop(timer->e_connect_over_id);
    //eloop_timer_stop(timer->e_discovery_id);

    eloop_timer_delete(timer->e_auto_connect_id);
    eloop_timer_delete(timer->e_connect_over_id);
    //eloop_timer_delete(timer->e_discovery_id);
}
static void bt_a2dpsource_on(char *name) {
    int ret = 0;
    struct a2dpsource_handle *a2dpsource = g_bt_a2dpsource;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (a2dpsource->state.open_state == A2DP_SOURCE_STATE_OPENED) {
        broadcast_a2dpsource_state(NULL);
        a2dpsource_bluetooth_autoconnect();
        return;
    }
    pthread_mutex_lock(&(a2dpsource->state_mutex));

    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        a2dpsource->state.open_state = A2DP_SOURCE_STATE_OPENFAILED;
        broadcast_a2dpsource_state(NULL);
        pthread_mutex_unlock(&(a2dpsource->state_mutex));
        return ;
    }

    memset(&a2dpsource->state, 0, sizeof(struct a2dpsource_state));
    ret = bt_a2dpsource_enable(bt, name);
    if (ret == 0) {
        a2dpsource->state.open_state = A2DP_SOURCE_STATE_OPENED;
        g_bt_handle->status |= A2DP_SOURCE_STATUS_MASK;
    } else {
        a2dpsource->state.open_state = A2DP_SOURCE_STATE_OPENFAILED;
        broadcast_a2dpsource_state(NULL);
        pthread_mutex_unlock(&(a2dpsource->state_mutex));
        return ;
    }

    broadcast_a2dpsource_state(NULL);

    a2dpsource->config.autoconnect_flag = AUTOCONNECT_BY_HISTORY_INIT;

    a2dpsource_bluetooth_autoconnect();

    eloop_timer_start(a2dpsource->timer.e_connect_over_id);

    pthread_mutex_unlock(&(a2dpsource->state_mutex));
    return ;
}

void bt_a2dpsource_off(struct rk_bluetooth *bt) {
    int ret = 0;
    struct a2dpsource_handle *a2dpsource = g_bt_a2dpsource;

    if (a2dpsource->state.open_state == A2DP_SOURCE_STATE_CLOSED) {
        broadcast_a2dpsource_state(NULL);
        return;
    }
    pthread_mutex_lock(&(a2dpsource->state_mutex));

    ret = bt->cancel_discovery(bt);
    if (ret) {
        BT_LOGV("a2dpsource cancel discovery error\n");
    }

    ret = bt->a2dp.disable(bt);
    if (ret) {
        BT_LOGI("a2dpsource OFF error\n");
    }

    g_bt_handle->status &= ~A2DP_SOURCE_STATUS_MASK;
    bt_close(g_bt_handle);

    eloop_timer_stop(a2dpsource->timer.e_connect_over_id);
    eloop_timer_stop(a2dpsource->timer.e_auto_connect_id);
    //eloop_timer_stop(a2dpsource->timer.e_discovery_id);

    memset(&a2dpsource->state, 0, sizeof(struct a2dpsource_state));
    a2dpsource->state.broadcast_state = A2DP_SOURCE_BROADCAST_CLOSED;
    a2dpsource->state.open_state = A2DP_SOURCE_STATE_CLOSED;
    broadcast_a2dpsource_state(NULL);

    pthread_mutex_unlock(&(a2dpsource->state_mutex));
}

void bt_a2dpsource_on_prepare(struct bt_service_handle *handle, json_object *obj) {
    int unique = 0;
    json_object *bt_unique = NULL;

    if (json_object_object_get_ex(obj, "unique", &bt_unique) == TRUE) {
        unique = json_object_get_boolean(bt_unique);
        BT_LOGI("unique :: %d\n", unique);
        if (unique) {
            bt_close_mask_profile(handle, ~A2DP_SOURCE_STATUS_MASK);
        }
    }
}

static void bt_a2dpsource_disconnect_device(struct a2dpsource_handle *a2dpsource) {
    BTDevice device;
    int count = 0;
    int ret = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsource_state *a2dpsource_state = &a2dpsource->state;

    memset(&device, 0, sizeof(device));

    count = bt->a2dp.get_connected_devices(bt, &device, 1);
    if (count) {
        BT_LOGI("disconnect bdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                 device.bd_addr[0],
                 device.bd_addr[1],
                 device.bd_addr[2],
                 device.bd_addr[3],
                 device.bd_addr[4],
                 device.bd_addr[5]);
        ret = bt->a2dp.disconnect(bt, device.bd_addr);
        if (ret) {
            BT_LOGE("a2dp DISCONNECT error\n");
        }
    } else {
        a2dpsource_state->connect_state = A2DP_SOURCE_STATE_DISCONNECTED;
        broadcast_a2dpsource_state(NULL);
    }
}

void a2dpsource_upload_scan_results(const char *bt_name, BTAddr bt_addr, int is_completed, void *data) {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    BTDevice connect_dev;
    char address[18] = {0};
    BTDevice scan_devices[BT_DISC_NB_DEVICES];
    int scan_devices_num = 0;
    int i;
    char *action = "discovery";
    char *buf = NULL;

    if (!g_bt_handle->scanning) return;
    if (g_bt_a2dpsource->state.open_state != A2DP_SOURCE_STATE_OPENED) {
        return;
    }

    json_object *root = json_object_new_object();
    json_object *results = json_object_new_object();
    json_object *array = NULL;
    json_object *connect_device = NULL;
    json_object *item[BT_DISC_NB_DEVICES];

    json_object_object_add(results, "is_completed", json_object_new_boolean(is_completed));
    json_object_object_add(root, "action", json_object_new_string(action));

    memset(scan_devices, 0, sizeof(scan_devices));
    scan_devices_num = bt->get_disc_devices(bt, scan_devices, BT_DISC_NB_DEVICES);
    if (scan_devices_num > 0) {
        array = json_object_new_array();
        for (i = 0; i < scan_devices_num; i++) {
            memset(address, 0, sizeof(address));

            sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
                        scan_devices[i].bd_addr[0],
                        scan_devices[i].bd_addr[1],
                        scan_devices[i].bd_addr[2],
                        scan_devices[i].bd_addr[3],
                        scan_devices[i].bd_addr[4],
                        scan_devices[i].bd_addr[5]);

            item[i] = json_object_new_object();
            json_object_object_add(item[i], "address", json_object_new_string(address));
            json_object_object_add(item[i], "name", json_object_new_string(scan_devices[i].name));
            json_object_array_add(array, item[i]);
        }
        json_object_object_add(results, "deviceList", array);
    }

    memset(&connect_dev, 0, sizeof(BTDevice));
    if (bt->a2dp.get_connected_devices(bt, &connect_dev, 1)) {
        connect_device = json_object_new_object();
        memset(address, 0, sizeof(address));
        sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
                    connect_dev.bd_addr[0],
                    connect_dev.bd_addr[1],
                    connect_dev.bd_addr[2],
                    connect_dev.bd_addr[3],
                    connect_dev.bd_addr[4],
                    connect_dev.bd_addr[5]);

        json_object_object_add(connect_device, "name", json_object_new_string(connect_dev.name));
        json_object_object_add(connect_device, "address", json_object_new_string(address));
        json_object_object_add(results, "currentDevice", connect_device);
    }
    json_object_object_add(root, "results", results);

    buf = (char *)json_object_to_json_string(root);
    BT_LOGD("results :: %s\n", buf);
    report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSOURCE, (uint8_t *)buf, strlen(buf));
    json_object_put(root);
}

static void bt_a2dpsource_discovery() {
    struct a2dpsource_handle *a2dpsource = g_bt_a2dpsource;
    int ret = 0;

    if (a2dpsource->state.open_state != A2DP_SOURCE_STATE_OPENED) {
        broadcast_a2dpsource_state(NULL);
        return;
    }
    ret = bt_discovery(g_bt_handle, BT_A2DP_SINK);
    if (ret) {
        broadcast_a2dpsource_state(NULL);
        return;
    }
}

void bt_a2dpsource_connect(const char *addr) {
    struct a2dpsource_handle *a2dpsource = g_bt_a2dpsource;
    uint8_t bd_addr[6] = {0};
    int ret = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (a2dpsource->state.open_state != A2DP_SOURCE_STATE_OPENED ||
        get_a2dpsource_connected_device_num()) {
        broadcast_a2dpsource_state(NULL);
        return;
    }

    ret = bd_strtoba(bd_addr, addr);
    if (ret != 0) {
        BT_LOGE("mac not valid\n");
        return ;
    }

    ret = bt->a2dp.connect(bt, bd_addr);
    if (ret != 0) {
        BT_LOGE("connect error\n");
        a2dpsource->state.connect_state = A2DP_SOURCE_STATE_CONNECTEFAILED;
        broadcast_a2dpsource_state(NULL);
        return ;
    }
}

int handle_a2dpsource_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    json_object *bt_cmd = NULL;
    json_object *bt_name = NULL;
    json_object *bt_addr = NULL;
    struct a2dpsource_state *a2dpsource_state = NULL;
    struct a2dpsource_timer *a2dpsource_timer = &g_bt_a2dpsource->timer;
    struct rk_bluetooth *bt = NULL;
    BTAddr bd_addr = {0};
    char name[128] = {0};

    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("bt_a2dpsource :: command %s \n", command);

        a2dpsource_state = &g_bt_a2dpsource->state;
        bt = g_bt_handle->bt;

        if (strcmp(command, "ON") == 0) {
            if (a2dpsource_state->open_state == A2DP_SOURCE_STATE_OPENED &&
                a2dpsource_state->connect_state != A2DP_SOURCE_STATE_CONNECTED) {
                a2dpsource_state->connect_state = A2DP_SOURCE_STATE_CONNECT_INVALID;
                eloop_timer_stop(a2dpsource_timer->e_connect_over_id);
                eloop_timer_start(a2dpsource_timer->e_connect_over_id);
            }

            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                snprintf(name, sizeof(name), "%s", (char *)json_object_get_string(bt_name));
            } else {
                bt->get_module_addr(bt, bd_addr);
                snprintf(name, sizeof(name), "rokidsource_%02x:%02x:%02x:%02x:%02x:%02x",
                         bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);
            }
            BT_LOGI("bt_a2dpsource :: name %s \n", name);

            bt_a2dpsource_on_prepare(handle, obj);
            bt_a2dpsource_on(name);
        } else if (strcmp(command, "OFF") == 0) {
            bt_a2dpsource_off(bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_a2dpsource_state(reply);
        } else if (strcmp(command, "DISCOVERY") == 0) {
            bt_a2dpsource_discovery();
        } else if (strcmp(command, "DISCONNECT") == 0) {
            bt_a2dpsource_disconnect_device(g_bt_a2dpsource);
        } else if (strcmp(command, "DISCONNECT_PEER") == 0) {
            bt_a2dpsource_disconnect_device(g_bt_a2dpsource);
        } else if (strcmp(command, "CONNECT") == 0) {
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_a2dpsource_connect((char *)json_object_get_string(bt_addr));
            }
        }
    }

    return 0;
}

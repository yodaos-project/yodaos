#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <json-c/json.h>

#include "common.h"

struct hh_handle *g_bt_hh = NULL;

void broadcast_hh_state(void *reply) {
    struct hh_state *hh_state = &g_bt_hh->state;
    json_object *root = json_object_new_object();
    char *state = NULL;
    char *connect_state = NULL;
    char *re_state = NULL;
    char *name = NULL;
    char addr[18] = {0};
    char *action = "stateupdate";

    switch (hh_state->open_state) {
    case HH_STATE_OPEN: {
        state = "open";
        break;
    }
    case HH_STATE_OPENING: {
        state = "opening";
        break;
    }
    case HH_STATE_OPENED: {
        state = "opened";
        break;
    }
    case HH_STATE_OPENFAILED: {
        state = "openfailed";
        break;
    }
    case HH_STATE_CLOSE: {
        state = "close";
        break;
    }
    case HH_STATE_CLOSING: {
        state = "closing";
        break;
    }
    case HH_STATE_CLOSED: {
        state = "closed";
        break;
    }
    default:
        state = "invalid";
        break;
    }

    switch (hh_state->connect_state) {
    case HH_STATE_CONNECTING: {
        connect_state = "conncting";
        break;
    }
    case HH_STATE_CONNECTED: {
        connect_state = "connected";
        name = g_bt_hh->dev.name;
        sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    g_bt_hh->dev.addr[0],
                    g_bt_hh->dev.addr[1],
                    g_bt_hh->dev.addr[2],
                    g_bt_hh->dev.addr[3],
                    g_bt_hh->dev.addr[4],
                    g_bt_hh->dev.addr[5]);
        break;
    }
    case HH_STATE_CONNECTFAILED: {
        connect_state = "connectfailed";
        break;
    }
    case HH_STATE_DISCONNECTED: {
        connect_state = "disconnected";
        name = g_bt_hh->dev.name;
        sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    g_bt_hh->dev.addr[0],
                    g_bt_hh->dev.addr[1],
                    g_bt_hh->dev.addr[2],
                    g_bt_hh->dev.addr[3],
                    g_bt_hh->dev.addr[4],
                    g_bt_hh->dev.addr[5]);
        break;
    }
    default:
        connect_state = "invalid";
        break;
    }

    json_object_object_add(root, "action", json_object_new_string(action));
    json_object_object_add(root, "state", json_object_new_string(state));
    json_object_object_add(root, "connect_state", json_object_new_string(connect_state));
    if (name) {
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "address", json_object_new_string(addr));
    }
    re_state = (char *)json_object_to_json_string(root);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_HH, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_HH, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}

void hh_listener(void *caller, BT_HH_EVT event, void *data) {
    //struct rk_bluetooth *bt = caller;
    BT_HH_MSG *msg = data;
    struct hh_handle *hh = g_bt_hh;

    switch (event) {
    case BT_HH_REPORT_EVT:
            if (msg) {
                BT_LOGI("BT_HH_REPORT_EVT : BdAddr:%02X:%02X:%02X:%02X:%02X:%02X len(%d)\n", msg->report.bd_addr[0], msg->report.bd_addr[1], msg->report.bd_addr[2],
                                msg->report.bd_addr[3], msg->report.bd_addr[4], msg->report.bd_addr[5], msg->report.report_data.length);
            }
        break;
    case BT_HH_OPEN_EVT:
        if (msg)
            BT_LOGI("BT_HH_OPEN_EVT : status= %d, BdAddr:%02X:%02X:%02X:%02X:%02X:%02X\n", msg->open.status,msg->open.bd_addr[0], msg->open.bd_addr[1], msg->open.bd_addr[2],
                                msg->open.bd_addr[3], msg->open.bd_addr[4], msg->open.bd_addr[5]);

        if (msg->open.status)
            hh->state.connect_state = HH_STATE_CONNECTFAILED;
        else {
            hh->state.connect_state = HH_STATE_CONNECTED;
        }
        snprintf(hh->dev.name, sizeof(hh->dev.name), "%s", msg->open.name);
        memcpy(hh->dev.addr, msg->open.bd_addr, sizeof(hh->dev.addr));
        broadcast_hh_state(NULL);
        break;
    case BT_HH_CLOSE_EVT:
        if (msg)
            BT_LOGI("BT_HH_CLOSE_EVT : reason= %d, BdAddr:%02X:%02X:%02X:%02X:%02X:%02X\n", msg->close.status, msg->close.bd_addr[0], msg->close.bd_addr[1], msg->close.bd_addr[2],
                                msg->close.bd_addr[3], msg->close.bd_addr[4], msg->close.bd_addr[5]);

        hh->state.connect_state = HH_STATE_DISCONNECTED;
        broadcast_hh_state(NULL);
        break;
    default:
        break;
    }

    return;
}

static void bt_hh_on(struct rk_bluetooth *bt, char *name) {
    int ret = 0;
    struct hh_handle *hh = g_bt_hh;

    if (hh->state.open_state == HH_STATE_OPENED) {
        broadcast_hh_state(NULL);
        return;
    }
    pthread_mutex_lock(&(hh->state_mutex));

    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        hh->state.open_state = HH_STATE_OPENFAILED;
        broadcast_hh_state(NULL);
        pthread_mutex_unlock(&(hh->state_mutex));
        return ;
    }

    bt->hh.set_listener(bt, hh_listener, bt);

    bt->set_bt_name(bt, name);

    if (0 != bt->hh.enable(bt)) {
        BT_LOGE("enbale hh failed\n");
        hh->state.open_state = HH_STATE_OPENFAILED;
        broadcast_hh_state(NULL);
        pthread_mutex_unlock(&(hh->state_mutex));
        return ;
    }
    g_bt_handle->status |= HID_HOST_STATUS_MASK;

    memset(&hh->state, 0, sizeof(struct hh_state));

    hh->state.open_state = HH_STATE_OPENED;
    broadcast_hh_state(NULL);

    pthread_mutex_unlock(&(hh->state_mutex));

    return ;
}

void bt_hh_off(struct rk_bluetooth *bt) {
    struct hh_handle *hh = g_bt_hh;

    if (hh->state.open_state >= HH_STATE_CLOSE) {
        broadcast_hh_state(NULL);
        return ;
    }
    pthread_mutex_lock(&(hh->state_mutex));
    hh->state.open_state = HH_STATE_CLOSE;
    bt->hh.disable(bt);
    g_bt_handle->status &= ~HID_HOST_STATUS_MASK;
    bt_close(g_bt_handle);
    hh->state.open_state = HH_STATE_CLOSED;
    broadcast_hh_state(NULL);
    pthread_mutex_unlock(&(hh->state_mutex));
}
static int get_hh_connected_devices_num(struct rk_bluetooth *bt) {
    BTDevice device[20];

    memset(device, 0, sizeof(device));

    return bt->hh.get_connected_devices(bt, device, 20);
}
void bt_hh_connect(struct rk_bluetooth *bt, const char *addr) {
    uint8_t bd_addr[6] = {0};
    int ret = 0;
    struct hh_handle *hh = g_bt_hh;

    if (hh->state.open_state != HH_STATE_OPENED ||
        get_hh_connected_devices_num(bt)) {
        broadcast_hh_state(NULL);
        return;
    }
    ret = bd_strtoba(bd_addr, addr);
    if (ret != 0) {
        BT_LOGE("mac not valid\n");
        return ;
    }

    ret = bt->hh.connect(bt, bd_addr, 0);
    if (ret != 0) {
        BT_LOGE("connect error\n");
        broadcast_hh_state(NULL);
        return ;
    }
}

void bt_hh_disconnect(struct rk_bluetooth *bt, const char *address) {
    uint8_t bd_addr[6] = {0};
    int ret = 0;
    BTDevice device[20];
    int num;

    memset(device, 0, sizeof(device));

    num = bt->hh.get_connected_devices(bt, device, 20);
    if (num) {
        if (address) {
            ret = bd_strtoba(bd_addr, address);
            if (ret != 0) {
                BT_LOGE("mac not valid\n");
                return ;
            }
             ret = bt->hh.disconnect(bt, bd_addr);
             if (ret != 0) {
                BT_LOGE("disconnect error\n");
                broadcast_hh_state(NULL);
                return ;
            }
        } else {//disconnect all
            int i;
            for (i=0; i<num;i++) {
                bt->hh.disconnect(bt, device[i].bd_addr);
            }
        }
    } else {
        broadcast_hh_state(NULL);
    }
}

void bt_hh_on_prepare(struct bt_service_handle *handle, json_object *obj) {

}

int handle_hh_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    json_object *bt_cmd = NULL;

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("hh :: command %s \n", command);

        if (strcmp(command, "ON") == 0) {
            json_object *bt_name = NULL;
            char *name = NULL;

            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                name = (char *)json_object_get_string(bt_name);
                BT_LOGI("hh :: name %s \n", name);
            } else {
                name = "ROKID-BT-9999zz";
            }

            bt_hh_on_prepare(handle, obj);
            bt_hh_on(handle->bt, name);
        } else if (strcmp(command, "OFF") == 0) {
            bt_hh_off(handle->bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_hh_state(reply);
        } else if (strcmp(command, "CONNECT") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_hh_connect(handle->bt, (char *)json_object_get_string(bt_addr));
            }
        } else if (strcmp(command, "DISCONNECT") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_hh_disconnect(handle->bt, (char *)json_object_get_string(bt_addr));
            } else
                bt_hh_disconnect(handle->bt, NULL);
        }
    }

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <json-c/json.h>

#include "common.h"

struct ble_client_handle *g_bt_ble_client = NULL;

void broadcast_ble_client_state(void *reply) {
    struct ble_client_state *ble_client_state = &g_bt_ble_client->state;
    json_object *root = json_object_new_object();
    char *state = NULL;
    char *connect = NULL;
    char *name = NULL;
    char addr[18] = {0};
    char *re_state = NULL;
    char *action = "stateupdate";

    switch (ble_client_state->open_state) {
    case BLE_CLI_STATE_OPENING: {
        state = "opening";
        break;
    }
    case BLE_CLI_STATE_OPENED: {
        state = "opened";
        break;
    }
    case BLE_CLI_STATE_OPENFAILED: {
        state = "openfailed";
        break;
    }
    case BLE_CLI_STATE_CLOSE: {
        state = "close";
        break;
    }
    case BLE_CLI_STATE_CLOSING: {
        state = "closing";
        break;
    }
    case BLE_CLI_STATE_CLOSED: {
        state = "closed";
        break;
    }
    default:
        state = "invalid";
        break;
    }
    switch (ble_client_state->connect_state) {
    case BLE_CLI_STATE_CONNECTING: {
        connect = "connecting";
        break;
    }
    case BLE_CLI_STATE_CONNECTED: {
        connect = "connected";
        name = g_bt_ble_client->dev.name;
        sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    g_bt_ble_client->dev.addr[0],
                    g_bt_ble_client->dev.addr[1],
                    g_bt_ble_client->dev.addr[2],
                    g_bt_ble_client->dev.addr[3],
                    g_bt_ble_client->dev.addr[4],
                    g_bt_ble_client->dev.addr[5]);
        break;
    }
    case BLE_CLI_STATE_CONNECTFAILED: {
        connect = "connectfailed";
        name = g_bt_ble_client->dev.name;
        sprintf(addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    g_bt_ble_client->dev.addr[0],
                    g_bt_ble_client->dev.addr[1],
                    g_bt_ble_client->dev.addr[2],
                    g_bt_ble_client->dev.addr[3],
                    g_bt_ble_client->dev.addr[4],
                    g_bt_ble_client->dev.addr[5]);
        break;
    }
    case BLE_CLI_STATE_DISCONNECTED: {
        connect = "disconnected";
        break;
    }
    default:
        connect = "invalid";
        break;
    }

    json_object_object_add(root, "action", json_object_new_string(action));
    json_object_object_add(root, "state", json_object_new_string(state));
    json_object_object_add(root, "connect_state", json_object_new_string(connect));
    if (name) {
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "address", json_object_new_string(addr));
    }
    re_state = (char *)json_object_to_json_string(root);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_BLE_CLIENT, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_BLE_CLIENT, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}


void rokid_ble_client_receive_data(int type, char *addr, char *service, char *character,   char *buf, int len) {
    struct rokid_ble_client_header *head;
    char *data = NULL;

    data = malloc(sizeof(struct rokid_ble_client_header) + len);
    if (!data) return;
    memset(data, 0, (sizeof(struct rokid_ble_client_header) + len));
    head = (struct rokid_ble_client_header *)data;
    head->len = sizeof(struct rokid_ble_client_header);
    memcpy(head->address, addr, sizeof(head->address));
    memcpy(head->service, service, sizeof(head->service));
    memcpy(head->character, character, sizeof(head->character));
    memcpy(data + sizeof(struct rokid_ble_client_header) , buf, len);
    report_bluetooth_binary_information(type, (uint8_t *)data, (sizeof(struct rokid_ble_client_header) + len));
    free(data);
}

void ble_client_listener(void *caller, BT_BLE_EVT event, void *data) {
    //struct rk_bluetooth *bt = caller;
    BT_BLE_MSG *msg = data;
    struct ble_client_handle *ble = g_bt_ble_client;

    switch (event) {
    case BT_BLE_CL_OPEN_EVT:
        if (msg) {
            BT_LOGI("BT_BLE_CL_OPEN_EVT : status= %d, %s\n", msg->cli_open.status, msg->cli_open.name);
            if (msg->cli_open.status)
                ble->state.connect_state = BLE_CLI_STATE_CONNECTFAILED;
            else {
                ble->state.connect_state = BLE_CLI_STATE_CONNECTED;
            }
            snprintf(ble->dev.name, sizeof(ble->dev.name), "%s", msg->cli_open.name);
            memcpy(ble->dev.addr, msg->cli_open.bd_addr, sizeof(ble->dev.addr));
            broadcast_ble_client_state(NULL);
        }
        break;
    case BT_BLE_CL_CLOSE_EVT:
        if (msg) {
            BT_LOGI("BT_BLE_CL_CLOSE_EVT : status= %d, %s\n", msg->cli_close.status, msg->cli_close.name);
            ble->state.connect_state = BLE_CLI_STATE_DISCONNECTED;
            broadcast_ble_client_state(NULL);
        }
        break;
    case BT_BLE_CL_NOTIF_EVT:
        if (msg) {
            char addr[18];
            snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X", 
                msg->cli_notif.bda[0], msg->cli_notif.bda[1], msg->cli_notif.bda[2],
                msg->cli_notif.bda[3], msg->cli_notif.bda[4], msg->cli_notif.bda[5]);
            BT_LOGD("BT_BLE_CL_NOTIF_EVT : len(%d), %s", msg->cli_notif.len, addr);
            rokid_ble_client_receive_data(BLUETOOTH_FUNCTION_BLE_CLI_NOTIFY, addr, msg->cli_notif.service, msg->cli_notif.char_id, (char *)msg->cli_notif.value, msg->cli_notif.len);
        }
        break;
    case BT_BLE_CL_READ_EVT:
        if (msg) {
            char addr[18];
            snprintf(addr, sizeof(addr), "%02X:%02X:%02X:%02X:%02X:%02X", 
                msg->cli_read.bda[0], msg->cli_read.bda[1], msg->cli_read.bda[2],
                msg->cli_read.bda[3], msg->cli_read.bda[4], msg->cli_read.bda[5]);
            BT_LOGI("BT_BLE_CL_READ_EVT : len(%d), %s", msg->cli_read.len, addr);
            rokid_ble_client_receive_data(BLUETOOTH_FUNCTION_BLE_CLI_READ, addr, msg->cli_read.service, msg->cli_read.char_id, (char *)msg->cli_read.value, msg->cli_read.len);
        }
        break;
    default:
        break;
    }

    return;
}

static void bt_ble_client_on(struct rk_bluetooth *bt, char *name) {
    int ret = 0;
    struct ble_client_handle *ble = g_bt_ble_client;

    if (ble->state.open_state == BLE_CLI_STATE_OPENED) {
        broadcast_ble_client_state(NULL);
        return;
    }
    pthread_mutex_lock(&(ble->state_mutex));

    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        ble->state.open_state = BLE_CLI_STATE_OPENFAILED;
        broadcast_ble_client_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return ;
    }

    bt->blec.set_listener(bt, ble_client_listener, bt);

    bt->set_bt_name(bt, name);

    if (0 != bt->blec.enable(bt)) {
        BT_LOGE("enbale ble failed\n");
        ble->state.open_state = BLE_CLI_STATE_OPENFAILED;
        broadcast_ble_client_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return ;
    }
    g_bt_handle->status |= BLE_CLIENT_STATUS_MASK;

    memset(&ble->state, 0, sizeof(struct ble_client_state));

    ble->state.open_state = BLE_CLI_STATE_OPENED;
    broadcast_ble_client_state(NULL);

    pthread_mutex_unlock(&(ble->state_mutex));

    return ;
}

void bt_ble_client_off(struct rk_bluetooth *bt) {
    struct ble_client_handle *ble = g_bt_ble_client;

    pthread_mutex_lock(&(ble->state_mutex));

    if (ble->state.open_state >= BLE_CLI_STATE_CLOSE) {
        broadcast_ble_client_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return;
    }
    ble->state.open_state = BLE_CLI_STATE_CLOSE;
    bt->blec.disable(bt);
    g_bt_handle->status &= ~BLE_CLIENT_STATUS_MASK;
    bt_close(g_bt_handle);
    ble->state.open_state = BLE_CLI_STATE_CLOSED;
    broadcast_ble_client_state(NULL);
    pthread_mutex_unlock(&(ble->state_mutex));
}

static int get_ble_client_connected_device_num(struct rk_bluetooth *bt) {
    BTDevice device[20];

    memset(device, 0, sizeof(device));

    return bt->blec.get_connected_devices(bt, device, 20);
}

void bt_ble_client_connect(struct rk_bluetooth *bt, char *address) {
    int ret;
    BTAddr bd_addr = {0};
    struct ble_client_handle *ble = g_bt_ble_client;

    if (ble->state.open_state != BLE_CLI_STATE_OPENED ||
        (ble->state.connect_state == BLE_CLI_STATE_CONNECTING ||
        get_ble_client_connected_device_num(bt))) {
        broadcast_ble_client_state(NULL);
        return;
    }
    ret = bd_strtoba(bd_addr, address);
    if (ret != 0) {
        BT_LOGE("mac not valid\n");
        return ;
    }
    ret = bt->blec.connect(bt, bd_addr);
    if (ret != 0) {
        BT_LOGE("connect error\n");
        ble->state.connect_state = BLE_CLI_STATE_CONNECTFAILED;
        broadcast_ble_client_state(NULL);
        return ;
    }
    ble->state.connect_state = BLE_CLI_STATE_CONNECTING;
}

void bt_ble_client_disconnect(struct rk_bluetooth *bt, char *address) {
    int ret;
    BTDevice device[20];
    int num;

    memset(device, 0, sizeof(device));

    num = bt->blec.get_connected_devices(bt, device, 20);
    if (num) {
        if (address) {
            BTAddr bd_addr = {0};
            ret = bd_strtoba(bd_addr, address);
            if (ret != 0) {
                BT_LOGE("mac address not valid\n");
                return ;
            }
            bt->blec.disconnect(bt, bd_addr);
        } else {//disconnect all
            int i;
            for (i=0; i<num;i++) {
                bt->blec.disconnect(bt, device[i].bd_addr);
            }
        }
    } else {
        broadcast_ble_client_state(NULL);
    }
}

void ble_client_write_data(struct rk_bluetooth *bt, char *service, char *character, char *buf, int len) {
    int index_len = 0;
    char send_buf[BLE_DATA_LEN] = {0};
    int act_len;

    if (get_ble_client_connected_device_num(bt)) {
        while (len > 0) {
                act_len = (len > BLE_DATA_LEN) ? BLE_DATA_LEN : len;
                memcpy(send_buf, buf + index_len, act_len);
                bt->blec.write(bt, service, character, send_buf, act_len);
                usleep(20 * 1000);
                index_len += act_len;
                len -= act_len;
        }
    }
}

void ble_client_register_notify(struct rk_bluetooth *bt, char *service, char *character) {

    if (get_ble_client_connected_device_num(bt)) {
        bt->blec.register_notification(bt, service, character);
    } else {
        broadcast_ble_client_state(NULL);
    }
}

void ble_client_read(struct rk_bluetooth *bt, char *service, char *character) {

    if (get_ble_client_connected_device_num(bt)) {
        bt->blec.read(bt, service, character);
    } else {
        broadcast_ble_client_state(NULL);
    }
}


void bt_ble_client_on_prepare(struct bt_service_handle *handle, json_object *obj) {
    //todo
}

int handle_ble_client_write(struct bt_service_handle *handle, const char *buff, int len) {
    struct rokid_ble_client_header *head = (struct rokid_ble_client_header *)buff;
    ble_client_write_data(handle->bt, (char *)head->service, (char *)head->character, ((char *)buff + head->len), (len - head->len));
    return 0;
}

int handle_ble_client_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL; 
    json_object *bt_cmd = NULL; 

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("bt_ble_client :: command %s \n", command);

        if (strcmp(command, "ON") == 0) {
            json_object *bt_name = NULL;
            char *name = NULL;
            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                name = (char *)json_object_get_string(bt_name);
                BT_LOGI("bt_ble_client :: name %s \n", name);
            } else {
                name = "ROKID-BT-9999zz";
            }

            bt_ble_client_on_prepare(handle, obj);
            bt_ble_client_on(handle->bt, name);
        } else if (strcmp(command, "OFF") == 0) {
            bt_ble_client_off(handle->bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_ble_client_state(reply);
        } else if (strcmp(command, "REGISTER_NOTIFY") == 0) {
            json_object *bt_obj = NULL;
            char *s_uuid = NULL;
            char *c_uuid = NULL;
            if (json_object_object_get_ex(obj, "service", &bt_obj) == TRUE) {
                s_uuid = (char *)json_object_get_string(bt_obj);
                if (json_object_object_get_ex(obj, "character", &bt_obj) == TRUE) {
                    c_uuid = (char *)json_object_get_string(bt_obj);
                    BT_LOGI("bt_ble_client :: register notify, service %s, character %s\n", s_uuid, c_uuid);
                    ble_client_register_notify(handle->bt, s_uuid, c_uuid);
                }
            }
        } else if (strcmp(command, "READ") == 0) {
            json_object *bt_obj = NULL;
            char *s_uuid = NULL;
            char *c_uuid = NULL;
            if (json_object_object_get_ex(obj, "service", &bt_obj) == TRUE) {
                s_uuid = (char *)json_object_get_string(bt_obj);
                if (json_object_object_get_ex(obj, "character", &bt_obj) == TRUE) {
                    c_uuid = (char *)json_object_get_string(bt_obj);
                    BT_LOGI("bt_ble_client :: read, service %s, character %s\n", s_uuid, c_uuid);
                    ble_client_read(handle->bt, s_uuid, c_uuid);
                }
            }
        } else if (strcmp(command, "CONNECT") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_ble_client_connect(handle->bt, (char *)json_object_get_string(bt_addr));
            }
        } else if (strcmp(command, "DISCONNECT") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_ble_client_disconnect(handle->bt, (char *)json_object_get_string(bt_addr));
            } else {
                bt_ble_client_disconnect(handle->bt, NULL);
            }
        }
    }

    return 0;
}

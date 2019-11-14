#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <json-c/json.h>

#include "common.h"

static int msgid = 0;
static char sendbuf[512] = {0};
struct ble_handle *g_bt_ble = NULL;

void broadcast_ble_state(void *reply) {
    struct ble_state *ble_state = &g_bt_ble->state;
    json_object *root = json_object_new_object();
    char *state = NULL;
    char *re_state = NULL;

    switch (ble_state->open_state) {
    case BLE_STATE_OPEN: {
        state = "open";
        break;
    }
    case BLE_STATE_OPENING: {
        state = "opening";
        break;
    }
    case BLE_STATE_OPENED: {
        state = "opened";
        break;
    }
    case BLE_STATE_OPENFAILED: {
        state = "openfailed";
        break;
    }
    case BLE_STATE_CONNECTED: {
        state = "connected";
        break;
    }
    case BLE_STATE_HANDSHAKED: {
        state = "handshaked";
        break;
    }
    case BLE_STATE_DISCONNECTED: {
        state = "disconnected";
        break;
    }
    case BLE_STATE_CLOSE: {
        state = "close";
        break;
    }
    case BLE_STATE_CLOSING: {
        state = "closing";
        break;
    }
    case BLE_STATE_CLOSED: {
        state = "closed";
        break;
    }
    default:
        break;
    }

    json_object_object_add(root, "state", json_object_new_string(state));
    re_state = (char *)json_object_to_json_string(root);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_BLE, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_BLE, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}

void ble_send_raw_data(struct rk_bluetooth *bt, uint8_t *buf, int len) {
    int index_len = 0;
    uint8_t send_buf[BLE_DATA_LEN] = {0};
    struct ble_state *ble_state = &g_bt_ble->state;
    int act_len;

    if ((ble_state->open_state > BLE_STATE_OPENED) && (ble_state->open_state < BLE_STATE_DISCONNECTED)) {
        while (len > 0) {
            act_len = (len > BLE_DATA_LEN) ? BLE_DATA_LEN : len;
            memcpy(send_buf, buf + index_len, act_len);
            bt->ble.send_buf(bt, BLE_DATA_CHARACTER, (char *)send_buf, act_len);
            usleep(20 * 1000);
            index_len += act_len;
            len -= act_len;
        }
    } else {
        broadcast_ble_state(NULL);
    }
}

void rokid_send_ble_data(struct rk_bluetooth *bt, uint8_t *buf, int len) {
    struct rokid_ble_format data;
    struct ble_handle *ble = g_bt_ble;
    int tmp_len = 0;
    int index_len = 0;

    pthread_mutex_lock(&(ble->state_mutex));

    while (len > 0) {
        memset(&data, 0, sizeof(struct rokid_ble_format));
        if (len > ROKID_BLE_DATA_LEN) {
            tmp_len = ROKID_BLE_DATA_LEN;
        } else {
            tmp_len = len;
            data.last_index = 0x80;
        }

        data.msgid = msgid;

        memcpy(data.data, buf + index_len, tmp_len);

        bt->ble.send_buf(bt, BLE_DATA_CHARACTER, (char *)&data, tmp_len + 3);

        usleep(10 * 1000);

        index_len += tmp_len;
        len -= tmp_len;
    }
    pthread_mutex_unlock(&(ble->state_mutex));
}

static void report_recv_data(void) {
    char *re_data = NULL;
    json_object *root = json_object_new_object();

    json_object_object_add(root, "data", json_tokener_parse(sendbuf));
    re_data = (char *)json_object_to_json_string(root);
    BT_LOGI("ble real data:: %s\n", re_data);
    report_bluetooth_information(BLUETOOTH_FUNCTION_BLE, (uint8_t *)re_data, strlen(re_data));
    memset(sendbuf, 0, sizeof(sendbuf));
    json_object_put(root);
}

void rokid_recv_ble_data(uint8_t *buf, int len) {
    struct rokid_ble_format data;
    static int index_len = 0;

    memset(&data, 0, sizeof(struct rokid_ble_format));
    memcpy(&data, buf, len);

    BT_LOGI("msgid :: %02x current :: %02x\n", msgid, data.msgid);
    if (msgid != data.msgid) {
        memset(sendbuf, 0, sizeof(sendbuf));
        index_len = 0;
        msgid = data.msgid;
    }

    if ((data.last_index & 0x0080) == 0) {
        BT_LOGI("not enough\n");
        if (index_len + len -3  > 255) {
            BT_LOGE("err: sendbuf overflow\n");
            index_len = 0;
        }
        memcpy(sendbuf + index_len, data.data, len - 3);
        index_len += (len - 3);
        return;
    } else {
        BT_LOGI("enough\n");
        memcpy(sendbuf + index_len, data.data, len - 3);
        index_len = 0;
    }
    report_recv_data();
}

void rokid_recv_ble_data_cmcc(uint8_t *buf, int len) {
    static int index_len = 0;
    static int pkgs = 0;
    char *pkgname = "size:";

    /*size:xxx*/
    if (memcmp(buf, pkgname, strlen(pkgname)) == 0) {
        memset(sendbuf, 0, sizeof(sendbuf));
        index_len = 0;
        pkgs = atoi(buf + strlen(pkgname));
        BT_LOGI("receive package nums %d", pkgs);
    } else {
        if (pkgs > 0) {
            memcpy(sendbuf + index_len, buf, len);
            pkgs--;
            index_len += len;
            if (pkgs == 0) {
                /*enough*/
                BT_LOGI("enough\n");
                report_recv_data();
                index_len = 0;
            }
        } else {
            BT_LOGI("receive error!!!\n");
            index_len =0;
            pkgs = 0;
        }
    }
}

void ble_listener(void *caller, BT_BLE_EVT event, void *data) {
    struct rk_bluetooth *bt = caller;
    BT_BLE_MSG *msg = data;
    struct rokid_ble_handshake handshake;
    struct ble_handle *ble = g_bt_ble;

    memset(&handshake, 0, sizeof(struct rokid_ble_handshake));
    handshake.magic_num[0] = 'R';
    handshake.magic_num[1] = 'K';
    handshake.version = 2;

    switch (event) {
    case BT_BLE_SE_WRITE_EVT:
        if (ble->state.open_state < BLE_STATE_CLOSE) {
            if (msg) {
                char buf[255] = {0};
                int len = msg->ser_write.len > 255 ? 255 : msg->ser_write.len;
                memcpy(buf, msg->ser_write.value + msg->ser_write.offset, len);
                buf[254] = '\0';
                BT_LOGV("BT_BLE_SE_WRITE_EVT : status %d, len: %d, buf: %s\n", msg->ser_write.status, msg->ser_write.len, buf);

                int i = 0;
                BT_LOGI("ser uuid : %04x\n", msg->ser_write.uuid);
                for (i = 0; i < len; i++) {
                    printf("0x%02x ", buf[i]);
                }
                printf("\n");

                if (msg->ser_write.uuid != BLE_DATA_CHARACTER &&
				    msg->ser_write.uuid != BLE_DATA_CHARACTER_CMCC) {
                    BT_LOGV("not right char id\n");
                    break;
                }

                if (msg->ser_write.len > 0) {
                    if (memcmp(&handshake, buf, sizeof(struct rokid_ble_handshake)) == 0) {
                        ble->state.open_state = BLE_STATE_HANDSHAKED;
                        usleep(50 * 1000);
                        bt->ble.send_buf(bt, msg->ser_write.uuid, (char *)&handshake, sizeof(struct rokid_ble_handshake));
                        broadcast_ble_state(NULL);
                    } else {
                        if (msg->ser_write.uuid == BLE_DATA_CHARACTER) {
                            rokid_recv_ble_data((uint8_t *)buf, len);
                        } else if (msg->ser_write.uuid == BLE_DATA_CHARACTER_CMCC) {
                            rokid_recv_ble_data_cmcc((uint8_t *)buf, len);
                        }
                    }

                }
            }
        }

        break;
    case BT_BLE_SE_OPEN_EVT:
        if (msg)
            BT_LOGI("BT_BLE_SE_OPEN_EVT : reason= %d, conn_id %d\n", msg->ser_open.reason, msg->ser_open.conn_id);

        ble->state.open_state = BLE_STATE_CONNECTED;
        broadcast_ble_state(NULL);
        break;
    case BT_BLE_SE_CLOSE_EVT:
        if (msg)
            BT_LOGI("BT_BLE_SE_CLOSE_EVT : reason= %d, conn_id %d\n", msg->ser_close.reason, msg->ser_close.conn_id);

        ble->state.open_state = BLE_STATE_DISCONNECTED;
        broadcast_ble_state(NULL);
        break;
    default:
        break;
    }

    return;
}

static void bt_ble_on(char *name) {
    int ret = 0;
    struct ble_handle *ble = g_bt_ble;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if ((ble->state.open_state == BLE_STATE_OPENED) ||
        (ble->state.open_state == BLE_STATE_CONNECTED) ||
        (ble->state.open_state == BLE_STATE_HANDSHAKED)) {
        broadcast_ble_state(NULL);
        return;
    }
    pthread_mutex_lock(&(ble->state_mutex));

    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        ble->state.open_state = BLE_STATE_OPENFAILED;
        broadcast_ble_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return ;
    }

    bt->ble.disable(bt);

    bt->ble.set_listener(bt, ble_listener, bt);

    bt->set_bt_name(bt, name);

    if (0 != bt->ble.enable(bt)) {
        BT_LOGE("enbale ble failed\n");
        ble->state.open_state = BLE_STATE_OPENFAILED;
        broadcast_ble_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return ;
    }
    g_bt_handle->status |= BLE_STATUS_MASK;

    ret = bt->ble.set_ble_visibility(bt, true, true);
    // if (ret) {
    //     BT_LOGI("set visibility error\n");
    //     return -3;
    // }
    memset(&ble->state, 0, sizeof(struct ble_state));

    ble->state.open_state = BLE_STATE_OPENED;
    broadcast_ble_state(NULL);

    pthread_mutex_unlock(&(ble->state_mutex));

    return ;
}

void bt_ble_off(struct rk_bluetooth *bt) {
    struct ble_handle *ble = g_bt_ble;

    pthread_mutex_lock(&(ble->state_mutex));

    if (ble->state.open_state >= BLE_STATE_CLOSE) {
        broadcast_ble_state(NULL);
        pthread_mutex_unlock(&(ble->state_mutex));
        return ;
    }

    usleep(100 * 1000);

    ble->state.open_state = BLE_STATE_CLOSE;
    bt->ble.disable(bt);
    g_bt_handle->status &= ~BLE_STATUS_MASK;
    bt_close(g_bt_handle);
    ble->state.open_state = BLE_STATE_CLOSED;
    broadcast_ble_state(NULL);
    pthread_mutex_unlock(&(ble->state_mutex));
}

void bt_ble_on_prepare(struct bt_service_handle *handle, json_object *obj) {
    int unique = 0;
    json_object *bt_unique = NULL;

    if (json_object_object_get_ex(obj, "unique", &bt_unique) == TRUE) {
        unique = json_object_get_boolean(bt_unique);
        BT_LOGI("bt_ble :: unique :: %d\n", unique);
        if (unique) {
            bt_close_mask_profile(handle, ~BLE_STATUS_MASK);
        }
    }
}

int handle_ble_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    char *data = NULL;
    json_object *bt_cmd = NULL;
    json_object *bt_data = NULL;

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("bt_ble :: command %s \n", command);

        // g_bt_ble = &handle->bt_ble;
        //ble_state = &g_bt_ble->state;

        if (strcmp(command, "ON") == 0) {
            json_object *bt_name = NULL;
            char *name = NULL;

            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                name = (char *)json_object_get_string(bt_name);
                BT_LOGI("bt_ble :: name %s \n", name);
            } else {
                name = "ROKID-BT-9999zz";
            }

            bt_ble_on_prepare(handle, obj);
            bt_ble_on(name);
        } else if (strcmp(command, "OFF") == 0) {
            bt_ble_off(g_bt_handle->bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_ble_state(reply);
        }
    } else if (json_object_object_get_ex(obj, "data", &bt_data) == TRUE) {
        data = (char *)json_object_get_string(bt_data);
        rokid_send_ble_data(g_bt_handle->bt, (uint8_t *)data, strlen(data));
    } else if (json_object_object_get_ex(obj, "rawdata", &bt_data) == TRUE) {
        data = (char *)json_object_get_string(bt_data);
        ble_send_raw_data(g_bt_handle->bt, (uint8_t *)data, strlen(data));
    }

    return 0;
}

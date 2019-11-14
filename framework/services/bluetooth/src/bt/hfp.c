#if defined(BT_SERVICE_HAVE_HFP)
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <json-c/json.h>
#include <hardware/bt_hfp.h>

#include "turen.h"
#include "common.h"

struct hfp_handle *g_bt_hfp = NULL;

static void broadcast_hfp_state(void *reply) {
    json_object *root = json_object_new_object();
    struct hfp_state *hfp_state = &g_bt_hfp->state;
    char *hfp = NULL;
    char *conn_state = NULL;
    char *conn_name = NULL;
    char *service = NULL;
    char *call = NULL;
    char *setup = NULL;
    char *held = NULL;
    char *audio = NULL;
    char *re_state = NULL;
    char *action = "stateupdate";

    switch (hfp_state->open_state) {
    case (HFP_STATE_OPEN):
        hfp = "open";
        break;
    case (HFP_STATE_OPENED):
        hfp = "opened";
        break;
    case (HFP_STATE_OPENFAILED):
        hfp = "open failed";
        break;
    case (HFP_STATE_CLOSE):
        hfp = "close";
        break;
    case (HFP_STATE_CLOSING):
        hfp = "closing";
        break;
    case (HFP_STATE_CLOSED):
        hfp = "closed";
        break;
    default:
        hfp = "invalid";
        break;
    }

    switch (hfp_state->connect_state) {
    case HFP_STATE_CONNECTING:
        conn_state = "connecting";
        break;
    case HFP_STATE_CONNECTED:
        conn_state = "connected";
        conn_name = g_bt_hfp->dev.name;
        break;
    case HFP_STATE_CONNECTEFAILED:
        conn_state = "connect failed";
        break;
    case HFP_STATE_DISCONNECTING:
        conn_state = "disconnecting";
        break;
    case HFP_STATE_DISCONNECTED:
        conn_state = "disconnected";
        break;
    default:
        conn_state = "invalid";
        break;
    }


    json_object_object_add(root, "hfpstate", json_object_new_string(hfp));
    json_object_object_add(root, "connect_state", json_object_new_string(conn_state));
    if (conn_name) {
        json_object_object_add(root, "connect_name", json_object_new_string(conn_name));
    }

    switch (hfp_state->service) {
    case HFP_SERVICE_INACTIVE:
        service = "inactive";
        break;
    case HFP_SERVICE_ACTIVE:
        service = "active";
        break;
    default:
        service = "invalid";
        break;
    }

    switch (hfp_state->call) {
    case HFP_CALL_INACTIVE:
        call = "inactive";
        break;
    case HFP_CALL_ACTIVE:
        call = "active";
        break;
    default:
        call = "invalid";
        break;
    }

    switch (hfp_state->callsetup) {
    case HFP_CALLSETUP_NONE:
        setup = "none";
        break;
    case HFP_CALLSETUP_INCOMING:
        setup = "incoming";
        break;
    case HFP_CALLSETUP_OUTGOING:
        setup = "outgoing";
        break;
    case HFP_CALLSETUP_ALERTING:
        setup = "alerting";
        break;
    default:
        setup = "invalid";
        break;
    }

    switch (hfp_state->callheld) {
    case HFP_CALLHELD_NONE:
        held = "none";
        break;
    case HFP_CALLHELD_ACTIVE_HELD:
        held = "hold_active";
        break;
    case HFP_CALLHELD_HELD:
        held = "hold";
        break;
    default:
        held = "invalid";
        break;
    }
    json_object_object_add(root, "service", json_object_new_string(service));
    json_object_object_add(root, "call", json_object_new_string(call));
    json_object_object_add(root, "setup", json_object_new_string(setup));
    json_object_object_add(root, "held", json_object_new_string(held));


    switch (hfp_state->audio) {
    case HFP_AUDIO_OFF:
        audio = "off";
        break;
    case HFP_AUDIO_ON:
        audio = "on";
        break;
    default:
        audio = "invalid";
        break;
    }
    json_object_object_add(root, "audio", json_object_new_string(audio));

    json_object_object_add(root, "action", json_object_new_string(action));

    re_state = (char *)json_object_to_json_string(root);
    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_HFP, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_HFP, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}

static void broadcast_hfp_ring(void) {
    json_object *root = json_object_new_object();
    struct hfp_state *hfp_state = &g_bt_hfp->state;
    char *re_state = NULL;
    char *action = "ring";
    char *audio = NULL;

    json_object_object_add(root, "action", json_object_new_string(action));
    if (hfp_state->audio == HFP_AUDIO_ON)
        audio = "on";
    else
        audio = "off";
    json_object_object_add(root, "audio", json_object_new_string(audio));

    re_state = (char *)json_object_to_json_string(root);
    report_bluetooth_information(BLUETOOTH_FUNCTION_HFP, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}

static void hfp_listener (void *caller, BT_HS_EVT event, void *data)
{
    //struct rk_bluetooth *bt = caller;
    BT_HFP_MSG *msg = data;
    struct hfp_state *hfp_state = &g_bt_hfp->state;

    switch (event) {
        case BT_HS_CONN_EVT:
            if (msg) {
                BT_LOGI("BT_HS_CONN_EVT : status= %d, idx %d, name: %s, addr %02X:%02X:%02X:%02X:%02X:%02X\n",
                    msg->conn.status, msg->conn.idx, msg->conn.name,
                    msg->conn.bd_addr[0], msg->conn.bd_addr[1],
                    msg->conn.bd_addr[2], msg->conn.bd_addr[3],
                    msg->conn.bd_addr[4], msg->conn.bd_addr[5]);
                if (msg->conn.status == 0) {
                    memcpy(g_bt_hfp->dev.addr, msg->conn.bd_addr, sizeof(g_bt_hfp->dev.addr));
                    snprintf(g_bt_hfp->dev.name, sizeof(g_bt_hfp->dev.name), "%s", msg->conn.name);
                    hfp_state->connect_state = HFP_STATE_CONNECTED;
                    broadcast_hfp_state(NULL);
                } else {
                    hfp_state->connect_state = HFP_STATE_CONNECTEFAILED;
                    broadcast_hfp_state(NULL);
                }
            }
            break;
        case BT_HS_CLOSE_EVT:
            if (msg)
                BT_LOGI("BT_HS_CLOSE_EVT : status= %d, idx %d\n", msg->close.status, msg->close.idx);
            turen_close();
            hfp_state->connect_state = HFP_STATE_DISCONNECTED;
            broadcast_hfp_state(NULL);
            break;
        case BT_HS_AUDIO_OPEN_EVT:
            BT_LOGD("BT_HS_AUDIO_OPEN_EVT\n");
            if (hfp_state->call == HFP_CALL_ACTIVE) {
                turen_open();
            }
            hfp_state->audio = HFP_AUDIO_ON;
            broadcast_hfp_state(NULL);
            break;
        case BT_HS_AUDIO_CLOSE_EVT:
            BT_LOGD("BT_HS_AUDIO_CLOSE_EVT\n");
            hfp_state->audio = HFP_AUDIO_OFF;
            turen_close();
            broadcast_hfp_state(NULL);
            break;
        case BT_HS_FEED_MIC_EVT:
            if (msg) {
                //BT_LOGV("BT_HS_FEED_MIC_EVT : need len=%d, bits=%d, channel =%d, rate=%d\n",
                //    msg->feed_mic.need_len, msg->feed_mic.bits, msg->feed_mic.channel, msg->feed_mic.rate);
                /*todo*/
                msg->feed_mic.get_len = turen_recv_data(msg->feed_mic.data, msg->feed_mic.need_len);
                if (msg->feed_mic.get_len == 0) {
                    if (hfp_state->call == HFP_CALL_ACTIVE && hfp_state->audio == HFP_AUDIO_ON) {
                        BT_LOGE("turen maybe crash,so we should re-connect turen!!\n");
                        turen_close();
                        turen_open();
                    }
                }
            }
            break;
        case BT_HS_CIND_EVT:
            BT_LOGI("BT_HS_CIND_EVT\n");
            if (msg) {
                hfp_state->service = msg->cind.status.curr_service_ind;
                hfp_state->call = msg->cind.status.curr_call_ind;
                hfp_state->callsetup = msg->cind.status.curr_call_setup_ind;
                hfp_state->callheld = msg->cind.status.curr_callheld_ind;
                if (hfp_state->call == HFP_CALL_ACTIVE && hfp_state->audio == HFP_AUDIO_ON) {
                    turen_open();
                }
                broadcast_hfp_state(NULL);
            }
            break;
        case BT_HS_CIEV_EVT:
            if (msg) {
                BT_LOGI("BT_HS_CIEV_EVT : CIEV= %d\n", msg->ciev.status);
                switch (msg->ciev.status) {
                    case BT_HS_SERVICE_OFF:
                        hfp_state->service = HFP_SERVICE_INACTIVE;
                        break;
                    case BT_HS_SERVICE_ON:
                        hfp_state->service = HFP_SERVICE_ACTIVE;
                        break;
                    case BT_HS_CALL_OFF:
                        hfp_state->call = HFP_CALL_INACTIVE;
                        break;
                    case BT_HS_CALL_ON:
                        hfp_state->call = HFP_CALL_ACTIVE;
                        break;
                    case BT_HS_CALLSETUP_DONE:
                        hfp_state->callsetup = HFP_CALLSETUP_NONE;
                        break;
                    case BT_HS_CALLSETUP_INCOMING:
                        hfp_state->callsetup = HFP_CALLSETUP_INCOMING;
                        break;
                    case BT_HS_CALLSETUP_OUTGOING:
                        hfp_state->callsetup = HFP_CALLSETUP_OUTGOING;
                        break;
                    case BT_HS_CALLSETUP_REMOTE_ALERTED:
                        hfp_state->callsetup = HFP_CALLSETUP_ALERTING;
                        break;
                    case BT_HS_CALLHELD_NONE:
                        hfp_state->callheld= HFP_CALLHELD_NONE;
                        break;
                    case BT_HS_CALLHELD_HOLD_WITH_ACTIVE:
                        hfp_state->callheld = HFP_CALLHELD_ACTIVE_HELD;
                     case BT_HS_CALLHELD_HOLD:
                        hfp_state->callheld = HFP_CALLHELD_HELD;
                        break;
                     default:
                        return;//not care
                        break;
                }
                if (hfp_state->call == HFP_CALL_ACTIVE && hfp_state->audio == HFP_AUDIO_ON) {
                    turen_open();
                }
                broadcast_hfp_state(NULL);
            }
            break;
        case BT_HS_RING_EVT:
            BT_LOGI("BT_HS_RING_EVT\n");
            broadcast_hfp_ring();
            break;
        case BT_HS_CHLD_EVT:
            BT_LOGI("BT_HS_CHLD_EVT\n");
            break;
        case BT_HS_OK_EVT:
            BT_LOGI("BT_HS_OK_EVT\n");
            break;
        case BT_HS_ERROR_EVT:
            BT_LOGI("BT_HS_ERROR_EVT\n");
            break;
        default:
            break;
    }
    return;
}

static int bt_hfp_enable(struct rk_bluetooth *bt, const char *name) {
    int ret = 0;

    ret = bt->hfp.set_listener(bt, hfp_listener, bt);
    if (ret) {
        BT_LOGE("set ble listener error :: %d\n", ret);
        return -2;
    }
    if (name)
        bt->set_bt_name(bt, name);

    if (bt->hfp.enable(bt)) {
        BT_LOGE("enable hfp failed!\n");
        return -2;
    }

    return 0;
}

void bt_hfp_on(const char *name) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_handle *a2dpsink = g_bt_a2dpsink;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;

    if (hfp->state.open_state == HFP_STATE_OPENED) {
        broadcast_hfp_state(NULL);
        return;
    }
    if (bt->hfp.type == BT_HS_USE_UART) {//not support uart fro hfp data
        BT_LOGE("hfp UART not support!!!!!");
        hfp->state.open_state = HFP_STATE_OPENFAILED;
        broadcast_hfp_state(NULL);
        return;
    }
    pthread_mutex_lock(&(hfp->state_mutex));
    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        hfp->state.open_state = BLE_STATE_OPENFAILED;
        broadcast_hfp_state(NULL);
        pthread_mutex_unlock(&(hfp->state_mutex));
        return ;
    }
    //memset(&hfp->state, 0, sizeof(struct hfp_state));
    ret = bt_hfp_enable(bt, name);
    if (ret == 0) {
        hfp->state.open_state = HFP_STATE_OPENED;
        g_bt_handle->status |= HFP_STATUS_MASK;
    } else {
        hfp->state.open_state = HFP_STATE_OPENFAILED;
    }
    if (ret == 0 && a2dpsink->state.open_state == A2DP_SINK_STATE_OPENED  &&
        a2dpsink->state.connect_state == A2DP_SINK_STATE_CONNECTED) {
        bt_hfp_connect(a2dpsink_config->dev[0].addr);
    } else {
        broadcast_hfp_state(NULL);
    }
    pthread_mutex_unlock(&(hfp->state_mutex));

    return ;
}

void bt_hfp_off(struct rk_bluetooth *bt) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;

    if (hfp->state.open_state == HFP_STATE_CLOSED) {
        broadcast_hfp_state(NULL);
        return;
    }
    pthread_mutex_lock(&(hfp->state_mutex));
    ret = bt->hfp.disable(bt);
    if (ret) {
        BT_LOGE("hfp OFF error");
    }
    g_bt_handle->status &= ~HFP_STATUS_MASK;
    bt_close(g_bt_handle);
    memset(&hfp->state, 0, sizeof(struct hfp_state));
    hfp->state.open_state = HFP_STATE_CLOSED;
    turen_close();
    broadcast_hfp_state(NULL);
    pthread_mutex_unlock(&(hfp->state_mutex));
    return ;
}



static void bt_hfp_answer_call(void) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (hfp->state.connect_state == HFP_STATE_CONNECTED) {
        ret = bt->hfp.answercall(bt);
        if (ret) {
            BT_LOGE("failed");
        }
    }

    return ;
}

static void bt_hfp_hangup_call(void) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (hfp->state.connect_state == HFP_STATE_CONNECTED) {
        ret = bt->hfp.hangup(bt);
        if (ret) {
            BT_LOGE("failed");
        }
    }
    return ;
}

static void bt_hfp_dialing_call(const char *num) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (hfp->state.connect_state == HFP_STATE_CONNECTED) {
        ret = bt->hfp.dial_num(bt, num);
        if (ret) {
            BT_LOGE("failed");
        }
    }
    return ;
}

void bt_hfp_connect(BTAddr bd_addr) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    hfp->state.connect_state = HFP_STATE_CONNECTEFAILED;

    if (hfp->state.open_state == HFP_STATE_OPENED) {
        ret = bt->hfp.connect(bt, bd_addr);
        if (ret) {
            BT_LOGE("failed");
        } else {
            hfp->state.connect_state = HFP_STATE_CONNECTING;
        }
    }

    broadcast_hfp_state(NULL);
    return ;
}


void bt_hfp_disconnect_device(void) {
    int ret = 0;
    struct hfp_handle *hfp = g_bt_hfp;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct hfp_state *hfp_state = &hfp->state;

    if (hfp_state->connect_state == HFP_STATE_CONNECTED) {
        BT_LOGI("disconnect  %s, bdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                g_bt_hfp->dev.name,
                g_bt_hfp->dev.addr[0],
                g_bt_hfp->dev.addr[1],
                g_bt_hfp->dev.addr[2],
                g_bt_hfp->dev.addr[3],
                g_bt_hfp->dev.addr[4],
                g_bt_hfp->dev.addr[5]);
        ret = bt->hfp.disconnect(bt);
        if (ret) {
            BT_LOGE("hfp DISCONNECT error");
        } else {
            hfp_state->connect_state = HFP_STATE_DISCONNECTING;
        }
    }
}

int handle_hfp_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    json_object *bt_cmd = NULL;

    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("bt_hfp :: command %s \n", command);

        if (strcmp(command, "ON") == 0) {
            char *name = NULL;
            json_object *bt_name = NULL;
            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                name = (char *)json_object_get_string(bt_name);
                BT_LOGI("hfp :: name %s \n", name);
            }
            bt_hfp_on(name);
        } else if (strcmp(command, "OFF") == 0) {
            bt_hfp_off(handle->bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_hfp_state(reply);
        } else if (strcmp(command, "ANSWERCALL") == 0) {
            bt_hfp_answer_call();
        } else if (strcmp(command, "HANGUP") == 0) {
            bt_hfp_hangup_call();
        } else if (strcmp(command, "DISCONNECT") == 0) {
            bt_hfp_disconnect_device();
        } else if (strcmp(command, "DISCONNECT_PEER") == 0) {
            bt_hfp_disconnect_device();
        } else if (strcmp(command, "DIALING") == 0) {
            char *num = NULL;
            json_object *bt_num = NULL;
            if (json_object_object_get_ex(obj, "NUMBER", &bt_num) == TRUE) {
                num = (char *)json_object_get_string(bt_num);
                BT_LOGI("num %s \n", num);
                bt_hfp_dialing_call(num);
            }
        }
    }

    return 0;
}
#endif

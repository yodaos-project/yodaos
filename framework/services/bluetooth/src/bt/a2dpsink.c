#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>

#include <cutils/properties.h>
#include <json-c/json.h>
#include <hardware/bt_a2dp_sink.h>

#include "common.h"

struct a2dpsink_handle *g_bt_a2dpsink = NULL;

static int bt_a2dpsink_enable(struct rk_bluetooth *bt, char *name, bool discoverable);
static void bt_a2dp_sink_play(void *eloop_ctx);
static void handle_a2dpsink_connect_subsequent();

static int get_a2dpsink_connected_device_num() {
    BTDevice device;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    memset(&device, 0, sizeof(device));

    return bt->a2dp_sink.get_connected_devices(bt, &device, 1);
}

static int64_t get_current_micro_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t ts = (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000;

    return ts;
}

static void auto_connect_by_history_index(void *eloop_ctx) {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;
    char zero_addr[6] = {0};
    int ret;

    if (get_a2dpsink_connected_device_num()) {
        a2dpsink_config->autoconnect_index = 0;
        a2dpsink_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_OVER;
        eloop_timer_stop(a2dpsink_timer->e_auto_connect_id);
        return;
    }
    if (a2dpsink_config->autoconnect_flag != AUTOCONNECT_BY_HISTORY_WORK) {
        a2dpsink_config->autoconnect_index = 0;
        return ;
    }

    BT_LOGI("autoconnect index ;:; %d\n", a2dpsink_config->autoconnect_index);
    if (a2dpsink_config->autoconnect_index >= 0) {
        if (a2dpsink_config->autoconnect_index < a2dpsink_config->autoconnect_num) {
            BT_LOGI("autoconnect  %02X:%02X:%02X:%02X:%02X:%02X\n",
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[0],
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[1],
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[2],
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[3],
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[4],
                     a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr[5]);
            BT_LOGI("autoconnect name :: %s\n", a2dpsink_config->dev[a2dpsink_config->autoconnect_index].name);
            if (memcmp(a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr, zero_addr, sizeof(zero_addr)) != 0) {
                ret = bt->a2dp_sink.connect(bt, a2dpsink_config->dev[a2dpsink_config->autoconnect_index].addr);
                if (ret) {
                    BT_LOGI("autoconnect failed :: %d\n", ret);//busy  auto connect next
                    a2dpsink_config->autoconnect_index ++;
                    eloop_timer_start(a2dpsink_timer->e_auto_connect_id);
                    return;
                } else
                    g_bt_a2dpsink->state.connect_state = A2DP_SINK_STATE_CONNECTING;
            }
        } else {
            a2dpsink_config->autoconnect_index = 0;
            a2dpsink_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_OVER;
            eloop_timer_stop(a2dpsink_timer->e_auto_connect_id);
            return ;
        }
    }

    a2dpsink_config->autoconnect_index++;
}

void auto_connect_by_scan_results(struct rk_bluetooth *bt) {
    BTDevice scan_devices[20];
    BTDevice history_devices[20];
    int scan_devices_num = 0;
    int history_devices_num = 0;
    int i = 0;
    int j = 0;
    int ret = 0;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;

    if (a2dpsink_state->open_state != A2DP_SINK_STATE_OPENED) {
        return ;
    }
    if (get_a2dpsink_connected_device_num()) {
        return;
    }
    if (a2dpsink_config->autoconnect_mode != ROKIDOS_BT_AUTOCONNECT_BY_SCANRESULTS) {
        return ;
    }
    memset(scan_devices, 0, sizeof(scan_devices));
    memset(history_devices, 0, sizeof(history_devices));

    scan_devices_num = bt->get_disc_devices(bt, scan_devices, 20);
    BT_LOGI("num %d\n", scan_devices_num);

    history_devices_num = bt->get_bonded_devices(bt, history_devices, 20);
    BT_LOGI("history num %d\n", history_devices_num);

    for (i = 0; i < scan_devices_num; i++) {
        BT_LOGV("dev:%d\n", i);
        BT_LOGV("\tbdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                 scan_devices[i].bd_addr[0],
                 scan_devices[i].bd_addr[1],
                 scan_devices[i].bd_addr[2],
                 scan_devices[i].bd_addr[3],
                 scan_devices[i].bd_addr[4],
                 scan_devices[i].bd_addr[5]);
        BT_LOGV("\tname:%s\n", scan_devices[i].name);
        BT_LOGV("\trssi:%d\n", scan_devices[i].rssi);

        for (j = 0; j < history_devices_num; j++) {
            BT_LOGI("\t history name:%s\n", history_devices[j].name);
            if (memcmp(scan_devices[i].bd_addr, history_devices[j].bd_addr, sizeof(scan_devices[i].bd_addr)) == 0) {
                if (bt->a2dp_sink.connect) {
                    ret = bt->a2dp_sink.connect(bt, scan_devices[i].bd_addr);
                    if (ret == 0) {
                        a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECTING;
                        return;
                    } else {
                        continue;
                    }
                }
            }
        }
    }
}

static void bt_a2dp_sink_play(void *eloop_ctx) {
    int is_playing = 0;
    static int index = 0;
    int64_t current_diff_time = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    if (eloop_ctx == NULL) {
        index = 0;
    }

    if (!get_a2dpsink_connected_device_num()) {
        return;
    }

    bt->a2dp_sink.send_get_playstatus(bt);
    is_playing = bt->a2dp_sink.get_playing(bt);
    BT_LOGI("is_playing is %d, get_play_status %d, index %d\n", is_playing, g_bt_a2dpsink->get_play_status, index);

    current_diff_time = get_current_micro_time() - g_bt_a2dpsink->last_stop_time;

    //avoid continuous recv pause and play in a little time
    if (current_diff_time < 2000 &&  is_playing) {
        eloop_timer_stop(a2dpsink_timer->e_play_command_id);
        eloop_timer_start(a2dpsink_timer->e_play_command_id);
        return ;
    }
    if (is_playing && g_bt_a2dpsink->get_play_status == 1) {
        eloop_timer_stop(a2dpsink_timer->e_play_command_id);
        return;
    }
    if (index < BLUETOOTH_A2DPSINK_CHECK_PLAY_INDEX) {
        if (is_playing && g_bt_a2dpsink->get_play_status == 1) {
            index++;
        } else {
            bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_PLAY);
            index++;
        }
    } else {
        eloop_timer_stop(a2dpsink_timer->e_play_command_id);
        return;
    }

    eloop_timer_start(a2dpsink_timer->e_play_command_id);
}

static void bt_a2dp_sink_stop(void *eloop_ctx) {
    int is_playing = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    eloop_timer_stop(a2dpsink_timer->e_play_command_id);

    if (!get_a2dpsink_connected_device_num()) {
        return;
    }

    is_playing = bt->a2dp_sink.get_playing(bt);
    BT_LOGV("current state is %d\n", is_playing);
    if (!is_playing) {
        return;
    }

    bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_STOP);
}

static void bt_a2dp_sink_pause(void *eloop_ctx) {
    int is_playing = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    eloop_timer_stop(a2dpsink_timer->e_play_command_id);

    if (!get_a2dpsink_connected_device_num()) {
        return;
    }

    is_playing = bt->a2dp_sink.get_playing(bt);
    BT_LOGV("current state is %d\n", is_playing);
    if (!is_playing) {
        return;
    }

    bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_PAUSE);
}

void broadcast_a2dpsink_state(void *reply) {
    json_object *root = json_object_new_object();
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;
    char *a2dpsink = NULL;
    char *conn_state = NULL;
    char *conn_name = NULL;
    char conn_addr[18] = {0};
    char *play_state = NULL;
    char *broadcast_state = NULL;
    char *re_state = NULL;
    char *action = "stateupdate";

    switch (a2dpsink_state->open_state) {
    case (A2DP_SINK_STATE_OPEN): {
        a2dpsink = "open";
        break;
    }
    case (A2DP_SINK_STATE_OPENED): {
        a2dpsink = "opened";
        break;
    }
    case (A2DP_SINK_STATE_OPENFAILED): {
        a2dpsink = "open failed";
        break;
    }
    case (A2DP_SINK_STATE_CLOSE): {
        a2dpsink = "close";
        break;
    }
    case (A2DP_SINK_STATE_CLOSING): {
        a2dpsink = "closing";
        break;
    }
    case (A2DP_SINK_STATE_CLOSED): {
        a2dpsink = "closed";
        break;
    }
    default:
        a2dpsink = "invalid";
        break;
    }

    switch (a2dpsink_state->connect_state) {
    case (A2DP_SINK_STATE_CONNECTED): {
        conn_state = "connected";
        conn_name = a2dpsink_config->dev[0].name;
        sprintf(conn_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
                    a2dpsink_config->dev[0].addr[0],
                    a2dpsink_config->dev[0].addr[1],
                    a2dpsink_config->dev[0].addr[2],
                    a2dpsink_config->dev[0].addr[3],
                    a2dpsink_config->dev[0].addr[4],
                    a2dpsink_config->dev[0].addr[5]);
        break;
    }
    case (A2DP_SINK_STATE_CONNECTEFAILED): {
        conn_state = "connect failed";
        break;
    }
    case (A2DP_SINK_STATE_DISCONNECTED): {
        conn_state = "disconnected";
        break;
    }
    default:
        conn_state = "invalid";
        break;
    }

    switch (a2dpsink_state->play_state) {
    case (A2DP_SINK_STATE_PLAYED): {
        play_state = "played";
        break;
    }
    case (A2DP_SINK_STATE_STOPED): {
        play_state = "stopped";
        break;
    }
    default:
        play_state = "invalid";
        break;
    }

    switch (a2dpsink_state->broadcast_state) {
    case (A2DP_SINK_BROADCAST_OPENED): {
        broadcast_state = "opened";
        break;
    }
    case (A2DP_SINK_BROADCAST_CLOSED): {
        broadcast_state = "closed";
        break;
    }
    default:
        break;
    }

    json_object_object_add(root, "linknum", json_object_new_int(a2dpsink_config->autoconnect_linknum));

    if (a2dpsink) {
        json_object_object_add(root, "a2dpstate", json_object_new_string(a2dpsink));
    }

    if (conn_state) {
        json_object_object_add(root, "connect_state", json_object_new_string(conn_state));
        if (conn_name) {
            json_object_object_add(root, "connect_name", json_object_new_string(conn_name));
            json_object_object_add(root, "connect_address", json_object_new_string(conn_addr));
        }
    }

    if (play_state) {
        json_object_object_add(root, "play_state", json_object_new_string(play_state));
    }

    if (broadcast_state) {
        json_object_object_add(root, "broadcast_state", json_object_new_string(broadcast_state));
    }

    json_object_object_add(root, "action", json_object_new_string(action));

    re_state = (char *)json_object_to_json_string(root);
    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_A2DPSINK, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSINK, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);
}

static void broadcast_a2dpsink_element_attrs(BT_AVK_GET_ELEMENT_ATTR_MSG elem_attr) {
    json_object *root = json_object_new_object();
    int i;
    char *data = NULL;
    char *action = "element_attrs";

    json_object_object_add(root, "action", json_object_new_string(action));
    for (i = 0; i < elem_attr.num_attr; i++) {
        switch (elem_attr.attr_entry[i].attr_id) {
            case BT_AVRC_MEDIA_ATTR_ID_TITLE:
                json_object_object_add(root, "title", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_ARTIST:
                json_object_object_add(root, "artist", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_ALBUM:
                json_object_object_add(root, "album", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_TRACK_NUM:
                json_object_object_add(root, "track_num", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_NUM_TRACKS:
                json_object_object_add(root, "track_nums", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_GENRE:
                json_object_object_add(root, "genre", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            case BT_AVRC_MEDIA_ATTR_ID_PLAYING_TIME:
                json_object_object_add(root, "time", json_object_new_string((const char *)elem_attr.attr_entry[i].data));
                break;
            default:
                break;
        }
    }

    data = (char *)json_object_to_json_string(root);
    report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSINK, (uint8_t *)data, strlen(data));

    json_object_put(root);
}

static void bt_a2dpsink_playstate_check(void *args) {
    int now_playing = 0;
    int last_playing = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;

    switch (a2dpsink_state->play_state) {
    case A2DP_SINK_STATE_PLAY_INVALID:
        last_playing = 0;
        break;
    case A2DP_SINK_STATE_PLAYED:
        last_playing = 1;
        break;
    case A2DP_SINK_STATE_STOPED:
        last_playing = 0;
        break;
    default:
        last_playing = 0;
        break;
    }

    // judge is played
    now_playing = bt->a2dp_sink.get_playing(bt);
    if (last_playing != now_playing) {
        if (now_playing) {
            a2dpsink_state->play_state = A2DP_SINK_STATE_PLAYED;
            broadcast_a2dpsink_state(NULL);

            property_set("audio.bluetooth.playing", "true");
        } else {
            a2dpsink_state->play_state = A2DP_SINK_STATE_STOPED;
            broadcast_a2dpsink_state(NULL);

            property_set("audio.bluetooth.playing", "false");
        }
    }

    return;
}

static void bluetooth_autoconnect() {
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    if (get_a2dpsink_connected_device_num()) {
        a2dpsink_config->autoconnect_index = 0;
        return;
    }
    if (a2dpsink_config->autoconnect_mode == ROKIDOS_BT_AUTOCONNECT_BY_SCANRESULTS) {
        bt_discovery(g_bt_handle, BT_A2DP_SOURCE);
    } else if (a2dpsink_config->autoconnect_mode == ROKIDOS_BT_AUTOCONNECT_BY_HISTORY) {
        a2dpsink_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_WORK;
        a2dpsink_config->autoconnect_index = 0;
        eloop_timer_start(a2dpsink_timer->e_auto_connect_id);
    }

    BT_LOGI("begin autoconnect\n");
}

#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
static void a2dpsink_autoconnect_over(void *eloop_ctx) {

    if (get_a2dpsink_connected_device_num()) {
        return;
    } else { 
        bt_close_mask_profile(g_bt_handle, A2DP_SINK_STATUS_MASK | HFP_STATUS_MASK);
    }
}
#endif

static void bt_a2dpsink_on(char *name, bool auto_conn, bool discoverable) {
    int ret = 0;
    struct a2dpsink_handle *a2dpsink = g_bt_a2dpsink;
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (a2dpsink->state.open_state == A2DP_SINK_STATE_OPENED) {
        if (a2dpsink->state.connect_state == A2DP_SINK_STATE_CONNECTED) {
            broadcast_a2dpsink_state(NULL);
            handle_a2dpsink_connect_subsequent();
        } else {
            bt->set_visibility(bt, discoverable, true);
            a2dpsink->state.discoverable = discoverable;
            a2dpsink->state.broadcast_state = discoverable? A2DP_SINK_BROADCAST_OPENED : A2DP_SINK_BROADCAST_CLOSED;
            broadcast_a2dpsink_state(NULL);
            if (auto_conn)
                bluetooth_autoconnect();
        }
        return;
    }
    pthread_mutex_lock(&(a2dpsink->state_mutex));

    ret = bt_open(g_bt_handle, NULL);
    if (ret) {
        BT_LOGE("failed to open bt\n");
        a2dpsink->state.open_state = A2DP_SINK_STATE_OPENFAILED;
        broadcast_a2dpsink_state(NULL);
        pthread_mutex_unlock(&(a2dpsink->state_mutex));
        return ;
    }

    memset(&a2dpsink->state, 0, sizeof(struct a2dpsink_state));
    a2dpsink->state.discoverable = discoverable;
    ret = bt_a2dpsink_enable(bt, name, discoverable);
    if (ret == 0) {
        a2dpsink->state.open_state = A2DP_SINK_STATE_OPENED;
        g_bt_handle->status |= A2DP_SINK_STATUS_MASK;
    } else {
        a2dpsink->state.open_state = A2DP_SINK_STATE_OPENFAILED;
        broadcast_a2dpsink_state(NULL);
        pthread_mutex_unlock(&(a2dpsink->state_mutex));
        return ;
    }

    broadcast_a2dpsink_state(NULL);

    a2dpsink->config.autoconnect_flag = AUTOCONNECT_BY_HISTORY_INIT;

    if (auto_conn)
        bluetooth_autoconnect();
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
    eloop_timer_start(a2dpsink->timer.e_connect_over_id);
#endif
    pthread_mutex_unlock(&(a2dpsink->state_mutex));

    return ;
}

void bt_a2dpsink_off(struct rk_bluetooth *bt) {
    int ret = 0;
    struct a2dpsink_handle *a2dpsink = g_bt_a2dpsink;

    if (a2dpsink->state.open_state == A2DP_SINK_STATE_CLOSED) {
        broadcast_a2dpsink_state(NULL);
        return;
    }
    pthread_mutex_lock(&(a2dpsink->state_mutex));

    if (a2dpsink->config.autoconnect_mode == ROKIDOS_BT_AUTOCONNECT_BY_SCANRESULTS) {
        bt->cancel_discovery(bt);
    }

    ret = bt->a2dp_sink.disable(bt);
    if (ret) {
        BT_LOGE("a2dpsink OFF error\n");
    }

    g_bt_handle->status &= ~A2DP_SINK_STATUS_MASK;
    bt_close(g_bt_handle);

    eloop_timer_stop(a2dpsink->timer.e_play_state_id);
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
    eloop_timer_stop(a2dpsink->timer.e_connect_over_id);
#endif
    eloop_timer_stop(a2dpsink->timer.e_auto_connect_id);
    eloop_timer_stop(a2dpsink->timer.e_play_command_id);
    a2dpsink->get_play_status = BT_AVRC_PLAYSTATE_STOPPED;

    memset(&a2dpsink->state, 0, sizeof(struct a2dpsink_state));
    a2dpsink->state.broadcast_state = A2DP_SINK_BROADCAST_CLOSED;
    a2dpsink->state.open_state = A2DP_SINK_STATE_CLOSED;
    broadcast_a2dpsink_state(NULL);

    property_set("audio.bluetooth.playing", "false");
    pthread_mutex_unlock(&(a2dpsink->state_mutex));
    return ;
}

static void bt_a2dpsink_disconnect_device(struct a2dpsink_handle *a2dpsink) {
    BTDevice device;
    int count = 0;
    int ret = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_state *a2dpsink_state = &a2dpsink->state;

    memset(&device, 0, sizeof(device));

    count = bt->a2dp_sink.get_connected_devices(bt, &device, 1);
    if (count) {
        BT_LOGI("disconnect bdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                 device.bd_addr[0],
                 device.bd_addr[1],
                 device.bd_addr[2],
                 device.bd_addr[3],
                 device.bd_addr[4],
                 device.bd_addr[5]);
        ret = bt->a2dp_sink.disconnect(bt, device.bd_addr);
        if (ret) {
            BT_LOGE("a2dpsink DISCONNECT error\n");
        }
    } else {
        a2dpsink_state->connect_state = A2DP_SINK_STATE_DISCONNECTED;
        broadcast_a2dpsink_state(NULL);
    }
}

void broadcast_a2dpsink_volume(int volume) {
    json_object *root = json_object_new_object();
    char *data = NULL;
    char *action = "volumechange";

    if (volume < 0) {
        volume = 0;
    } else if (volume > 0x7f) {
        volume = 100;
    } else {
        volume = (volume * 100) / 127;
    }

    json_object_object_add(root, "action", json_object_new_string(action));
    json_object_object_add(root, "value", json_object_new_int(volume));
    data = (char *)json_object_to_json_string(root);
    report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSINK, (uint8_t *)data, strlen(data));

    json_object_put(root);
}

void broadcast_a2dpsink_init_volume() {
    json_object *root = json_object_new_object();
    char *data = NULL;
    char *action = "volumechange";

    json_object_object_add(root, "action", json_object_new_string(action));
    data = (char *)json_object_to_json_string(root);
    report_bluetooth_information(BLUETOOTH_FUNCTION_A2DPSINK, (uint8_t *)data, strlen(data));

    json_object_put(root);
}

static void handle_a2dpsink_connect_subsequent() {
    struct a2dpsink_handle *a2dpsink = g_bt_a2dpsink;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    if (a2dpsink == NULL) {
        return;
    }
    if (!get_a2dpsink_connected_device_num()) {
        return;
    }
    switch (a2dpsink->subsequent) {
    case A2DP_SINK_COMMAND_PLAY: {
        eloop_timer_stop(a2dpsink_timer->e_play_command_id);
        bt_a2dp_sink_play(NULL);
        break;
    }
    case A2DP_SINK_COMMAND_PAUSE: {
        bt_a2dp_sink_pause(NULL);
        break;
    }
    case A2DP_SINK_COMMAND_STOP: {
        bt_a2dp_sink_stop(NULL);
        break;
    }
    default:
        break;
    }

    a2dpsink->subsequent = 0;

    return;
}

void a2dp_sink_listener(void *caller, BT_A2DP_SINK_EVT event, void *data) {
    struct rk_bluetooth *bt = caller;
    BT_AVK_MSG *msg = data;
    struct bt_autoconnect_device dev ;
    int i = 0;
    int index = 0;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;

    memset(&dev, 0, sizeof(struct bt_autoconnect_device));

    switch (event) {
    case BT_AVK_OPEN_EVT: {
        if (msg) {
            BT_LOGI("BT_AVK_OPEN_EVT : status= %d, idx %d, name: %s , addr %02x:%02x:%02x:%02x:%02x:%02x\n",
                     msg->sig_chnl_open.status, msg->sig_chnl_open.idx, msg->sig_chnl_open.name,
                     msg->sig_chnl_open.bd_addr[0], msg->sig_chnl_open.bd_addr[1],
                     msg->sig_chnl_open.bd_addr[2], msg->sig_chnl_open.bd_addr[3],
                     msg->sig_chnl_open.bd_addr[4], msg->sig_chnl_open.bd_addr[5]);
        }
        if (msg->sig_chnl_open.status == 0) {
        } else {
            if (a2dpsink_config->autoconnect_flag == AUTOCONNECT_BY_HISTORY_WORK) {
                eloop_timer_start(a2dpsink_timer->e_auto_connect_id);
                for (i = 0; i < a2dpsink_config->autoconnect_num; i++) {
                    if (memcmp(a2dpsink_config->dev[i].addr, msg->sig_chnl_open.bd_addr, sizeof(msg->sig_chnl_open.bd_addr)) == 0) {
                        a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECT_INVALID;//in auto connecting ignorge connect failed
                        return ;
                    }
                }
            }

            a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECTEFAILED;
            broadcast_a2dpsink_state(NULL);
        }

        break;
    }
    case BT_AVK_STR_OPEN_EVT:
        if (msg->stream_chnl_open.status == 0) {
    #if defined(BT_SERVICE_HAVE_HFP)
            struct hfp_handle *hfp = g_bt_hfp;
            if (hfp->state.open_state == HFP_STATE_OPENED &&
                (hfp->state.connect_state != HFP_STATE_CONNECTED)) {
                bt_hfp_connect(msg->stream_chnl_open.bd_addr);
            }
     #endif
            memcpy(dev.addr, msg->stream_chnl_open.bd_addr, sizeof(dev.addr));

            index = bt_autoconnect_get_index(a2dpsink_config, &dev);
            if (index >= 0) {
                strncpy(dev.name, a2dpsink_config->dev[index].name, strlen(a2dpsink_config->dev[index].name));
            }

            //BT_LOGV("old name :: %s\n", dev.name);

            if (strlen(msg->stream_chnl_open.name) != 0) {
                //BT_LOGV("connect name :; %s\n", msg->stream_chnl_open.name);
                memset(dev.name, 0, sizeof(dev.name));
                strncpy(dev.name, msg->stream_chnl_open.name, strlen(msg->stream_chnl_open.name));
            }

            if (a2dpsink_config->autoconnect_mode == ROKIDOS_BT_AUTOCONNECT_BY_SCANRESULTS) {
                bt->cancel_discovery(bt);
            }
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
            eloop_timer_stop(a2dpsink_timer->e_connect_over_id);
#endif
            bt_autoconnect_update(a2dpsink_config, &dev);
            bt_autoconnect_sync(a2dpsink_config);

            if (bt->set_visibility(bt, false, false)) {
                BT_LOGI("set visibility error\n");
            }

            if (a2dpsink_state->open_state != A2DP_SINK_STATE_CLOSED) {
                a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECTED;
                a2dpsink_state->play_state = A2DP_SINK_STATE_PLAY_INVALID;
                a2dpsink_state->broadcast_state = A2DP_SINK_BROADCAST_CLOSED;
                broadcast_a2dpsink_state(NULL);
            }
            if (a2dpsink_state->rc_open) {
                handle_a2dpsink_connect_subsequent();
            }

        }
        break;
    case BT_AVK_CLOSE_EVT: {
        if (msg)
            BT_LOGI("BT_AVK_CLOSE_EVT : status= %d, idx %d\n", msg->sig_chnl_close.status, msg->sig_chnl_close.idx);

        eloop_timer_stop(a2dpsink_timer->e_play_command_id);
        eloop_timer_stop(a2dpsink_timer->e_play_state_id);
        a2dpsink_state->play_state = A2DP_SINK_STATE_PLAY_INVALID;
        g_bt_a2dpsink->get_play_status = BT_AVRC_PLAYSTATE_STOPPED;
        a2dpsink_state->rc_open = 0;

        // avoid bluez disconnect the link with pulseaudio
        bt_a2dpsink_set_unmute();

        //ignorge to broadcast disconnect when not  connected yet
        if (a2dpsink_state->open_state < A2DP_SINK_STATE_CLOSED &&
            a2dpsink_state->connect_state == A2DP_SINK_STATE_CONNECTED) {
            if (a2dpsink_state->discoverable) {
                bt->set_visibility(bt, true, true);
            }
            a2dpsink_state->connect_state = A2DP_SINK_STATE_DISCONNECTED;
            a2dpsink_state->broadcast_state = a2dpsink_state->discoverable? A2DP_SINK_BROADCAST_OPENED : A2DP_SINK_BROADCAST_CLOSED;
            broadcast_a2dpsink_state(NULL);
        }
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
        if ((a2dpsink_state->open_state < A2DP_SINK_STATE_CLOSE) &&
                (a2dpsink_state->open_state >= A2DP_SINK_STATE_OPENED)) {
            eloop_timer_start(a2dpsink_timer->e_connect_over_id);
        }
#endif
        break;
    }
    case BT_AVK_START_EVT: {
        if (msg)
            BT_LOGI("BT_AVK_START_EVT : status= %d, idx %d\n", msg->start_streaming.status, msg->start_streaming.idx);
        if (msg->start_streaming.status == 0) {
            broadcast_a2dpsink_init_volume();
            eloop_timer_start(a2dpsink_timer->e_play_state_id);
        }

        break;
    }
    case BT_AVK_STOP_EVT: {
        if (msg)
            BT_LOGI("BT_AVK_STOP_EVT : status= %d, idx %d, suspended=%d\n", msg->stop_streaming.status, msg->stop_streaming.idx, msg->stop_streaming.suspended);

        eloop_timer_stop(a2dpsink_timer->e_play_state_id);
        bt_a2dpsink_playstate_check(NULL);
        g_bt_a2dpsink->get_play_status = BT_AVRC_PLAYSTATE_STOPPED;

        break;
    }
    case BT_AVK_RC_OPEN_EVT: {
        if (msg)
            BT_LOGI("BT_AVK_RC_OPEN_EVT : status= %d, idx %d\n", msg->rc_open.status, msg->rc_open.idx);
        a2dpsink_state->rc_open = 1;
        if (a2dpsink_state->connect_state == A2DP_SINK_STATE_CONNECTED)
            handle_a2dpsink_connect_subsequent();
        break;
    }
    case BT_AVK_RC_CLOSE_EVT: {
        if (msg)
            BT_LOGI("BT_AVK_RC_CLOSE_EVT : status= %d, idx %d\n", msg->rc_close.status, msg->rc_close.idx);
        a2dpsink_state->rc_open = 0;
        break;
    }
    case BT_AVK_GET_PLAY_STATUS_EVT: {
        if (msg) {
            BT_LOGI("BT_AVK_GET_PLAY_STATUS_EVT : play_status= %d\n", msg->get_play_status.play_status);
            g_bt_a2dpsink->get_play_status = msg->get_play_status.play_status;
        }
        break;
    }
    case BT_AVK_SET_ABS_VOL_CMD_EVT: {
        if (msg) {
            BT_LOGI("BT_AVK_SET_ABS_VOL_CMD_EVT : volume= %d\n", msg->abs_volume.volume);
            broadcast_a2dpsink_volume(msg->abs_volume.volume);
        }
        break;
    }
    case BT_AVK_GET_ELEM_ATTR_EVT: {
        if (msg) {
            broadcast_a2dpsink_element_attrs(msg->elem_attr);
        }
        break;
    }
    case BT_AVK_REGISTER_NOTIFICATION_EVT: {
        if (msg) {
            if (msg->reg_notif.rsp.event_id == BT_AVRC_EVT_PLAY_STATUS_CHANGE) {
                g_bt_a2dpsink->get_play_status = msg->reg_notif.rsp.param.play_status;
                eloop_timer_stop(a2dpsink_timer->e_play_state_id);
                bt_a2dpsink_playstate_check(NULL);
            }
        }
        break;
    }
    default:
        break;
    }
}

static int bt_a2dpsink_enable(struct rk_bluetooth *bt, char *name, bool discoverable) {
    int ret = 0;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;

    bt->a2dp_sink.disable(bt);
    ret = bt->a2dp_sink.set_listener(bt, a2dp_sink_listener, bt);
    if (ret) {
        BT_LOGE("set ble listener error :: %d\n", ret);
        return -2;
    }

    if (name)
        bt->set_bt_name(bt, name);

    if (bt->a2dp_sink.enable(bt)) {
        BT_LOGE("enable a2dp sink failed!\n");
        return -2;
    }

    ret = bt->set_visibility(bt, discoverable, true);
    if (ret) {
        BT_LOGE("set visibility error\n");
        return -3;
    }

    a2dpsink_state->broadcast_state = discoverable? A2DP_SINK_BROADCAST_OPENED : A2DP_SINK_BROADCAST_CLOSED;

    return 0;
}

void bt_a2dpsink_on_prepare(struct bt_service_handle *handle, json_object *obj) {
    char *subsequent = NULL;
    json_object *bt_subsequent = NULL;
    int unique = 0;
    json_object *bt_unique = NULL;
    struct a2dpsink_handle *a2dpsink = &handle->bt_a2dpsink;

    a2dpsink->subsequent = 0;

    if (json_object_object_get_ex(obj, "subsequent", &bt_subsequent) == TRUE) {
        subsequent = (char *)json_object_get_string(bt_subsequent);
        BT_LOGI("bt_a2dpsink :: subsequent %s \n", subsequent);
        if (strncmp (subsequent, "PLAY", strlen(subsequent)) == 0) {
            a2dpsink->subsequent = A2DP_SINK_COMMAND_PLAY;
        } else if (strncmp (subsequent, "STOP", strlen(subsequent)) == 0) {
            a2dpsink->subsequent = A2DP_SINK_COMMAND_STOP;
        } else if (strncmp (subsequent, "PAUSE", strlen(subsequent)) == 0) {
            a2dpsink->subsequent = A2DP_SINK_COMMAND_PAUSE;
        }
    }

    if (json_object_object_get_ex(obj, "unique", &bt_unique) == TRUE) {
        unique = json_object_get_boolean(bt_unique);
        BT_LOGI("bt_a2dpsink :: unique :: %d\n", unique);
        if (unique) {
            bt_close_mask_profile(handle, ~(A2DP_SINK_STATUS_MASK | HFP_STATUS_MASK));
        }
    }
}

int bt_a2dpsink_timer_init() {
    //struct a2dpsink_handle *a2dpsink = g_bt_a2dpsink;
    struct a2dpsink_timer *timer = &g_bt_a2dpsink->timer;

    timer->e_auto_connect_id = eloop_timer_add(auto_connect_by_history_index, NULL, 1000, 0);
    timer->e_play_command_id = eloop_timer_add(bt_a2dp_sink_play, g_bt_handle->bt, 1500, 0);
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
    timer->e_connect_over_id = eloop_timer_add(a2dpsink_autoconnect_over, NULL, BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME, 0);
#endif
    timer->e_play_state_id = eloop_timer_add(bt_a2dpsink_playstate_check, NULL, 500, 500);

    if ((timer->e_auto_connect_id <= 0) ||
            (timer->e_play_command_id <= 0) ||
       #if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
            (timer->e_connect_over_id <= 0) ||
       #endif
            (timer->e_play_state_id <= 0)) {
        return -1;
    }

    return 0;
}

void bt_a2dpsink_timer_uninit() {
    struct a2dpsink_timer *timer = &g_bt_a2dpsink->timer;

    eloop_timer_stop(timer->e_auto_connect_id);
    eloop_timer_stop(timer->e_play_command_id);
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
    eloop_timer_stop(timer->e_connect_over_id);
#endif
    eloop_timer_stop(timer->e_play_state_id);

    eloop_timer_delete(timer->e_auto_connect_id);
    eloop_timer_delete(timer->e_play_command_id);
#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
    eloop_timer_delete(timer->e_connect_over_id);
#endif
    eloop_timer_delete(timer->e_play_state_id);
}

static void  bt_a2dpsink_connect(const char *addr) {
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;
    uint8_t bd_addr[6] = {0};
    int ret = 0;
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;
    struct a2dpsink_timer *a2dpsink_timer = &g_bt_a2dpsink->timer;

    a2dpsink_config->autoconnect_index = 0;
    a2dpsink_config->autoconnect_flag = AUTOCONNECT_BY_HISTORY_OVER;
    eloop_timer_stop(a2dpsink_timer->e_auto_connect_id);

    if (a2dpsink_state->open_state != A2DP_SINK_STATE_OPENED ||
        get_a2dpsink_connected_device_num()) {
        broadcast_a2dpsink_state(NULL);
        return;
    }
    if (a2dpsink_state->connect_state == A2DP_SINK_STATE_CONNECTING) {
        BT_LOGW("connecting busy");
        return;
    }

    ret = bd_strtoba(bd_addr, addr);
    if (ret != 0) {
        BT_LOGE("mac not valid\n");
        return ;
    }

    ret = bt->a2dp_sink.connect(bt, bd_addr);
    if (ret != 0) {
        BT_LOGE("connect error\n");
        a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECTEFAILED;
        broadcast_a2dpsink_state(NULL);
        return ;
    }
    a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECTING;
}

void bt_a2dpsink_set_mute() {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;

    if (a2dpsink_state->open_state == A2DP_SINK_STATE_OPENED && get_a2dpsink_connected_device_num()) {
        BT_LOGI("set mute\n");
        bt->a2dp_sink.set_mute(bt, 1);
    }
}

void bt_a2dpsink_set_unmute() {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    struct a2dpsink_state *a2dpsink_state = &g_bt_a2dpsink->state;

    if (a2dpsink_state->open_state == A2DP_SINK_STATE_OPENED) {
        BT_LOGI("set unmute\n");
        bt->a2dp_sink.set_mute(bt, 0);
    }
}

void bt_a2dpsink_set_abs_volume(int volume) {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    uint8_t vol;

    if (!get_a2dpsink_connected_device_num()) {
        return;
    }
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    vol = (volume * 127 + 99) / 100;

    BT_LOGI("set vol: %d \n", vol);
    bt->a2dp_sink.set_abs_vol(bt, vol);
}

static void bt_a2dpsink_get_element_attrs() {
    struct rk_bluetooth *bt = g_bt_handle->bt;

    if (!get_a2dpsink_connected_device_num()) {
        return;
    }
    bt->a2dp_sink.get_element_attrs(bt);
}

int handle_a2dpsink_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    json_object *bt_cmd = NULL;
    struct a2dpsink_state *a2dpsink_state = NULL;
    struct a2dpsink_timer *a2dpsink_timer = NULL;
    struct rk_bluetooth *bt = NULL;

    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("bt_a2dpsink :: command %s \n", command);

        // g_bt_a2dpsink = &handle->bt_a2dpsink;
        a2dpsink_state = &g_bt_a2dpsink->state;
        a2dpsink_timer = &g_bt_a2dpsink->timer;
        bt = g_bt_handle->bt;

        if (strcmp(command, "ON") == 0) {
            char *name = NULL;
            json_object *bt_name = NULL;
            json_object *bt_auto_connect = NULL;
            json_object *bt_discoverable = NULL;
            bool auto_conn = true;
            bool discoverable = true;
#if defined(BT_SERVICE_HAVE_HFP)
            char *sec_pro = NULL;
            json_object *bt_sec_pro = NULL;
            int hfp = 0;

            if (json_object_object_get_ex(obj, "sec_pro", &bt_sec_pro) == TRUE) {
                sec_pro = (char *)json_object_get_string(bt_sec_pro);
                if (strcmp(sec_pro, "HFP") == 0) {
                    hfp = 1;
                }
            }
#endif

#if (BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME != 0)
            if (a2dpsink_state->open_state == A2DP_SINK_STATE_OPENED &&
                a2dpsink_state->connect_state != A2DP_SINK_STATE_CONNECTED) {
                a2dpsink_state->connect_state = A2DP_SINK_STATE_CONNECT_INVALID;
                eloop_timer_stop(a2dpsink_timer->e_connect_over_id);
                eloop_timer_start(a2dpsink_timer->e_connect_over_id);
            }
#endif
            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                name = (char *)json_object_get_string(bt_name);
                BT_LOGI("bt_a2dpsink :: name %s \n", name);
            } else {
                name = "ROKID-BT-9999zz";
            }

            if (json_object_object_get_ex(obj, "auto_connect", &bt_auto_connect) == TRUE) {
                auto_conn = json_object_get_boolean(bt_auto_connect);
            }
            if (json_object_object_get_ex(obj, "discoverable", &bt_discoverable) == TRUE) {
                discoverable = json_object_get_boolean(bt_discoverable);
            }

            bt_a2dpsink_on_prepare(handle, obj);
            bt_a2dpsink_on(name, auto_conn, discoverable);
        #if defined(BT_SERVICE_HAVE_HFP)
            if (hfp) {
                BT_LOGI("bt_a2dpsink :: enable sec_pro hfp\n");
                bt_hfp_on(NULL);
            }
        #endif
        } else if (strcmp(command, "OFF") == 0) {
        #if defined(BT_SERVICE_HAVE_HFP)
            char *pro_ex = NULL;
            json_object *bt_pro_ex = NULL;

            if (json_object_object_get_ex(obj, "sec_pro", &bt_pro_ex) == TRUE) {
                pro_ex = (char *)json_object_get_string(bt_pro_ex);
                if (strcmp(pro_ex, "HFP") == 0) {
                    bt_hfp_off(bt);
                }
            }
        #endif
            bt_a2dpsink_off(bt);
        } else if (strcmp(command, "GETSTATE") == 0) {
            broadcast_a2dpsink_state(reply);
        } else if (strcmp(command, "PLAY") == 0) {
            eloop_timer_stop(a2dpsink_timer->e_play_command_id);
            bt_a2dp_sink_play(NULL);
        } else if (strcmp(command, "STOP") == 0) {
            g_bt_a2dpsink->last_stop_time = get_current_micro_time();
            bt_a2dp_sink_stop(NULL);
        } else if (strcmp(command, "PAUSE") == 0) {
            g_bt_a2dpsink->last_stop_time = get_current_micro_time();
            bt_a2dp_sink_pause(NULL);
        } else if (strcmp(command, "PLAY_UNMUTE") == 0) {
            eloop_timer_stop(a2dpsink_timer->e_play_command_id);
            bt_a2dp_sink_play(NULL);
            bt_a2dpsink_set_unmute();
        } else if (strcmp(command, "PAUSE_MUTE") == 0) {
            g_bt_a2dpsink->last_stop_time = get_current_micro_time();
            bt_a2dp_sink_pause(NULL);
            bt_a2dpsink_set_mute();
        } else if (strcmp(command, "NEXT") == 0) {
            bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_FORWARD);
        } else if (strcmp(command, "PREV") == 0) {
            bt->a2dp_sink.send_avrc_cmd(bt, BT_AVRC_BACKWARD);
        } else if (strcmp(command, "SEND_KEY") == 0) {
            json_object *bt_key = NULL;
            int key;
            if (json_object_object_get_ex(obj, "key", &bt_key) == TRUE) {
                key = json_object_get_int(bt_key);
                BT_LOGI("send avrcp key 0x%X", key);
                if (key < 0xff && key >= 0) {
                    bt->a2dp_sink.send_avrc_cmd(bt, key);
                }
            }
        } else if (strcmp(command, "DISCONNECT") == 0) {
        #if defined(BT_SERVICE_HAVE_HFP)
            struct hfp_handle *hfp = g_bt_hfp;

            if (hfp->state.connect_state == HFP_STATE_CONNECTED &&
                hfp->state.open_state == HFP_STATE_OPENED) {
                bt_hfp_disconnect_device();
            }
        #endif
            bt_a2dpsink_disconnect_device(g_bt_a2dpsink);
        } else if (strcmp(command, "DISCONNECT_PEER") == 0) {
            bt_a2dpsink_disconnect_device(g_bt_a2dpsink);
        } else if (strcmp(command, "MUTE") == 0) {
            bt_a2dpsink_set_mute();
        } else if (strcmp(command, "UNMUTE") == 0) {
            bt_a2dpsink_set_unmute();
        } else if (strcmp(command, "VOLUME") == 0) {
            json_object *j_volume = NULL;
            int vol_int;
            if (json_object_object_get_ex(obj, "value", &j_volume) == TRUE) {
                vol_int = json_object_get_int(j_volume);
                BT_LOGI("bt_a2dpsink :: volume %d \n", vol_int);
                bt_a2dpsink_set_abs_volume(vol_int);
            }
        } else if (strcmp(command, "GETSONG_ATTRS") == 0) {
            bt_a2dpsink_get_element_attrs();
        } else if (strcmp(command, "CONNECT") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_a2dpsink_connect((char *)json_object_get_string(bt_addr));
            }
        }
    }

    return 0;
}

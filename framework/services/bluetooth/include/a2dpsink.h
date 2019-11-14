
#ifndef __BT_SERVICE_A2DPSINK_H_
#define __BT_SERVICE_A2DPSINK_H_

#include <hardware/bluetooth.h>
#include <eloop_mini/eloop.h>

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
#include <libzeromq_handle/sub.h>
#include <libzeromq_handle/pub.h>
ZEROMQ_PUB_DECLARE(pub_bt_a2dpsink);
#endif

#include "config.h"

#define BLUETOOTH_A2DPSINK_AUTOCONNECT_LIMIT_TIME (5 * 60 * 1000)
#define BLUETOOTH_RECONNECT_LIMIT_TIME 3

#define BLUETOOTH_A2DPSINK_CHECK_PLAY_INDEX 5  //(5*1.5s)
#define BLUETOOTH_A2DPSINK_CHECK_PLAY_ZERO_DATA_INDEX  5

// 60s
#define BLUETOOTH_A2DPSINK_CLOSE_LIMIT_TIME (60 * 2)

enum {
    A2DP_SINK_INIVATE = 0,
    A2DP_SINK_STATE_OPEN,
    A2DP_SINK_STATE_OPENED,
    A2DP_SINK_STATE_OPENFAILED,
    A2DP_SINK_STATE_CLOSE,
    A2DP_SINK_STATE_CLOSING,
    A2DP_SINK_STATE_CLOSED,
} A2DP_SINK_CURRENT_STATE;


enum {
    A2DP_SINK_STATE_CONNECT_INVALID,
    A2DP_SINK_STATE_CONNECTING = 100,
    A2DP_SINK_STATE_CONNECTED,
    A2DP_SINK_STATE_CONNECTEFAILED,
    A2DP_SINK_STATE_DISCONNECTED,
};

enum {
    A2DP_SINK_STATE_PLAY_INVALID,
    A2DP_SINK_STATE_PLAY = 200,
    A2DP_SINK_STATE_PLAYING,
    A2DP_SINK_STATE_PLAYED,
    A2DP_SINK_STATE_STOPED,
};

enum {
    A2DP_SINK_COMMAND_PLAY = 300,
    A2DP_SINK_COMMAND_PAUSE,
    A2DP_SINK_COMMAND_STOP,
};

enum {
    A2DP_SINK_BROADCAST_OPENED = 400,
    A2DP_SINK_BROADCAST_CLOSED,
};

enum {
    A2DP_SINK_AUTOCONNECT_BY_HISTORY_INIT = 99,
    A2DP_SINK_AUTOCONNECT_BY_HISTORY_WORK,
    A2DP_SINK_AUTOCONNECT_BY_HISTORY_OVER,
};

struct a2dpsink_state {
    bool discoverable;
    int open_state;
    int connect_state;
    int play_state;
    int broadcast_state;
    int rc_open;
};

struct a2dpsink_timer {
    eloop_id_t e_auto_connect_id;
    eloop_id_t e_play_command_id;
    eloop_id_t e_connect_over_id;
    eloop_id_t e_play_state_id;
};

struct a2dpsink_handle {
    int get_play_status;
    int subsequent;
    int64_t last_stop_time;
    pthread_mutex_t state_mutex;
    struct bt_autoconnect_config config;
    struct a2dpsink_timer timer;
    struct a2dpsink_state state;
};

void auto_connect_by_scan_results(struct rk_bluetooth *bt);
void bt_a2dpsink_off(struct rk_bluetooth *bt);
int bt_a2dpsink_timer_init();
void bt_a2dpsink_timer_uninit();

void bt_a2dpsink_set_mute();
void bt_a2dpsink_set_unmute();


extern struct a2dpsink_handle *g_bt_a2dpsink;

#endif

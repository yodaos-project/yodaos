#ifndef __BT_SERVICE_A2DPSOURCE_H_
#define __BT_SERVICE_A2DPSOURCE_H_

#include <hardware/bluetooth.h>
#include <eloop_mini/eloop.h>

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
#include <libzeromq_handle/sub.h>
#include <libzeromq_handle/pub.h>
#endif

#include "config.h"

#define BLUETOOTH_A2DPSOURCE_AUTOCONNECT_LIMIT_TIME (60 * 1000 * 5)
#define BLUETOOTH_A2DPSOURCE_DISCOVERY_LIMIT_TIME (1 * 1000)

enum {
    A2DP_SOURCE_BROADCAST_OPENED = 400,
    A2DP_SOURCE_BROADCAST_CLOSED,
};

enum {
    A2DP_SOURCE_STATE_CONNECT_INVALID,
    A2DP_SOURCE_STATE_CONNECTING = 100,
    A2DP_SOURCE_STATE_CONNECTED,
    A2DP_SOURCE_STATE_CONNECTEFAILED,
    A2DP_SOURCE_STATE_CONNECTEOVER,
    A2DP_SOURCE_STATE_DISCONNECTED,
};

enum {
    A2DP_SOURCE_INVALID = 0,
    A2DP_SOURCE_STATE_OPEN,
    A2DP_SOURCE_STATE_OPENED,
    A2DP_SOURCE_STATE_OPENFAILED,
    A2DP_SOURCE_STATE_CLOSE,
    A2DP_SOURCE_STATE_CLOSING,
    A2DP_SOURCE_STATE_CLOSED,
} A2DP_SOURCE_CURRENT_STATE;

struct a2dpsource_timer {
    eloop_id_t e_auto_connect_id;
    eloop_id_t e_connect_over_id;
    //eloop_id_t e_discovery_id;
};

struct a2dpsource_state {
    int open_state;
    int connect_state;
    int broadcast_state;
};

struct a2dpsource_handle {
    int subsequent;
    pthread_mutex_t state_mutex;
    struct bt_autoconnect_config config;
    struct a2dpsource_timer timer;
    struct a2dpsource_state state;
};

extern struct a2dpsource_handle *g_bt_a2dpsource;

void a2dpsource_upload_scan_results(const char *bt_name, BTAddr bt_addr, int is_completed, void *data);
int bt_a2dpsource_timer_init();
void bt_a2dpsource_timer_uninit();
void bt_a2dpsource_off(struct rk_bluetooth *bt);
#endif

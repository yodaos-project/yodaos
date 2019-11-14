#ifndef __BT_SERVICE_HH_H_
#define __BT_SERVICE_HH_H_

#include <hardware/bluetooth.h>
#include <json-c/json.h>
#include "common.h"

enum {
    HH_STATE_INVALID = 0,
    HH_STATE_OPEN,
    HH_STATE_OPENING,
    HH_STATE_OPENFAILED,
    HH_STATE_OPENED,
    HH_STATE_CLOSE,
    HH_STATE_CLOSING,
    HH_STATE_CLOSED,
} HH_CURRENT_STATE;


enum {
    HH_CONNECT_INVALID = 0,
    HH_STATE_CONNECTING = 100,
    HH_STATE_CONNECTED,
    HH_STATE_CONNECTFAILED,
    HH_STATE_DISCONNECTED,
} HH_CONNECT_STATE;

struct hh_state {
    int open_state;
    int connect_state;
};

struct hh_handle {
    struct hh_state state;
    struct bt_autoconnect_device dev;
    pthread_mutex_t state_mutex;
};

extern struct hh_handle *g_bt_hh;

void bt_hh_off(struct rk_bluetooth *bt);
#endif

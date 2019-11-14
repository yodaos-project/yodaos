#if defined(BT_SERVICE_HAVE_HFP)
#ifndef __BT_SERVICE_HFP_H_
#define __BT_SERVICE_HFP_H_

#include <hardware/bluetooth.h>
#include <eloop_mini/eloop.h>

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
#include <libzeromq_handle/sub.h>
#include <libzeromq_handle/pub.h>
ZEROMQ_PUB_DECLARE(pub_bt_hfp);
#endif

#include "config.h"

enum {
    HFP_INIVATE = 0,
    HFP_STATE_OPEN,
    HFP_STATE_OPENED,
    HFP_STATE_OPENFAILED,
    HFP_STATE_CLOSE,
    HFP_STATE_CLOSING,
    HFP_STATE_CLOSED,
};


enum {
    HFP_STATE_CONNECT_INVALID,
    HFP_STATE_CONNECTING,
    HFP_STATE_CONNECTED,
    HFP_STATE_CONNECTEFAILED,
    HFP_STATE_DISCONNECTING,
    HFP_STATE_DISCONNECTED,
};

enum {
    HFP_SERVICE_INACTIVE,
    HFP_SERVICE_ACTIVE,
};

enum {
    HFP_CALL_INACTIVE,
    HFP_CALL_ACTIVE,
};

enum {
    HFP_CALLSETUP_NONE,
    HFP_CALLSETUP_INCOMING,
    HFP_CALLSETUP_OUTGOING,
    HFP_CALLSETUP_ALERTING,
};

enum {
    HFP_CALLHELD_NONE,
    HFP_CALLHELD_ACTIVE_HELD,/* AG has both an active and a held call */
    HFP_CALLHELD_HELD,/* AG has call on hold, no active call */
};

enum {
    HFP_AUDIO_OFF,
    HFP_AUDIO_ON,
};


struct hfp_state {
    int open_state;
    int connect_state;
    int service;
    int call;
    int callsetup;
    int callheld;
    int audio;
};

struct hfp_timer {
    eloop_id_t e_auto_connect_id;
    eloop_id_t e_connect_over_id;
};

struct hfp_handle {
    pthread_mutex_t state_mutex;
    struct bt_autoconnect_device dev;
    //struct hfp_timer timer;
    struct hfp_state state;
};

void bt_hfp_on(const char *name);
void bt_hfp_off(struct rk_bluetooth *bt);
void bt_hfp_disconnect_device(void);
void bt_hfp_connect(BTAddr bd_addr);


extern struct hfp_handle *g_bt_hfp;

#endif
#endif

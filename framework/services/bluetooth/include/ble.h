#ifndef __BT_SERVICE_BLE_H_
#define __BT_SERVICE_BLE_H_

#include <hardware/bluetooth.h>
#include <json-c/json.h>
#include "common.h"

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
#include <libzeromq_handle/sub.h>
#include <libzeromq_handle/pub.h>
ZEROMQ_PUB_DECLARE(pub_bt_ble);
ZEROMQ_PUB_DECLARE(ble_in_stream);
#endif


#define BLE_DATA_LEN 20
#define ROKID_BLE_DATA_LEN 17

#define BLE_DATA_CHARACTER_CMCC 0x2a06
#define BLE_DATA_CHARACTER 0x2a07

void ble_send_raw_data(struct rk_bluetooth *bt, uint8_t *buf, int len);
void bt_ble_off(struct rk_bluetooth *bt);

enum {
    BLE_INIVATE = 0,
    BLE_STATE_OPEN,
    BLE_STATE_OPENING,
    BLE_STATE_OPENFAILED,
    BLE_STATE_OPENED,
    BLE_STATE_CONNECTED,
    BLE_STATE_HANDSHAKED,
    BLE_STATE_DISCONNECTED,
    BLE_STATE_CLOSE,
    BLE_STATE_CLOSING,
    BLE_STATE_CLOSED,
} BLE_CURRENT_STATE;

#pragma pack(1)
struct rokid_ble_format {
    uint8_t msgid;
    uint16_t last_index;
    uint8_t data[17];
};

struct rokid_ble_handshake {
    uint8_t magic_num[2];
    uint8_t version;
    uint8_t data[5];
};
#pragma pack()

struct ble_state {
    int open_state;
};

struct ble_handle {
    struct ble_state state;
    pthread_mutex_t state_mutex;
};

extern struct ble_handle *g_bt_ble;

#endif

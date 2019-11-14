#ifndef __BT_SERVICE_BLE_CLI_H_
#define __BT_SERVICE_BLE_CLI_H_

#include <hardware/bluetooth.h>
#include <json-c/json.h>
#include "common.h"

enum {
    BLE_CLI_OPEN_INVALID = 0,
    BLE_CLI_STATE_OPENING,
    BLE_CLI_STATE_OPENFAILED,
    BLE_CLI_STATE_OPENED,
    BLE_CLI_STATE_CLOSE,
    BLE_CLI_STATE_CLOSING,
    BLE_CLI_STATE_CLOSED,
};

enum {
    BLE_CLI_CONNECT_INVALID = 0,
    BLE_CLI_STATE_CONNECTING = 100,
    BLE_CLI_STATE_CONNECTED,
    BLE_CLI_STATE_CONNECTFAILED,
    BLE_CLI_STATE_DISCONNECTED,
};

struct ble_client_state {
    int open_state;
    int connect_state;
};

struct ble_client_handle {
    struct ble_client_state state;
    struct bt_autoconnect_device dev;
    //struct bt_autoconnect_config config;
    pthread_mutex_t state_mutex;
};

#pragma pack(1)
struct rokid_ble_client_header {
    uint32_t len; // header length
    uint8_t address[18];
    uint8_t service[MAX_LEN_UUID_STR];
    uint8_t character[MAX_LEN_UUID_STR];
    uint8_t description[MAX_LEN_UUID_STR];
};
#pragma pack()

extern struct ble_client_handle *g_bt_ble_client;

void bt_ble_client_off(struct rk_bluetooth *bt);
#endif

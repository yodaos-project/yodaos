#ifndef _BLE_SERVER_H_
#define _BLE_SERVER_H_

#ifdef __cplusplus
extern "{"
#endif

#include "app_utils.h"
#include <bsa_rokid/bsa_api.h>
#include <hardware/bt_common.h>

typedef struct
{
    tBT_UUID       attr_UUID;
    UINT16         service_id;
    UINT16         attr_id;
    UINT8          attr_type;
    UINT8          prop;
    BOOLEAN        is_pri;
    BOOLEAN        wait_flag;
} tAPP_BLE_ATTRIBUTE;

typedef struct
{
    BOOLEAN             enabled;
    tBSA_BLE_IF         server_if;
    UINT16              conn_id;
    tAPP_BLE_ATTRIBUTE  attr[BSA_BLE_ATTRIBUTE_MAX];
} tAPP_BLE_SERVER;

struct BleServer_t
{
    BOOLEAN             enabled;

    tAPP_BLE_SERVER ble_server[BSA_BLE_SERVER_MAX];

    void *listener_handle;
    ble_callbacks_t listener;
    char ble_menufacturer[BSA_DM_BLE_AD_DATA_LEN];
    void *caller;
};

typedef struct BleServer_t BleServer;
BleServer *ble_create(void *caller);

int ble_server_set_listener(BleServer *bles, ble_callbacks_t listener, void *listener_handle);
int ble_destroy(BleServer *bles);
int rokid_ble_enable(BleServer *bles);
int rokid_ble_disable(BleServer *bles);
int rk_ble_send(BleServer *bles, UINT16 uuid, char *buf, int len);

int ble_server_register(BleServer *bles, unsigned short uuid);
int ble_server_create_service(BleServer *bles, int server_num);
int ble_server_add_char(BleServer *bles, int server_num, int attr_num, UINT16 uuid, int is_descript);
int ble_server_start_service(BleServer *bles, int server_num, int attr_num);
int ble_server_start_adv(BleServer *bles);

int ble_server_stop_service(BleServer *bles, int num, int attr_num);
int ble_server_close(BleServer *bles);
int ble_server_deregister(BleServer *bles);
int ble_server_set_menufacturer(BleServer *bles, char *name);
int ble_get_enabled(BleServer *bles);

#ifdef __cplusplus
}
#endif
#endif

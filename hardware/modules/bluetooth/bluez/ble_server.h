#ifndef _BLE_SERVER_H_
#define _BLE_SERVER_H_

#ifdef __cplusplus
extern "{"
#endif

#include <hardware/bt_common.h>
//#include "app_manager.h"

#define MAX_ADV_DATA_LEN 31
#define BLE_SERVICE_UUID16 0xffe1
#define BLE_SERVICE_UUID  "0000ffe1-0000-1000-8000-00805f9b34fb"
#define BLE_CHRC_UUID1     "00002a06-0000-1000-8000-00805f9b34fb"
#define BLE_CHRC_UUID2     "00002a07-0000-1000-8000-00805f9b34fb"

struct BleServer_t
{
    bool le;
    bool ad_completed;
    bool enabled;
    bool connected;
    void *listener_handle;
    ble_callbacks_t listener;
    void *caller;
};
typedef struct BleServer_t BleServer;

void ble_proxy_added(BleServer *bles, GDBusProxy *proxy, void *user_data);
void ble_proxy_removed(BleServer *bles, GDBusProxy *proxy, void *user_data);

void ble_property_changed(BleServer *bles, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data);

int ble_server_enable(BleServer *bles);

int ble_server_disable(BleServer *bles);

int ble_send_uuid_buff(BleServer *bles, uint16_t uuid, char *buff, int len);

BleServer *ble_create(void *caller);

int ble_destroy(BleServer *bles);

int ble_server_set_listener(BleServer *bles, ble_callbacks_t listener, void *listener_handle);
int ble_get_enabled(BleServer *bles);

int ble_change_to_bredr(BleServer *bles);

#ifdef __cplusplus
}
#endif
#endif

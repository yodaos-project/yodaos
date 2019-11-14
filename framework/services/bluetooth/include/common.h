#ifndef __BT_SERVICE_COMMON_H_
#define __BT_SERVICE_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <hardware/bluetooth.h>
#include <eloop_mini/eloop.h>
#include "config.h"

#include "ble.h"
#include "ble_client.h"
#include "a2dpsink.h"
#include "hfp.h"
#include "a2dpsource.h"
#include "hh.h"
#include "ipc.h"

//#define bt_print  printf

#define BLE_STATUS_MASK     (1 << 0)
#define BLE_CLIENT_STATUS_MASK     (1 << 1)
#define A2DP_SOURCE_STATUS_MASK     (1 << 2)
#define A2DP_SINK_STATUS_MASK   (1 << 3)
#define HFP_STATUS_MASK     (1 << 4)
#define HID_HOST_STATUS_MASK     (1 << 5)
#define HID_DEVICE_STATUS_MASK     (1 << 6)

enum {
    BLUETOOTH_FUNCTION_COMMON,
    BLUETOOTH_FUNCTION_BLE,
    BLUETOOTH_FUNCTION_BLE_CLIENT,
    BLUETOOTH_FUNCTION_A2DPSINK,
    BLUETOOTH_FUNCTION_A2DPSOURCE,
    BLUETOOTH_FUNCTION_HFP,
    BLUETOOTH_FUNCTION_HH,

    /*binary*/
    BLUETOOTH_FUNCTION_BLE_CLI_NOTIFY,
    BLUETOOTH_FUNCTION_BLE_CLI_READ,
} BLUETOOTH_FUNCTION;

extern struct bt_service_handle *g_bt_handle;

struct bt_service_handle {
    uint32_t status;
    uint8_t open;
    int scanning;
    struct rk_bluetooth *bt;
    struct ble_handle bt_ble;
    struct ble_client_handle bt_ble_client;
    struct a2dpsink_handle bt_a2dpsink;
    struct a2dpsource_handle bt_a2dpsource;
#if defined(BT_SERVICE_HAVE_HFP)
    struct hfp_handle bt_hfp;
#endif
    struct hh_handle bt_hh;
};

int bt_open(struct bt_service_handle *handle, const char *name);
int bt_close(struct bt_service_handle *handle);
/*close mask_status opened profiles */
int bt_close_mask_profile(struct bt_service_handle *handle, uint32_t mask_status);

void broadcast_remove_dev(const char* addr, int status);
void bt_upload_scan_results(const char *bt_name, BTAddr bt_addr, int is_completed, void *data);
int bt_discovery(struct bt_service_handle *handle, int type);


int handle_common_handle(json_object *obj, struct bt_service_handle *bt, void *reply);
int handle_ble_handle(json_object *obj, struct bt_service_handle *bt, void *reply);
int handle_ble_client_handle(json_object *obj, struct bt_service_handle *bt, void *reply);
int handle_a2dpsink_handle(json_object *obj, struct bt_service_handle *bt, void *reply);
int handle_a2dpsource_handle(json_object *obj, struct bt_service_handle *bt, void *reply);
#if defined(BT_SERVICE_HAVE_HFP)
int handle_hfp_handle(json_object *obj, struct bt_service_handle *handle, void *reply);
#endif
int handle_hh_handle(json_object *obj, struct bt_service_handle *handle, void *reply);

int handle_module_sleep(json_object *obj, struct bt_service_handle *handle);

int handle_power_sleep(json_object *obj, struct bt_service_handle *handle);


int handle_ble_client_write(struct bt_service_handle *handle, const char *buff, int len);

#ifdef __cplusplus
}
#endif
#endif

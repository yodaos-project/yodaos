#ifndef _BLE_CLIENT_H_
#define _BLE_CLIENT_H_

#ifdef __cplusplus
extern "{"
#endif
#include <bsa_rokid/bsa_api.h>
#include <hardware/bt_common.h>
#include "app_ble_client_db.h"

/* display function for read and notify events */
typedef void (*FP_DISP_FUNC)(char * fmt, ...);

typedef struct
{
    char name[249];
    tBT_UUID        service_UUID;
    BOOLEAN         enabled;
    tBSA_BLE_IF     client_if;
    UINT16          conn_id;
    BD_ADDR         server_addr;
    BOOLEAN         write_pending;
    BOOLEAN         read_pending;
    BOOLEAN         congested;
} tAPP_BLE_CLIENT;

struct BleClient_t
{
    bool             enabled;

    tAPP_BLE_CLIENT ble_client[BSA_BLE_CLIENT_MAX];

    void *listener_handle;
    ble_callbacks_t listener;
    void *caller;
};

typedef struct BleClient_t BleClient;


int app_ble_client_register(BleClient *blec, UINT16 uuid);

int app_ble_client_deregister_all(BleClient *blec);

int rokid_ble_client_enable(BleClient *blec);

int rokid_ble_client_disable(BleClient *blec);

BleClient *blec_create(void *caller);

int blec_destroy(BleClient *blec);

int app_ble_client_open(BleClient *blec, int client_num, BD_ADDR bd_addr, bool is_direct);

int app_ble_client_service_search_execute(BleClient *blec, int client_num, char *service);

int app_ble_client_read(BleClient *blec, int client_num, 
                        char * service, char * char_id, int ser_inst_id, int char_inst_id, int is_primary, char * descr_id);


int app_ble_client_write(BleClient *blec, int client_num,
            char * service, char * char_id, int ser_inst_id, int char_inst_id, int is_primary, UINT8 *buff, int len,
            char * descr_id, UINT8 desc_value);

int app_ble_client_register_notification(BleClient *blec, int client_num, char * service_id, char * character_id,
                    int ser_inst_id, int char_inst_id, int is_primary);

int app_ble_client_close(BleClient *blec, BD_ADDR bd_addr);


int app_ble_client_unpair(BleClient *blec, BD_ADDR bd_addr);


int app_ble_client_deregister_notification(BleClient *blec, int client_num, char * service_id, char * character_id,
                    int ser_inst_id, int char_inst_id, int is_primary);

int ble_client_set_listener(BleClient *blec, ble_callbacks_t listener, void *listener_handle);

int ble_client_get_connect_devs(BleClient *blec, BTDevice *devs, int arr_len);

#ifdef __cplusplus
}
#endif
#endif

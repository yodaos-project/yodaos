/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INCLUDE_BLUETOOTH_H__
#define __INCLUDE_BLUETOOTH_H__

#include <stdbool.h>
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <hardware/bt_common.h>


__BEGIN_DECLS

#define BLUETOOTH_HARDWARE_MODULE_ID "bluetooth"

#define BLUETOOTH_INTERFACE_STRING "bluetoothInterface"

/** Bluetooth profile interface IDs */

#define BT_PROFILE_HANDSFREE_ID "handsfree"
#define BT_PROFILE_HANDSFREE_CLIENT_ID "handsfree_client"
#define BT_PROFILE_ADVANCED_AUDIO_ID "a2dp"
#define BT_PROFILE_ADVANCED_AUDIO_SINK_ID "a2dp_sink"
#define BT_PROFILE_HEALTH_ID "health"
#define BT_PROFILE_SOCKETS_ID "socket"
#define BT_PROFILE_HIDHOST_ID "hidhost"
#define BT_PROFILE_HIDDEV_ID "hiddev"
#define BT_PROFILE_PAN_ID "pan"
#define BT_PROFILE_MAP_CLIENT_ID "map_client"
#define BT_PROFILE_SDP_CLIENT_ID "sdp"
#define BT_PROFILE_GATT_ID "gatt"
#define BT_PROFILE_AV_RC_ID "avrcp"
#define BT_PROFILE_AV_RC_CTRL_ID "avrcp_ctrl"

/** Bluetooth test interface IDs */
#define BT_TEST_INTERFACE_MCAP_ID "mcap_test"
struct rk_bluetooth;

typedef struct {
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, ble_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);

    int (*disable)(struct rk_bluetooth *bt);

    int (*send_buf)(struct rk_bluetooth *bt, uint16_t uuid,  char *buf, int len);
    int (*set_ble_visibility)(struct rk_bluetooth *bt, bool discoverable, bool connectable);

} ble_interface_t;

typedef struct {
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, ble_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);
    int (*disable)(struct rk_bluetooth *bt);
    int (*connect)(struct rk_bluetooth *bt, BTAddr bd_addr);
    int (*disconnect)(struct rk_bluetooth *bt, BTAddr bd_addr);
    int (*search_service)(struct rk_bluetooth *bt, char *service);
    int (*register_notification)(struct rk_bluetooth *bt, char *service_id, char *character_id);
    int (*read)(struct rk_bluetooth *bt, char *service_id, char *character_id);
    int (*write)(struct rk_bluetooth *bt, char *service_id, char *character_id, char *buff, int len);
    int (*get_connected_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
} blec_interface_t;

typedef struct {
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, a2dp_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);

    int (*disable)(struct rk_bluetooth *bt);

    int (*connect)(struct rk_bluetooth *bt, BTAddr bd_addr);

    int (*disconnect)(struct rk_bluetooth *bt, BTAddr bd_addr);

    int (*start)(struct rk_bluetooth *bt, BT_A2DP_CODEC_TYPE type);
    int (*send_avrc_cmd)(struct rk_bluetooth *bt, uint8_t command);

    int (*get_connected_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
    /**
     * Native support for dumpsys function
     * Function is synchronous and |fd| is owned by caller.
     * |arguments| are arguments which may affect the output, encoded as
     * UTF-8 strings.
     */
    //void (*dump)(struct rk_bluetooth *bt, int fd);
} a2dp_interface_t;

typedef struct {
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, a2dp_sink_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);

    int (*disable)(struct rk_bluetooth *bt);

    int (*connect)(struct rk_bluetooth *bt, BTAddr bd_addr);

    int (*disconnect)(struct rk_bluetooth *bt, BTAddr bd_addr);

    int (*send_avrc_cmd)(struct rk_bluetooth *bt, uint8_t command);
    int (*send_get_playstatus)(struct rk_bluetooth *bt);

    int (*set_mute)(struct rk_bluetooth *bt, bool mute);
    int (*set_abs_vol)(struct rk_bluetooth *bt, uint8_t vol);
    int (*get_playing)(struct rk_bluetooth *bt);
    int (*get_element_attrs)(struct rk_bluetooth *bt);
    int (*get_connected_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
    /**
     * Native support for dumpsys function
     * Function is synchronous and |fd| is owned by caller.
     * |arguments| are arguments which may affect the output, encoded as
     * UTF-8 strings.
     */
    //void (*dump)(struct rk_bluetooth *bt, int fd);
} a2dp_sink_interface_t;

typedef struct {
    bt_hfp_data_type type;
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, hfp_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);
    int (*disable)(struct rk_bluetooth *bt);
    int (*connect)(struct rk_bluetooth *bt, BTAddr bd_addr);
    int (*disconnect)(struct rk_bluetooth *bt);
    int (*answercall)(struct rk_bluetooth *bt);
    int (*hangup)(struct rk_bluetooth *bt);
    int (*dial_num)(struct rk_bluetooth *bt, const char *num);
    int (*dial_last_num)(struct rk_bluetooth *bt);

    int (*send_dtmf)(struct rk_bluetooth *bt, char dtmf);
    int (*set_volume)(struct rk_bluetooth *bt, BT_HS_VOLUME_TYPE type, int volume);
    int (*mute_mic)(struct rk_bluetooth *bt, bool mute);
    /**
     * Native support for dumpsys function
     * Function is synchronous and |fd| is owned by caller.
     * |arguments| are arguments which may affect the output, encoded as
     * UTF-8 strings.
     */
    //void (*dump)(struct rk_bluetooth *bt, int fd);
} hfp_interface_t;

typedef struct {
    /**
     * set callback listener before enable
     */
    int (*set_listener)(struct rk_bluetooth *bt, hh_callbacks_t cb, void *listener_handle);
    int (*enable)(struct rk_bluetooth *bt);
    int (*disable)(struct rk_bluetooth *bt);
    int (*connect)(struct rk_bluetooth *bt, BTAddr bd_addr, int mode);
    int (*disconnect)(struct rk_bluetooth *bt, BTAddr bd_addr);
    int (*get_connected_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
} hh_interface_t;//hid host

struct rk_bluetooth
{
    /**
     * set callback listener before open
     */
    int (*set_manage_listener)(struct rk_bluetooth *bt, manage_callbacks_t m_cb, void *listener_handle);
    int (*set_discovery_listener)(struct rk_bluetooth *bt, discovery_cb_t dis_cb, void *listener_handle);
    int (*open)(struct rk_bluetooth *bt, const char *name);
    int (*close)(struct rk_bluetooth *bt);

    int (*set_bt_name)(struct rk_bluetooth *bt, const char *name);

    /**
     * start disvocery bt devices
     * enum bt_profile_type
     *     BT_A2DP_SOURCE: scan for a2dp source
     *     BT_A2DP_SINK:
     *     BT_BLE:
     *     BT_HS:
     *     BT_BR_EDR:
     *     BT_ALL:
     */
    int (*start_discovery)(struct rk_bluetooth *bt, enum bt_profile_type);
    int (*cancel_discovery)(struct rk_bluetooth *bt);
    int (*remove_dev)(struct rk_bluetooth *bt, BTAddr bd_addr);
    int (*set_visibility)(struct rk_bluetooth *bt, bool discoverable, bool connectable);
    int (*get_disc_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
    int (*get_bonded_devices)(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len);
    int (*find_scaned_device)(struct rk_bluetooth *bt, BTDevice *dev);
    int (*find_bonded_device)(struct rk_bluetooth *bt, BTDevice *dev);
    int (*get_module_addr)(struct rk_bluetooth *bt, BTAddr bd_addr);
    /**
     * Clear /data/misc/bt_config.conf and erase all stored connections
     */
    int (*config_clear)(struct rk_bluetooth *bt);

    ble_interface_t ble;
    blec_interface_t blec;//ble client
    a2dp_interface_t a2dp;
    a2dp_sink_interface_t a2dp_sink;
    hfp_interface_t hfp;
    hh_interface_t hh;
    void *handle;
};

struct bluetooth_module_t {
    struct hw_module_t common;
};

struct bluetooth_device_t {
    struct hw_device_t common;
    int (*creat)(struct rk_bluetooth **bt);
    int (*destroy)(struct rk_bluetooth *bt);
};

__END_DECLS

#endif /* __INCLUDE_BLUETOOTH_H__ */

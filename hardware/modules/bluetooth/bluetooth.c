/*************************************************************************
        > File Name: bluetooth.c
        > Author: tian.fan@rokid.com
        > Mail: tian.fan@rokid.com
 ************************************************************************/
#define LOG_TAG ""

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <hardware/hardware.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include <cutils/properties.h>
#include <time.h>

#include <hardware/bluetooth.h>

#define MODULE_NAME "bluetooth"
#define MODULE_AUTHOR "tian.fan@rokid.com"

int bt_log_level = LEVEL_OVER_LOGI;


static int rk_bluetooth_set_bug_level(void)
{
    char value[PROP_VALUE_MAX] = {0};

    property_get("persist.rokid.debug.bt", value, "0");
    if (atoi(value) > 0)
        bt_log_level = atoi(value);

    return 0;
}


static int rk_bluetooth_open(struct rk_bluetooth *bt, const char *name)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_open(bt->handle, name);

    return ret;
}

static int rk_bluetooth_close(struct rk_bluetooth *bt)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_close(bt->handle);

    return ret;
}

static int rk_bluetooth_get_module_addr(struct rk_bluetooth *bt, BTAddr addr)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_get_module_addr(bt->handle, addr);

    return ret;
}

static int rk_bluetooth_set_manage_listener(struct rk_bluetooth *bt, manage_callbacks_t m_cb, void *listener_handle)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_set_manage_listener(bt->handle, m_cb, listener_handle);

    return ret;
}

static int rk_bluetooth_set_discovery_listener(struct rk_bluetooth *bt, discovery_cb_t dis_cb, void *listener_handle)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_set_discovery_listener(bt->handle, dis_cb, listener_handle);
    return ret;
}

static int rk_bluetooth_set_bt_name(struct rk_bluetooth *bt, const char *name)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_set_name(bt->handle, name);
}

static int rk_bluetooth_start_discovery(struct rk_bluetooth *bt, enum bt_profile_type type)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_discovery(bt->handle, type);
}

static int rk_bluetooth_cancel_discovery(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_discovery_cancel(bt->handle);
}

static int rk_bluetooth_remove_device(struct rk_bluetooth *bt, BTAddr addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_remove_device(bt->handle, addr);
}

static int rk_bluetooth_set_visibility(struct rk_bluetooth *bt, bool discoverable, bool connectable)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_set_visibility(bt->handle, discoverable, connectable);
}

#if 0
static
#endif
int rk_bluetooth_get_disc_devices_count(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_get_disc_devices_count(bt->handle);
}

static int rk_bluetooth_get_disc_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_get_disc_devices(bt->handle, dev_array, arr_len);
}

static int rk_bluetooth_find_scaned_device(struct rk_bluetooth *bt, BTDevice *dev)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_find_scaned_device(bt->handle, dev);
}
static int rk_bluetooth_get_bonded_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_get_bonded_devices(bt->handle, dev_array, arr_len);
}

static int rk_bluetooth_find_bonded_device(struct rk_bluetooth *bt, BTDevice *dev)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_find_bonded_device(bt->handle, dev);
}

static int rk_bluetooth_config_clear(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_clear_devices(bt->handle);
}

/* BLE FUNC BEGIN */
static int rk_bluetooth_ble_enable(struct rk_bluetooth *bt)
{
    int ret = BT_STATUS_NOT_READY;
    BT_CHECK_HANDLE(bt);
    ret = rokidbt_ble_enable(bt->handle);
    return ret;
}

static int rk_bluetooth_ble_disable(struct rk_bluetooth *bt)
{
    int ret = BT_STATUS_NOT_READY;
    BT_CHECK_HANDLE(bt);
    ret = rokidbt_ble_disable(bt->handle);
    return ret;
}

static int rk_bluetooth_ble_send_buf(struct rk_bluetooth *bt, uint16_t uuid,  char *buf, int len)
{
    int ret = BT_STATUS_NOT_READY;
    BT_CHECK_HANDLE(bt);
    ret = rokidbt_ble_send_buf(bt->handle, uuid, buf, len);
    return ret;
}

static int rk_bluetooth_ble_set_listener(struct rk_bluetooth *bt, ble_callbacks_t cb, void *listener_handle)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_ble_set_listener(bt->handle, cb, listener_handle);

    return ret;
}

static int rk_bluetooth_ble_set_visibility(struct rk_bluetooth *bt, bool discoverable, bool connectable)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_ble_set_visibility(bt->handle, discoverable, connectable);
}
/* BLE FUNC END */

/* BLE CLIENT FUNC BEGIN */
static int rk_bluetooth_blec_set_listener(struct rk_bluetooth *bt, ble_callbacks_t cb, void *listener_handle)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_set_listener(bt->handle, cb, listener_handle);
    return ret;
}

static int rk_bluetooth_blec_enable(struct rk_bluetooth *bt)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_enable(bt->handle);
    return ret;
}

static int rk_bluetooth_blec_disable(struct rk_bluetooth *bt)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_disable(bt->handle);
    return ret;
}

static int rk_bluetooth_blec_connect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_connect(bt->handle, bd_addr);
    return ret;
}

static int rk_bluetooth_blec_disconnect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_disconnect(bt->handle, bd_addr);
    return ret;
}

static int rk_bluetooth_blec_search_service(struct rk_bluetooth *bt, char *service)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_search_service(bt->handle, service);
    return ret;
}

static int rk_bluetooth_blec_register_notification(struct rk_bluetooth *bt, char *service_id, char *character_id)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_register_notification(bt->handle, service_id, character_id);
    return ret;
}

static int rk_bluetooth_blec_read(struct rk_bluetooth *bt, char *service_id, char *character_id)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_read(bt->handle, service_id, character_id);
    return ret;
}

static int rk_bluetooth_blec_write(struct rk_bluetooth *bt, char *service_id, char *character_id, char *buff, int len)
{
    int ret = BT_STATUS_NOT_READY;

    BT_CHECK_HANDLE(bt);
    ret = rokidbt_blec_write(bt->handle, service_id, character_id, buff, len);
    return ret;
}

static int rk_bluetooth_blec_get_connected_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_blec_get_connected_devices(bt->handle, dev_array, arr_len);
}

/* BLE CLIENT FUNC END */

/* A2DP FUNC BEGIN*/
static int rk_bluetooth_a2dp_set_listener(struct rk_bluetooth *bt, a2dp_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_set_listener(bt->handle, listener, listener_handle);
}

static int rk_bluetooth_a2dp_enable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_enable(bt->handle);
}

static int rk_bluetooth_a2dp_disable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_disable(bt->handle);
}

static int rk_bluetooth_a2dp_connect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_open(bt->handle, bd_addr);
}

static int rk_bluetooth_a2dp_disconnect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_close_device(bt->handle, bd_addr);
}

static int rk_bluetooth_a2dp_start(struct rk_bluetooth *bt, BT_A2DP_CODEC_TYPE type)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_start(bt->handle, type);
}

static int rk_bluetooth_a2dp_send_avrc_cmd(struct rk_bluetooth *bt, uint8_t command)
{
    BT_CHECK_HANDLE(bt);
    BT_LOGD("send avrc cmd %d", command);
    return rokidbt_a2dp_send_rc_cmd(bt->handle, command);
}

static int rk_bluetooth_a2dp_get_connected_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_get_connected_devices(bt->handle, dev_array, arr_len);
}
/* A2DP FUNC END */

/* A2DP SINK FUNC BEGIN*/
static int rk_bluetooth_a2dp_sink_set_listener(struct rk_bluetooth *bt, a2dp_sink_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_set_listener(bt->handle, listener, listener_handle);
}

static int rk_bluetooth_a2dp_sink_enable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_enable(bt->handle);
}

static int rk_bluetooth_a2dp_sink_connect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_open(bt->handle, bd_addr);
}

static int rk_bluetooth_a2dp_sink_disconnect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_close_device(bt->handle, bd_addr);
}

#if 0
static
#endif
int rk_bluetooth_a2dp_sink_disconnect_all(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_close(bt->handle);
}

static int rk_bluetooth_a2dp_sink_disable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_disable(bt->handle);
}

static int rk_bluetooth_a2dp_sink_send_get_playstatus(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_send_getplaystatus(bt->handle);
}

static int rk_bluetooth_a2dp_sink_send_avrc_cmd(struct rk_bluetooth *bt, uint8_t command)
{
    BT_CHECK_HANDLE(bt);
    BT_LOGD("send avrc cmd %d", command);
    return rokidbt_a2dp_sink_send_avrc_cmd(bt->handle, command);
}

static int rk_bluetooth_a2dp_sink_get_playing(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_get_playing(bt->handle);
}

static int rk_bluetooth_a2dp_sink_get_element_attrs(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_get_element_attrs(bt->handle);
}

static int rk_bluetooth_a2dp_sink_get_connected_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_get_connected_devices(bt->handle, dev_array, arr_len);
}

static int rk_bluetooth_a2dp_sink_set_mute(struct rk_bluetooth *bt, bool mute)
{
    BT_CHECK_HANDLE(bt);
    BT_LOGD("set mute %d", mute);
    return rokidbt_a2dp_sink_set_mute(bt->handle, mute);
}

static int rk_bluetooth_a2dp_sink_set_abs_vol(struct rk_bluetooth *bt, uint8_t vol)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_a2dp_sink_set_abs_vol(bt->handle, vol);
}
/* A2DP SINK FUNC END */

/* HFP FUNC BEGIN*/
static int rk_bluetooth_hfp_get_type(void)
{
    return rokidbt_hs_get_type();
}

static int rk_bluetooth_hfp_set_listener(struct rk_bluetooth *bt, hfp_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_set_listener(bt->handle, listener, listener_handle);
}

static int rk_bluetooth_hfp_enable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_enable(bt->handle);
}

static int rk_bluetooth_hfp_disable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_disable(bt->handle);
}

static int rk_bluetooth_hfp_connect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_open(bt->handle, bd_addr);
}

static int rk_bluetooth_hfp_disconnect(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_close(bt->handle);
}

static int rk_bluetooth_hfp_answercall(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_answercall(bt->handle);
}

static int rk_bluetooth_hfp_hangup(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_hangup(bt->handle);
}

static int rk_bluetooth_hfp_dial_num(struct rk_bluetooth *bt, const char *num)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_dial_num(bt->handle, num);
}

static int rk_bluetooth_hfp_last_num_dial(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_last_num_dial(bt->handle);
}

static int rk_bluetooth_hfp_send_dtmf(struct rk_bluetooth *bt, char dtmf)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_send_dtmf(bt->handle, dtmf);
}

static int rk_bluetooth_hfp_set_volume(struct rk_bluetooth *bt, BT_HS_VOLUME_TYPE type, int volume)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_set_volume(bt->handle, type, volume);
}

static int rk_bluetooth_hfp_mute_mic(struct rk_bluetooth *bt, bool mute)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hs_mute_unmute_microphone(bt->handle, mute);
}
/* HFP FUNC END*/

/* HH FUNC BEGIN*/
static int rk_bluetooth_hh_set_listener(struct rk_bluetooth *bt, hh_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_set_listener(bt->handle, listener, listener_handle);
}

static int rk_bluetooth_hh_enable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_enable(bt->handle);
}

static int rk_bluetooth_hh_disable(struct rk_bluetooth *bt)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_disable(bt->handle);
}

static int rk_bluetooth_hh_connect(struct rk_bluetooth *bt, BTAddr bd_addr, int mode)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_open(bt->handle, bd_addr, mode);
}

static int rk_bluetooth_hh_disconnect(struct rk_bluetooth *bt, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_close(bt->handle, bd_addr);
}

static int rk_bluetooth_hh_get_connected_devices(struct rk_bluetooth *bt, BTDevice *dev_array, int arr_len)
{
    BT_CHECK_HANDLE(bt);
    return rokidbt_hh_get_connected_devices(bt->handle, dev_array, arr_len);
}
/* HH FUNC END*/

int rk_bluetooth_creat(struct rk_bluetooth **bt)
{
    int ret = BT_STATUS_NOT_READY;
    struct rk_bluetooth *rk_dev =
            calloc(1, sizeof(struct rk_bluetooth));

    *bt = NULL;
    if (!rk_dev) {
        BT_LOGE("Can not allocate memory for the rk_dev");
        return -BT_STATUS_NOMEM;
    }
    rk_bluetooth_set_bug_level();
    ret = rokidbt_create(&rk_dev->handle);
    if (ret || !rk_dev->handle) {
        ret = -BT_STATUS_FAIL;
       goto creat_err;
    }
    rk_dev->set_manage_listener = rk_bluetooth_set_manage_listener;
    rk_dev->set_discovery_listener = rk_bluetooth_set_discovery_listener;
    rk_dev->open = rk_bluetooth_open;
    rk_dev->close = rk_bluetooth_close;
    rk_dev->set_bt_name = rk_bluetooth_set_bt_name;
    rk_dev->start_discovery = rk_bluetooth_start_discovery;
    rk_dev->cancel_discovery = rk_bluetooth_cancel_discovery;
    rk_dev->remove_dev = rk_bluetooth_remove_device;
    rk_dev->set_visibility = rk_bluetooth_set_visibility;
    rk_dev->get_disc_devices = rk_bluetooth_get_disc_devices;
    rk_dev->get_bonded_devices = rk_bluetooth_get_bonded_devices;
    rk_dev->find_scaned_device = rk_bluetooth_find_scaned_device;
    rk_dev->find_bonded_device = rk_bluetooth_find_bonded_device;
    rk_dev->get_module_addr = rk_bluetooth_get_module_addr;
    rk_dev->config_clear = rk_bluetooth_config_clear;

    rk_dev->ble.set_listener = rk_bluetooth_ble_set_listener;
    rk_dev->ble.enable = rk_bluetooth_ble_enable;
    rk_dev->ble.disable = rk_bluetooth_ble_disable;
    rk_dev->ble.send_buf = rk_bluetooth_ble_send_buf;
    rk_dev->ble.set_ble_visibility = rk_bluetooth_ble_set_visibility;

    rk_dev->blec.set_listener = rk_bluetooth_blec_set_listener;
    rk_dev->blec.enable = rk_bluetooth_blec_enable;
    rk_dev->blec.disable = rk_bluetooth_blec_disable;
    rk_dev->blec.connect = rk_bluetooth_blec_connect;
    rk_dev->blec.disconnect = rk_bluetooth_blec_disconnect;
    rk_dev->blec.search_service = rk_bluetooth_blec_search_service;
    rk_dev->blec.register_notification = rk_bluetooth_blec_register_notification;
    rk_dev->blec.read = rk_bluetooth_blec_read;
    rk_dev->blec.write = rk_bluetooth_blec_write;
    rk_dev->blec.get_connected_devices = rk_bluetooth_blec_get_connected_devices;

    rk_dev->a2dp.set_listener = rk_bluetooth_a2dp_set_listener;
    rk_dev->a2dp.enable = rk_bluetooth_a2dp_enable;
    rk_dev->a2dp.disable = rk_bluetooth_a2dp_disable;
    rk_dev->a2dp.connect = rk_bluetooth_a2dp_connect;
    rk_dev->a2dp.disconnect = rk_bluetooth_a2dp_disconnect;
    rk_dev->a2dp.start = rk_bluetooth_a2dp_start;
    rk_dev->a2dp.send_avrc_cmd = rk_bluetooth_a2dp_send_avrc_cmd;
    rk_dev->a2dp.get_connected_devices = rk_bluetooth_a2dp_get_connected_devices;

    rk_dev->a2dp_sink.set_listener = rk_bluetooth_a2dp_sink_set_listener;
    rk_dev->a2dp_sink.enable = rk_bluetooth_a2dp_sink_enable;
    rk_dev->a2dp_sink.disable = rk_bluetooth_a2dp_sink_disable;
    rk_dev->a2dp_sink.connect = rk_bluetooth_a2dp_sink_connect;
    rk_dev->a2dp_sink.disconnect = rk_bluetooth_a2dp_sink_disconnect;
    rk_dev->a2dp_sink.send_avrc_cmd = rk_bluetooth_a2dp_sink_send_avrc_cmd;
    rk_dev->a2dp_sink.send_get_playstatus = rk_bluetooth_a2dp_sink_send_get_playstatus;
    rk_dev->a2dp_sink.get_playing = rk_bluetooth_a2dp_sink_get_playing;
    rk_dev->a2dp_sink.get_element_attrs = rk_bluetooth_a2dp_sink_get_element_attrs;
    rk_dev->a2dp_sink.get_connected_devices = rk_bluetooth_a2dp_sink_get_connected_devices;
    rk_dev->a2dp_sink.set_mute = rk_bluetooth_a2dp_sink_set_mute;
    rk_dev->a2dp_sink.set_abs_vol = rk_bluetooth_a2dp_sink_set_abs_vol;

    rk_dev->hfp.set_listener = rk_bluetooth_hfp_set_listener;
    rk_dev->hfp.enable = rk_bluetooth_hfp_enable;
    rk_dev->hfp.disable = rk_bluetooth_hfp_disable;
    rk_dev->hfp.connect = rk_bluetooth_hfp_connect;
    rk_dev->hfp.disconnect = rk_bluetooth_hfp_disconnect;
    rk_dev->hfp.answercall = rk_bluetooth_hfp_answercall;
    rk_dev->hfp.hangup = rk_bluetooth_hfp_hangup;
    rk_dev->hfp.dial_num = rk_bluetooth_hfp_dial_num;
    rk_dev->hfp.dial_last_num = rk_bluetooth_hfp_last_num_dial;
    rk_dev->hfp.send_dtmf = rk_bluetooth_hfp_send_dtmf;
    rk_dev->hfp.set_volume = rk_bluetooth_hfp_set_volume;
    rk_dev->hfp.mute_mic = rk_bluetooth_hfp_mute_mic;
    rk_dev->hfp.type = rk_bluetooth_hfp_get_type();

    rk_dev->hh.set_listener = rk_bluetooth_hh_set_listener;
    rk_dev->hh.enable = rk_bluetooth_hh_enable;
    rk_dev->hh.disable = rk_bluetooth_hh_disable;
    rk_dev->hh.connect = rk_bluetooth_hh_connect;
    rk_dev->hh.disconnect = rk_bluetooth_hh_disconnect;
    rk_dev->hh.get_connected_devices = rk_bluetooth_hh_get_connected_devices;

    *bt = rk_dev;
    return 0;

creat_err:
    if (rk_dev)
        free(rk_dev);
    return ret;
}

int rk_bluetooth_destroy(struct rk_bluetooth *bt)
{
    if(bt) {
        if (bt->handle) {
            rokidbt_destroy(bt->handle);
            bt->handle = NULL;
        }
        free(bt);
    }

    return 0;
}

static int bluetooth_device_close(struct hw_device_t* dev)
{
    struct bluetooth_device_t* btdev = (struct bluetooth_device_t*)dev;
    if (btdev) {
        free(btdev);
    }
    return 0;
}

static int bluetooth_device_open(const struct hw_module_t* module,
    const char* name, struct hw_device_t** device)
{
    struct bluetooth_device_t *btdev =
            calloc(1, sizeof(struct bluetooth_device_t));

    if (!btdev) {
        BT_LOGE("Can not allocate memory for the bluetooth device");
        return -BT_STATUS_NOMEM;
    }

    btdev->common.tag = HARDWARE_DEVICE_TAG;
    btdev->common.module = (hw_module_t *) module;
    btdev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);
    btdev->common.close = bluetooth_device_close;

    btdev->creat = rk_bluetooth_creat;
    btdev->destroy= rk_bluetooth_destroy;

    *device = &btdev->common;
    return 0;
}

static struct hw_module_methods_t bluetooth_module_methods = {
    .open = bluetooth_device_open,
};

struct bluetooth_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = BLUETOOTH_HARDWARE_MODULE_ID,
        .name = MODULE_NAME,
        .author = MODULE_AUTHOR,
        .methods = &bluetooth_module_methods,
    },
};

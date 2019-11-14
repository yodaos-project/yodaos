#include <hardware/bt_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>
#include "app_manager.h"
#include "ble_server.h"
#include "a2dp_sink.h"


int rokidbt_get_disc_devices_count(void *handle)
{
    int i, count = 0;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    pthread_mutex_lock(&bt->mutex);
    for (i = 0; i< BT_DISC_NB_DEVICES; ++i) {
        if (bt->discovery_devs[i].in_use) {
            count ++;
        }
    }
    pthread_mutex_unlock(&bt->mutex);
    return count;
}

int rokidbt_get_disc_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    pthread_mutex_lock(&bt->mutex);
    for (i = 0; i< BT_DISC_NB_DEVICES; ++i) {
        if (bt->discovery_devs[i].in_use) {
            if (count < arr_len && dev_array) {
                memcpy(dev_array->bd_addr, bt->discovery_devs[i].device.bd_addr, sizeof(dev_array->bd_addr));
                snprintf(dev_array->name, sizeof(dev_array->name), "%s", bt->discovery_devs[i].device.name);
                dev_array->rssi = bt->discovery_devs[i].device.rssi;
                dev_array ++;
                count++;
            }
        }
        if (count >= arr_len) break;
    }
    pthread_mutex_unlock(&bt->mutex);
    return count;
}

int rokidbt_find_scaned_device(void *handle, BTDevice *dev)
{
    int i = 0;
    int found = 0;
    manager_cxt *bt = (manager_cxt *)handle;

    if (!bt) {
        return 0;
    }

    pthread_mutex_lock(&bt->mutex);
    for (i = 0; i < BT_DISC_NB_DEVICES; i++) {
        if (bdcmp(bt->discovery_devs[i].device.bd_addr, dev->bd_addr) == 0) {
            snprintf(dev->name, sizeof(dev->name), "%s", bt->discovery_devs[i].device.name);
            BT_LOGI("name :: %s rem :: %s", dev->name, bt->discovery_devs[i].device.name);
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&bt->mutex);

    return found;
}

int rokidbt_get_bonded_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);

    return get_paired_devices(bt, dev_array, arr_len);
}

int rokidbt_find_bonded_device(void *handle, BTDevice *dev)
{
    //int i;
    int found = 0;
#if 0
    for (i = 0; i< APP_NUM_ELEMENTS(app_xml_remote_devices_db); ++i)
    {
        tAPP_XML_REM_DEVICE *rem_dev = &app_xml_remote_devices_db[i];

        if (rem_dev->in_use)
        {
            if (bdcmp(dev->bd_addr, rem_dev->bd_addr) == 0)
            {
                snprintf(dev->name, sizeof(dev->name), "%s", rem_dev->name);
                found = 1;
                break;
            }
        }
    }
#endif
    return found;
}

/* A2DP FUN BEGIN */
int rokidbt_a2dp_set_listener(void *handle, a2dp_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);
    return a2dp_set_listener(bt->a2dp_ctx, listener, listener_handle);
}

int rokidbt_a2dp_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dp_enable(bt->a2dp_ctx);
}

int rokidbt_a2dp_open(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return  a2dp_open(bt->a2dp_ctx, bd_addr);
}

int rokidbt_a2dp_start(void *handle, BT_A2DP_CODEC_TYPE type)
{
    //manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    BT_LOGI("do not need user start,  will start auto");
    return 0;
#if 0
    switch (type) {
        case BT_A2DP_CODEC_TYPE_PCM:
            return  a2dp_start(bt->a2dp_ctx);
            break;
        default:
            BT_LOGE("not support  type %d", type);
            break;
    }
    return -1;
#endif
}

int rokidbt_a2dp_close_device(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_close_device(bt->a2dp_ctx, bd_addr);
}

int  rokidbt_a2dp_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_get_connected_devices(bt->a2dp_ctx, dev_array, arr_len);
}

int rokidbt_a2dp_close(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dp_close(bt->a2dp_ctx);
}

int rokidbt_a2dp_disable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_disable(bt->a2dp_ctx);
}

int  rokidbt_a2dp_get_enabled(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_get_enabled(bt->a2dp_ctx);
}

int  rokidbt_a2dp_get_opened(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_get_opened(bt->a2dp_ctx);
}

int rokidbt_a2dp_send_rc_cmd(void *handle, uint8_t cmd)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_send_rc_command(bt->a2dp_ctx, cmd);
}
/* A2DP FUN END*/

/* A2DP SINK FUN BEGIN*/
int rokidbt_a2dp_sink_set_listener(void *handle, a2dp_sink_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);

    return a2dpk_set_listener(bt->a2dp_sink, listener, listener_handle);
}

int rokidbt_a2dp_sink_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_enable(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_open(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_open(bt->a2dp_sink, bd_addr);
}

int rokidbt_a2dp_sink_close_device(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_close_device(bt->a2dp_sink, bd_addr);
}

int rokidbt_a2dp_sink_close(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_close(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_disable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_disable(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_getplaystatus(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_getstatus(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_avrc_cmd(void *handle, uint8_t command)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_cmd(bt->a2dp_sink, command);
}
int rokidbt_a2dp_sink_send_play(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_cmd(bt->a2dp_sink, BT_AVRC_PLAY);
}

int rokidbt_a2dp_sink_send_stop(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_rc_send_cmd(bt->a2dp_sink, BT_AVRC_STOP);
}

int rokidbt_a2dp_sink_send_pause(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_rc_send_cmd(bt->a2dp_sink, BT_AVRC_PAUSE);
}

int rokidbt_a2dp_sink_send_forward(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_cmd(bt->a2dp_sink, BT_AVRC_FORWARD);
}

int rokidbt_a2dp_sink_send_backward(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_rc_send_cmd(bt->a2dp_sink, BT_AVRC_BACKWARD);
}

int rokidbt_a2dp_sink_get_enabled(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_enabled(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_opened(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_get_opened(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_playing(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_playing(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_element_attrs(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_element_attr_command(bt->a2dp_sink);
}

int  rokidbt_a2dp_sink_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_get_connected_devices(bt->a2dp_sink, dev_array, arr_len);
}

int  rokidbt_a2dp_sink_set_mute(void *handle, bool mute)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_set_mute(bt->a2dp_sink, mute);
}

int  rokidbt_a2dp_sink_set_abs_vol(void *handle, uint8_t vol)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_set_abs_vol(bt->a2dp_sink, vol);
}
/* A2DP SINK FUNC END*/


/* HFP HS FUNC BEGIN */
int rokidbt_hs_get_type(void)
{
    return -1;
    //return app_hs_get_type();
}

int rokidbt_hs_set_listener(void *handle, hfp_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);
    return -1;
    //return app_hs_set_listener(bt->hs_ctx, listener, listener_handle);
}

int rokidbt_hs_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_enable(bt->hs_ctx);
}

int rokidbt_hs_disable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_stop(bt->hs_ctx);
}

int rokidbt_hs_open(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return rokid_hs_open(bt->hs_ctx, bd_addr);
}

int rokidbt_hs_close(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_close(bt->hs_ctx);
}

int rokidbt_hs_answercall(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_answer_call(bt->hs_ctx);
}

int rokidbt_hs_hangup(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_hangup(bt->hs_ctx);
}

int rokidbt_hs_dial_num(void *handle, const char *num)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_dial_num(bt->hs_ctx, num);
}

int rokidbt_hs_last_num_dial(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_last_num_dial(bt->hs_ctx);
}

int rokidbt_hs_send_dtmf(void *handle, char dtmf)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_send_dtmf(bt->hs_ctx, dtmf);
}

int rokidbt_hs_set_volume(void *handle, BT_HS_VOLUME_TYPE type, int volume)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_set_volume(bt->hs_ctx, type, volume);
}

int rokidbt_hs_mute_unmute_microphone(void *handle, bool bMute)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;
    //return app_hs_mute_unmute_microphone(bt->hs_ctx, bMute);
}
/* HFP HS FUNC END */

/* BLE FUNC BEGIN */
int rokidbt_ble_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return ble_server_enable(bt->ble_ctx);
}

int rokidbt_ble_disable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return ble_server_disable(bt->ble_ctx);
}

int rokidbt_ble_set_menufacturer_name(void *handle, char *name)
{
    //manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
return -1;
    //return ble_server_set_menufacturer(bt->ble_ctx, name);
}

int rokidbt_ble_get_enabled(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return ble_get_enabled(bt->ble_ctx);
}

int rokidbt_ble_set_visibility(void *handle, bool discoverable, bool connectable)
{
    //manager_cxt *bt = (manager_cxt *)handle;
    //BT_CHECK_HANDLE(bt);
    return 0;
}


int rokidbt_ble_send_buf(void *handle, uint16_t uuid,  char *buf, int len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return ble_send_uuid_buff(bt->ble_ctx, uuid, buf, len);
}

int rokidbt_ble_set_listener(void *handle, ble_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);

    return ble_server_set_listener(bt->ble_ctx, listener, listener_handle);
}
/* BLE FUNC END */

/* BLE CLIENT FUNC BEGIN */
int rokidbt_blec_set_listener(void *handle, ble_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);
    return -1;//ble_client_set_listener(bt->ble_client, listener, listener_handle);
}
int rokidbt_blec_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return -1;//rokid_ble_client_enable(bt->ble_client);
}

int rokidbt_blec_disable(void *handle)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    //hh_disable(bt->hh);
    ret = -1;//rokid_ble_client_disable(bt->ble_client);
    return ret;
}

int rokidbt_blec_connect(void *handle, BTAddr bd_addr)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_open(bt->ble_client, 0, bd_addr, true);
    return ret;
}

int rokidbt_blec_disconnect(void *handle, BTAddr bd_addr)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_close(bt->ble_client, bd_addr);
    return ret;
}

int rokidbt_blec_search_service(void *handle, char *service)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_service_search_execute(bt->ble_client, 0, service);
    return ret;
}

int rokidbt_blec_register_notification(void *handle, char *service_id, char *character_id)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_register_notification(bt->ble_client, 0, service_id, character_id,
                   // 0, 0, true);
    return ret;
}

int rokidbt_blec_read(void *handle, char *service_id, char *character_id)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_read(bt->ble_client, 0, 
                  //      service_id, character_id, 0, 0, true, 0);
    return ret;
}

int rokidbt_blec_write(void *handle, char *service_id, char *character_id, char *buff, int len)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    ret = -1;//app_ble_client_write(bt->ble_client, 0, 
                        //service_id, character_id, 0, 0, true, (UINT8 *)buff, len, NULL, 0);
    return ret;
}

int rokidbt_blec_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;//ble_client_get_connect_devs(bt->ble_client, dev_array, arr_len);
}

/* BLE CLIENT FUNC END */

/* HID HOST FUNC BEGIN */
int rokidbt_hh_set_listener(void *handle, hh_callbacks_t listener, void *listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);
    return -1;//hh_set_listener(bt->hh, listener, listener_handle);
}

int rokidbt_hh_enable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);
    //rokid_ble_client_enable(bt->ble_client);
    return -1;//hh_enable(bt->hh);
}

int rokidbt_hh_disable(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    int ret;
    BT_CHECK_HANDLE(handle);
    ret = -1;//hh_disable(bt->hh);
    //rokid_ble_client_disable(bt->ble_client);
    return ret;
}

int rokidbt_hh_open(void *handle, BTAddr bd_addr, int mode)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;//app_hh_open(bt->hh, bd_addr, mode);
}

int rokidbt_hh_close(void *handle, BTAddr bd_addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;//app_hh_disconnect(bt->hh, bd_addr);
}

int rokidbt_hh_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return -1;//app_hh_get_connected_devices(bt->hh, dev_array, arr_len);
}
/* HID HOST FUNC END */

/* COMMON MANAGE FUNC BEGIN */

int rokidbt_set_visibility(void *handle, bool discoverable, bool connectable)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(bt);

    return bt_set_visibility(bt, discoverable, connectable);
}

int rokidbt_discovery(void *handle, enum bt_profile_type type)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return set_scan(bt, true);
}

int rokidbt_discovery_cancel(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return set_scan(bt, false);
}

int rokidbt_remove_device(void *handle, BTAddr addr)
{
    char address[18] = {0};
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return set_remove_device(bt, address);
}

int rokidbt_clear_devices(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    return set_remove_device(bt, "*");
}

int rokidbt_set_name(void *handle, const char *name)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    BT_LOGD("%s", name);

    return bt_set_name(bt, name);
}

int rokidbt_set_manage_listener(void *handle, manage_callbacks_t listener, void *manage_listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    bt->manage_listener = listener;
    bt->manage_listener_handle = manage_listener_handle;

    return 0;
}

int rokidbt_set_discovery_listener(void *handle, discovery_cb_t listener, void *disc_listener_handle)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);

    bt->disc_listener = listener;
    bt->disc_listener_handle = disc_listener_handle;
    return 0;
}

int rokidbt_get_module_addr(void *handle, BTAddr addr)
{
    manager_cxt *bt = (manager_cxt *)handle;
    BT_CHECK_HANDLE(handle);
    return app_get_module_addr(bt, addr);
}


int rokidbt_open(void *handle, const char *name)
{
    int ret;
    manager_cxt *bt = (manager_cxt *)handle;

    BT_CHECK_HANDLE(handle);
    ret = manager_init(bt);
    if (name)
        bt_set_name(bt, name);

    return ret;
}

int rokidbt_close(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;

    BT_CHECK_HANDLE(handle);

    return manager_deinit(bt);
}
/* COMMON MANAGE FUNC END */

int rokidbt_create(void **handle)
{
    manager_cxt *bt = manager_create();

    if (!bt) {
        BT_LOGE("Can not allocate memory for the rokidbt_create");
        return -BT_STATUS_NOMEM;
    }

    *handle = (void *)bt;
    return 0;
}

int rokidbt_destroy(void *handle)
{
    manager_cxt *bt = (manager_cxt *)handle;

    BT_CHECK_HANDLE(handle);
    return manager_destroy(bt);
}

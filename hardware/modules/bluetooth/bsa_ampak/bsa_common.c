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

#include <bsa_rokid/bsa_api.h>
#include "app_utils.h"
#include "app_xml_utils.h"
#include "app_manager.h"
#include "app_disc.h"
#include "app_dm.h"
#include "app_services.h"
#include "app_mgt.h"

//#define APP_DISC_NB_DEVICES BT_DISC_NB_DEVICES
//#define APP_DEFAULT_UIPC_PATH BT_DEFAULT_UIPC_PATH

#include "a2dp.h"
#include "a2dp_sink.h"
#include "ble_server.h"
#include "hfp_hs.h"
#include "hid_host.h"

RKBluetooth *_global_bt_ctx = NULL;


int rokidbt_get_disc_devices_count(void *handle)
{
    int i, count = 0;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    pthread_mutex_lock(&bt->bt_mutex);
    for (i = 0; i< BT_DISC_NB_DEVICES; ++i)
    {
        if (bt->discovery_devs[i].in_use)
        {
            count ++;
        }
    }
    pthread_mutex_unlock(&bt->bt_mutex);
    return count;
}

int rokidbt_get_disc_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    pthread_mutex_lock(&bt->bt_mutex);
    for (i = 0; i< BT_DISC_NB_DEVICES; ++i)
    {
        if (bt->discovery_devs[i].in_use)
        {
            if (count < arr_len && dev_array)
            {
                bdcpy(dev_array->bd_addr, bt->discovery_devs[i].device.bd_addr);
                snprintf(dev_array->name, sizeof(dev_array->name), "%s", bt->discovery_devs[i].device.name);
                dev_array->rssi = bt->discovery_devs[i].device.rssi;
                dev_array++;
                count++;
            }
        }
        if (count >= arr_len) break;
    }
    pthread_mutex_unlock(&bt->bt_mutex);
    return count;
}

int rokidbt_find_scaned_device(void *handle, BTDevice *dev)
{
    int i = 0;
    int found = 0;
    RKBluetooth *bt = (RKBluetooth *)handle;

    if (!bt) {
        return 0;
    }

    pthread_mutex_lock(&bt->bt_mutex);
    for (i = 0; i < BT_DISC_NB_DEVICES; i++)
    {
        if (bdcmp(bt->discovery_devs[i].device.bd_addr, dev->bd_addr) == 0)
        {
            snprintf(dev->name, sizeof(dev->name), "%s", bt->discovery_devs[i].device.name);
            BT_LOGI("name :: %s", dev->name);
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&bt->bt_mutex);

    return found;
}

int rokidbt_get_bonded_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    pthread_mutex_lock(&bt->bt_mutex);
    for (i = 0; i< APP_NUM_ELEMENTS(app_xml_remote_devices_db); ++i)
    {
        tAPP_XML_REM_DEVICE *rem_dev = &app_xml_remote_devices_db[i];

        if (rem_dev->in_use)
        {
            if (count < arr_len && dev_array)
            {
                BTDevice *dev = dev_array + count;
                snprintf(dev->name, sizeof(dev->name), "%s", rem_dev->name);
                bdcpy(dev->bd_addr, rem_dev->bd_addr);
                count++;
            }
        }
        if (count >= arr_len) break;
    }
    pthread_mutex_unlock(&bt->bt_mutex);
    return count;
}

int rokidbt_find_bonded_device(void *handle, BTDevice *dev)
{
    int i;
    int found = 0;

    for (i = 0; i< APP_NUM_ELEMENTS(app_xml_remote_devices_db); ++i)
    {
        tAPP_XML_REM_DEVICE *rem_dev = &app_xml_remote_devices_db[i];

        if (rem_dev->in_use)
        {
            if (bdcmp(dev->bd_addr, rem_dev->bd_addr) == 0)
            {
                snprintf(dev->name, sizeof(dev->name), "%s", rem_dev->name);
                BT_LOGI("name :: %s", dev->name);
                found = 1;
                break;
            }
        }
    }
    return found;
}

int rokidbt_mgr_sec_unpair(RKBluetooth *bt, BD_ADDR bd_addr)
{
    int status;
    BT_MGT_MSG msg = {0};
    tBSA_SEC_REMOVE_DEV sec_remove;
    tAPP_XML_REM_DEVICE *rem_dev;
    char address[18] = {0};

    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    BT_LOGI("rokidbt_mgr_sec_unpair");

    /* Remove the device from Security database (BSA Server) */
    BSA_SecRemoveDeviceInit(&sec_remove);

    /* Read the XML file which contains all the bonded devices */
    app_read_xml_remote_devices();

    rem_dev = app_find_rem_device_by_bdaddr(bd_addr);
    if (rem_dev == NULL) {
        BT_LOGE("not find in database");
        if (bt->manage_listener) {
            msg.remove.status = 1;
            msg.remove.address = address;
            bt->manage_listener(bt->manage_listener_handle,
                        BT_EVENT_MGR_REMOVE_DEVICE, &msg);
        }
        return 0;
    }

    bdcpy(sec_remove.bd_addr, bd_addr);
    status = BSA_SecRemoveDevice(&sec_remove);

    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecRemoveDevice Operation Failed with status [%d]",status);
         if (bt->manage_listener) {
            msg.remove.status = -2;
            msg.remove.address = address;
            bt->manage_listener(bt->manage_listener_handle,
                        BT_EVENT_MGR_REMOVE_DEVICE, &msg);
        }
        return -1;
    }
    else
    {
        BTDevice dev;
        /* delete the device from database */
        rem_dev->in_use = FALSE;
        app_write_xml_remote_devices();
        if (rokidbt_get_bonded_devices(bt, &dev, 1) == 0) {
            msg.remove.status = 99;//empty
        } else 
            msg.remove.status = 0;

        if (bt->manage_listener) {
            msg.remove.address = address;
            bt->manage_listener(bt->manage_listener_handle,
                        BT_EVENT_MGR_REMOVE_DEVICE, &msg);
        }
    }

    return 0;
}

static void app_management_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    RKBluetooth *bt = _global_bt_ctx;
    BT_MGT_MSG data = {0};

    if(!bt) return;
    switch(event) {
    case BSA_MGT_STATUS_EVT:
        BT_LOGI("### BSA_MGT_STATUS_EVT");
        if (p_data->status.enable) {
            BT_LOGI("Bluetooth restarted => re-initialize the application");
        }
        if (bt->manage_listener) {
            data.connect.enable = p_data->status.enable;
            bt->manage_listener(bt->manage_listener_handle,
                    BT_EVENT_MGR_CONNECT, &data);
        }
        break;

    case BSA_MGT_DISCONNECT_EVT:
        BT_LOGI("BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        if (bt->manage_listener) {
            data.disconnect.reason = p_data->disconnect.reason;
            bt->manage_listener(bt->manage_listener_handle,
                     BT_EVENT_MGR_DISCONNECT, &data);
        }
        break;

    default:
        break;
    }
}

static void discovery_callback(tBSA_DISC_EVT event,
                        tBSA_DISC_MSG *p_data)
{
    RKBluetooth *bt = _global_bt_ctx;
    int index;

    if(!bt) return;
    if (bt->user_dis_cback) {
        bt->user_dis_cback(event, p_data);
    }
    switch (event) {
    /* a New Device has been discovered */
    case BSA_DISC_NEW_EVT:
        pthread_mutex_lock(&bt->bt_mutex);
        /* check if this device has already been received (update) */
        for (index = 0; index < BT_DISC_NB_DEVICES; index++)
        {
            if ((bt->discovery_devs[index].in_use == TRUE) &&
                (!bdcmp(bt->discovery_devs[index].device.bd_addr, p_data->disc_new.bd_addr)))
            {
                /* Update device */
                BT_LOGI("Update device:%d", index);
                bt->discovery_devs[index].device = p_data->disc_new;
                break;
            }
        }
        /* If this is a new device */
        if (index >= BT_DISC_NB_DEVICES)
        {
            /* Look for a free place to store dev info */
            for (index = 0; index < BT_DISC_NB_DEVICES; index++)
            {
                if (bt->discovery_devs[index].in_use == FALSE)
                {
                    BT_LOGI("New Discovered device:%d", index);
                    bt->discovery_devs[index].in_use = TRUE;
                    bt->discovery_devs[index].device = p_data->disc_new;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&bt->bt_mutex);
        /* If this is a new device */
        if (index >= BT_DISC_NB_DEVICES)
        {
            BT_LOGW("No room to save new discovered");
        }

        BT_LOGI("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->disc_new.bd_addr[0],
                p_data->disc_new.bd_addr[1],
                p_data->disc_new.bd_addr[2],
                p_data->disc_new.bd_addr[3],
                p_data->disc_new.bd_addr[4],
                p_data->disc_new.bd_addr[5]);
        BT_LOGI("\tName:%s", p_data->disc_new.name);
        BT_LOGI("\tClassOfDevice:%02x:%02x:%02x",
                p_data->disc_new.class_of_device[0],
                p_data->disc_new.class_of_device[1],
                p_data->disc_new.class_of_device[2]
                );
        BT_LOGI("\tServices:0x%08x (%s)",
                (int) p_data->disc_new.services,
                app_service_mask_to_string(p_data->disc_new.services));
        BT_LOGI("\tRssi:%d\n", p_data->disc_new.rssi);
        if (p_data->disc_new.eir_vid_pid[0].valid)
        {
            BT_LOGI("\tVidSrc:%d Vid:0x%04X Pid:0x%04X Version:0x%04X",
                    p_data->disc_new.eir_vid_pid[0].vendor_id_source,
                    p_data->disc_new.eir_vid_pid[0].vendor,
                    p_data->disc_new.eir_vid_pid[0].product,
                    p_data->disc_new.eir_vid_pid[0].version);
        }

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
        BT_LOGI("\tDeviceType:%s InquiryType:%s AddressType:%s",
                app_disc_device_type_desc(p_data->disc_new.device_type),
                app_disc_inquiry_type_desc(p_data->disc_new.inq_result_type),
                app_disc_address_type_desc(p_data->disc_new.ble_addr_type));
#endif

        if (p_data->disc_new.eir_data[0])
        {
            app_disc_parse_eir(p_data->disc_new.eir_data);
        }
        if (bt->disc_listener) {
            bt->disc_listener(bt->disc_listener_handle, (char *)p_data->disc_new.name, p_data->disc_new.bd_addr, 0, NULL);
        }
        break;

    case BSA_DISC_CMPL_EVT: /* Discovery complete. */
        BT_LOGI("### Discovery complete");
        bt->user_dis_cback = NULL;
        bt->is_discovery = false;
        if (bt->disc_listener) {
            bt->disc_listener(bt->disc_listener_handle, NULL, NULL, 1, NULL);
        }
        break;

    case BSA_DISC_DEV_INFO_EVT: /* Discovery Device Info */
        BT_LOGI("### Discover Device Info status:%d", p_data->dev_info.status);
        BT_LOGI("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x",
                 p_data->dev_info.bd_addr[0],
                 p_data->dev_info.bd_addr[1],
                 p_data->dev_info.bd_addr[2],
                 p_data->dev_info.bd_addr[3],
                 p_data->dev_info.bd_addr[4],
                 p_data->dev_info.bd_addr[5]);
#if 0
        if (p_data->dev_info.status == BSA_SUCCESS)
        {
            UINT8 dev_platform = app_get_dev_platform(p_data->dev_info.vendor,
                        p_data->dev_info.vendor_id_source);

            BT_LOGI("\tDevice VendorIdSource:0x%x", p_data->dev_info.vendor_id_source);
            BT_LOGI("\tDevice Vendor:0x%x", p_data->dev_info.vendor);
            BT_LOGI("\tDevice Product:0x%x", p_data->dev_info.product);
            BT_LOGI("\tDevice Version:0x%x", p_data->dev_info.version);
            BT_LOGI("\tDevice SpecId:0x%x", p_data->dev_info.spec_id);

            if (dev_platform == BSA_DEV_PLATFORM_IOS)
            {
                tBSA_SEC_ADD_SI_DEV si_dev;
                BSA_SecAddSiDevInit(&si_dev);
                bdcpy(si_dev.bd_addr, p_data->dev_info.bd_addr);
                si_dev.platform = dev_platform;
                BSA_SecAddSiDev(&si_dev);

                app_xml_update_si_dev_platform_db(app_xml_si_devices_db,
                    APP_NUM_ELEMENTS(app_xml_si_devices_db),
                    p_data->dev_info.bd_addr, dev_platform);
                app_write_xml_si_devices();
            }
        }
#endif
        break;
    case BSA_DISC_REMOTE_NAME_EVT:
        BT_LOGI("### BSA_DISC_REMOTE_NAME_EVT (status=%d)", p_data->remote_name.status);
        if (p_data->dev_info.status == BSA_SUCCESS)
        {
            BT_LOGI("Read device name for %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->remote_name.bd_addr[0], p_data->remote_name.bd_addr[1],
                p_data->remote_name.bd_addr[2], p_data->remote_name.bd_addr[3],
                p_data->remote_name.bd_addr[4], p_data->remote_name.bd_addr[5]);
            BT_LOGI("Name:%s", p_data->remote_name.remote_bd_name);

            pthread_mutex_lock(&bt->bt_mutex);
            for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
                if ((bt->discovery_devs[index].in_use == TRUE) &&
                    (!bdcmp(bt->discovery_devs[index].device.bd_addr, p_data->remote_name.bd_addr)))
                {
                    /* Update device */
                    BT_LOGI("Update device[%d] name:%s -> %s", index,
                                    bt->discovery_devs[index].device.name,
                                    p_data->remote_name.remote_bd_name);
                    snprintf((char *)bt->discovery_devs[index].device.name,
                                sizeof(bt->discovery_devs[index].device.name), "%s", (char *)p_data->remote_name.remote_bd_name);
                    break;
                }
            }
            pthread_mutex_unlock(&bt->bt_mutex);
        }
        break;

    default:
        BT_LOGI("unknown event:%d", event);
        break;
    }
}

/* A2DP FUN BEGIN */
int rokidbt_a2dp_set_listener(void *handle, a2dp_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return a2dp_set_listener(bt->a2dp_ctx, listener, listener_handle);
}

int rokidbt_a2dp_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dp_enable(bt->a2dp_ctx);
}

int rokidbt_a2dp_open(void *handle, BTAddr bd_addr)
{
    int status;

    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    status =  a2dp_open(bt->a2dp_ctx, bd_addr);

    return status;
}

int rokidbt_a2dp_start(void *handle, BT_A2DP_CODEC_TYPE type)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    switch (type) {
        case BT_A2DP_CODEC_TYPE_PCM:
            return  a2dp_start(bt->a2dp_ctx);
            break;
        case BT_A2DP_CODEC_TYPE_APTX:
            return  a2dp_start_with_aptx(bt->a2dp_ctx);
            break;
        default:
            BT_LOGE("not support  type %d", type);
            break;
    }
    return -1;
}

int rokidbt_a2dp_close_device(void *handle, BTAddr bd_addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_close_device(bt->a2dp_ctx, bd_addr);
}

int  rokidbt_a2dp_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_get_connected_devices(bt->a2dp_ctx, dev_array, arr_len);
}

int rokidbt_a2dp_close(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_close(bt->a2dp_ctx);
}

int rokidbt_a2dp_disable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_disable(bt->a2dp_ctx);
}

int  rokidbt_a2dp_get_enabled(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_is_enabled(bt->a2dp_ctx);
}

int  rokidbt_a2dp_get_opened(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_is_opened(bt->a2dp_ctx);
}

int rokidbt_a2dp_send_rc_cmd(void *handle, uint8_t cmd)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dp_send_rc_command(bt->a2dp_ctx, cmd);
}
/* A2DP FUN END*/

/* A2DP SINK FUN BEGIN*/
int rokidbt_a2dp_sink_set_listener(void *handle, a2dp_sink_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return a2dpk_set_listener(bt->a2dp_sink, listener, listener_handle);
}

int rokidbt_a2dp_sink_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_enable(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_open(void *handle, BTAddr bd_addr)
{
    int status;

    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    status =  a2dpk_open(bt->a2dp_sink, bd_addr);
    return status;
}

int rokidbt_a2dp_sink_close_device(void *handle, BTAddr bd_addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return a2dpk_close_device(bt->a2dp_sink, bd_addr);
}

int rokidbt_a2dp_sink_close(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_close(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_disable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_disable(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_getplaystatus(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_getstatus(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_avrc_cmd(void *handle, uint8_t command)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_cmd(bt->a2dp_sink, command);
}
int rokidbt_a2dp_sink_send_play(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_play(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_stop(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_stop(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_pause(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_pause(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_forward(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_forward(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_send_backward(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_rc_send_backward(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_enabled(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_enabled(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_opened(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_opened(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_playing(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_playing(bt->a2dp_sink);
}

int rokidbt_a2dp_sink_get_element_attrs(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_element_attr_command(bt->a2dp_sink);
}

int  rokidbt_a2dp_sink_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_connected_devices(bt->a2dp_sink, dev_array, arr_len);
}

int  rokidbt_a2dp_sink_set_mute(void *handle, bool mute)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_set_mute(bt->a2dp_sink, mute);
}

int  rokidbt_a2dp_sink_set_abs_vol(void *handle, uint8_t vol)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_set_abs_vol(bt->a2dp_sink, vol);
}

int  rokidbt_a2dp_sink_get_open_pending_addr(void *handle, BTAddr bd_addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return a2dpk_get_open_pending_addr(bt->a2dp_sink, bd_addr);
}

/* A2DP SINK FUNC END*/


/* HFP HS FUNC BEGIN */
int rokidbt_hs_get_type(void)
{
    return app_hs_get_type();
}

int rokidbt_hs_set_listener(void *handle, hfp_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return app_hs_set_listener(bt->hs_ctx, listener, listener_handle);
}

int rokidbt_hs_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_enable(bt->hs_ctx);
}

int rokidbt_hs_disable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_stop(bt->hs_ctx);
}

int rokidbt_hs_open(void *handle, BTAddr bd_addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return rokid_hs_open(bt->hs_ctx, bd_addr);
}

int rokidbt_hs_close(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_close(bt->hs_ctx);
}

int rokidbt_hs_answercall(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_answer_call(bt->hs_ctx);
}

int rokidbt_hs_hangup(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_hangup(bt->hs_ctx);
}

int rokidbt_hs_dial_num(void *handle, const char *num)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_dial_num(bt->hs_ctx, num);
}

int rokidbt_hs_last_num_dial(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_last_num_dial(bt->hs_ctx);
}

int rokidbt_hs_send_dtmf(void *handle, char dtmf)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_send_dtmf(bt->hs_ctx, dtmf);
}

int rokidbt_hs_set_volume(void *handle, BT_HS_VOLUME_TYPE type, int volume)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_set_volume(bt->hs_ctx, type, volume);
}

int rokidbt_hs_mute_unmute_microphone(void *handle, bool bMute)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hs_mute_unmute_microphone(bt->hs_ctx, bMute);
}
/* HFP HS FUNC END */

/* BLE FUNC BEGIN */
int rokidbt_ble_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return rokid_ble_enable(bt->ble_ctx);
}

int rokidbt_ble_disable(void *handle)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = rokid_ble_disable(bt->ble_ctx);
    return ret;
}

int rokidbt_ble_set_menufacturer_name(void *handle, char *name)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return ble_server_set_menufacturer(bt->ble_ctx, name);
}

int rokidbt_ble_get_enabled(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return ble_get_enabled(bt->ble_ctx);
}

int rokidbt_ble_set_visibility(void *handle, bool discoverable, bool connectable)
{
    BT_CHECK_HANDLE(handle);
    return app_dm_set_ble_visibility(discoverable, connectable);
}

int rokidbt_ble_send_buf(void *handle, uint16_t uuid,  char *buf, int len)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    ret = rk_ble_send(bt->ble_ctx, uuid, buf, len);
    return ret;
}

int rokidbt_ble_set_listener(void *handle, ble_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return ble_server_set_listener(bt->ble_ctx, listener, listener_handle);
}
/* BLE FUNC END */

/* BLE CLIENT FUNC BEGIN */
int rokidbt_blec_set_listener(void *handle, ble_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return ble_client_set_listener(bt->ble_client, listener, listener_handle);
}
int rokidbt_blec_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    return rokid_ble_client_enable(bt->ble_client);
}

int rokidbt_blec_disable(void *handle)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    //hh_disable(bt->hh);
    ret = rokid_ble_client_disable(bt->ble_client);
    return ret;
}

int rokidbt_blec_connect(void *handle, BTAddr bd_addr)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_open(bt->ble_client, 0, bd_addr, true);
    return ret;
}

int rokidbt_blec_disconnect(void *handle, BTAddr bd_addr)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_close(bt->ble_client, bd_addr);
    return ret;
}

int rokidbt_blec_search_service(void *handle, char *service)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_service_search_execute(bt->ble_client, 0, service);
    return ret;
}

int rokidbt_blec_register_notification(void *handle, char *service_id, char *character_id)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_register_notification(bt->ble_client, 0, service_id, character_id,
                    0, 0, true);
    return ret;
}

int rokidbt_blec_read(void *handle, char *service_id, char *character_id)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_read(bt->ble_client, 0, 
                        service_id, character_id, 0, 0, true, NULL);
    return ret;
}

int rokidbt_blec_write(void *handle, char *service_id, char *character_id, char *buff, int len)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    ret = app_ble_client_write(bt->ble_client, 0, 
                        service_id, character_id, 0, 0, true, (UINT8 *)buff, len, NULL, 0);
    return ret;
}

int rokidbt_blec_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return ble_client_get_connect_devs(bt->ble_client, dev_array, arr_len);
}

/* BLE CLIENT FUNC END */

/* HID HOST FUNC BEGIN */
int rokidbt_hh_set_listener(void *handle, hh_callbacks_t listener, void *listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    return hh_set_listener(bt->hh, listener, listener_handle);
}

int rokidbt_hh_enable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);
    //rokid_ble_client_enable(bt->ble_client);
    return hh_enable(bt->hh);
}

int rokidbt_hh_disable(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    int ret;
    BT_CHECK_HANDLE(handle);
    ret = hh_disable(bt->hh);
    //rokid_ble_client_disable(bt->ble_client);
    return ret;
}

int rokidbt_hh_open(void *handle, BTAddr bd_addr, int mode)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hh_open(bt->hh, bd_addr, mode);
}

int rokidbt_hh_close(void *handle, BTAddr bd_addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hh_disconnect(bt->hh, bd_addr);
}

int rokidbt_hh_get_connected_devices(void *handle, BTDevice *dev_array, int arr_len)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    return app_hh_get_connected_devices(bt->hh, dev_array, arr_len);
}
/* HID HOST FUNC END */


/* COMMON MANAGE FUNC BEGIN */

int rokidbt_set_visibility(void *handle, bool discoverable, bool connectable)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);

    return app_dm_set_visibility(discoverable, connectable);
}


 int rokidbt_disc_read_remote_device_name(void *handle, BTAddr bd_addr, tBSA_TRANSPORT transport, 
                                                            tBSA_DISC_CBACK *user_dis_cback)
{
    tBSA_STATUS status;
    tBSA_DISC_READ_REMOTE_NAME disc_read_remote_name_param;
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);

    bt->user_dis_cback = user_dis_cback;
    BSA_ReadRemoteNameInit(&disc_read_remote_name_param);
    disc_read_remote_name_param.cback = discovery_callback;
    disc_read_remote_name_param.transport = transport;
    bdcpy(disc_read_remote_name_param.bd_addr, bd_addr);

    status = BSA_ReadRemoteName(&disc_read_remote_name_param);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_ReadRemoteName failed status:%d", status);
        return -1;
    }
    return 0;
}


int rokidbt_discovery_start(void *handle, int max_num, BTAddr bd_addr,
                                    tBSA_SERVICE_MASK services, int mode, unsigned char major, unsigned char minor, tBSA_DISC_CBACK *user_dis_cback)
{
    int status;
    tBSA_DISC_START disc_start_param;

    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(bt);

    if (bt->is_discovery) {
        BT_LOGW("already have Discovery, not complete!");
        return -1;
    }
    BT_LOGD("Discovery");
    BSA_DiscStartInit(&disc_start_param);

    bt->user_dis_cback = user_dis_cback;
    disc_start_param.cback = discovery_callback;
    disc_start_param.nb_devices = max_num;
    disc_start_param.duration = 4;

    disc_start_param.mode = mode;
    if (services != 0)
        disc_start_param.services = services;

    if (bd_addr) {
        disc_start_param.nb_devices = 1;
        /* Filter on class of device */
        disc_start_param.filter_type = BTA_DM_INQ_BD_ADDR;
        bdcpy(disc_start_param.filter_cond.bd_addr, bd_addr);
    }
    if (major || minor) {
        DEV_CLASS dev_class;
        DEV_CLASS dev_class_mask;
        unsigned char major_mask = 0;
        unsigned char minor_mask = 0;
        disc_start_param.filter_type = BSA_DM_INQ_DEV_CLASS;
        FIELDS_TO_COD(dev_class, minor, major, services);
        BT_LOGI("COD: 0x%02x 0x%02x 0x%02x", dev_class[0], dev_class[1],dev_class[2]);
        memcpy(disc_start_param.filter_cond.dev_class_cond.dev_class,
            dev_class, sizeof(DEV_CLASS));
        if (major)
            major_mask = BTM_COD_MAJOR_CLASS_MASK;
        if (minor)
            minor_mask = BTM_COD_MINOR_CLASS_MASK;
        FIELDS_TO_COD(dev_class_mask, minor_mask, major_mask, services);
        BT_LOGI("COD Mask: 0x%02x 0x%02x 0x%02x", dev_class_mask[0], dev_class_mask[1],dev_class_mask[2]);
        memcpy(disc_start_param.filter_cond.dev_class_cond.dev_class_mask,
            dev_class_mask, sizeof(DEV_CLASS));
    }

    if (max_num > 1)
        memset(bt->discovery_devs, 0, sizeof(bt->discovery_devs));

    status = BSA_DiscStart(&disc_start_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DiscStart failed status:%d", status);
        return -1;
    }
    bt->is_discovery = true;
    return 0;
}

int rokidbt_discovery(void *handle, enum bt_profile_type type)
{
    int ret = -1;
    BT_CHECK_HANDLE(handle);
    switch (type) {
        case BT_A2DP_SOURCE:
            BT_LOGI("Discovery source devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                BTA_A2DP_SRC_SERVICE_MASK, BSA_DM_GENERAL_INQUIRY, 0, 0, NULL);
            break;
        case BT_A2DP_SINK:
            BT_LOGI("Discovery sink devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                BTA_A2DP_SERVICE_MASK, BSA_DM_GENERAL_INQUIRY, 0, 0, NULL);
            break;
        case BT_BLE:
            BT_LOGI("Discovery le devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                0, BSA_BLE_GENERAL_INQUIRY, 0, 0, NULL);
            break;
        case BT_HS:
            BT_LOGI("Discovery hsp hfp & source devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                BTA_A2DP_SRC_SERVICE_MASK | BTA_HFP_HS_SERVICE_MASK |BTA_HSP_HS_SERVICE_MASK,
                                BSA_DM_GENERAL_INQUIRY, 0, 0, NULL);
            break;
        case BT_BR_EDR:
            BT_LOGI("Discovery br/edr devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                0, BSA_DM_GENERAL_INQUIRY, 0, 0, NULL);
            break;
        case BT_PERIPHERAL:
            BT_LOGI("Discovery peripheral devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                0, BSA_DM_GENERAL_INQUIRY,
                                BTM_COD_MAJOR_PERIPHERAL, 0, NULL);
            break;
        default:
            BT_LOGI("Discovery all devices");
            ret = rokidbt_discovery_start(handle, BT_DISC_NB_DEVICES, NULL,
                                0, BSA_DM_GENERAL_INQUIRY |BSA_BLE_GENERAL_INQUIRY,
                                0, 0, NULL);
            break;
    }
    return ret;
}

static int app_disc_abort(void)
{
    tBSA_STATUS status;
    tBSA_DISC_ABORT disc_abort_param;

    BT_LOGI("Aborting Discovery");

    BSA_DiscAbortInit(&disc_abort_param);
    status = BSA_DiscAbort(&disc_abort_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DiscAbort failed status:%d", status);
        return -1;
    }
    return 0;
}

int rokidbt_discovery_cancel(void *handle)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;

    BT_CHECK_HANDLE(handle);
    ret = app_disc_abort();
    bt->is_discovery = false;
    return ret;
}

int rokidbt_remove_device(void *handle, BTAddr addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    /*fist disconnect if connected*/
    rokidbt_hh_close(handle, addr);
    rokidbt_blec_disconnect(handle, addr);
    rokidbt_hs_close(handle);
    rokidbt_a2dp_close_device(handle, addr);
    rokidbt_a2dp_sink_close_device(handle, addr);
    /*remove db*/
    app_hh_remove_dev(bt->hh, addr);
    app_ble_client_unpair(bt->ble_client, addr);
    return rokidbt_mgr_sec_unpair(bt, addr);
}

int rokidbt_clear_devices(void *handle)
{
    //RKBluetooth *bt = (RKBluetooth *)handle;
    BTDevice dev_array[APP_MAX_NB_REMOTE_STORED_DEVICES];
    int num, i;
    BT_CHECK_HANDLE(handle);

    num = rokidbt_get_bonded_devices(handle, dev_array, APP_MAX_NB_REMOTE_STORED_DEVICES);
    for (i=0; i<num; i++) {
        rokidbt_remove_device(handle, dev_array[0].bd_addr);
    }
    return 0;
}

int rokidbt_set_name(void *handle, const char *name)
{
    int i= 0;
    int ret = BT_STATUS_NOT_READY;

    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    BT_LOGD("%s", name);
    for (i = 0; i < 10; ++i)
    {
        if (bt->mgt_opened)
            break;
        usleep(1000 * 50);
    }
    if (bt->mgt_opened)
    {
        ret = app_mgr_set_bt_name(name);
    }
    return ret;
}

int rokidbt_set_manage_listener(void *handle, manage_callbacks_t listener, void *manage_listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    bt->manage_listener = listener;
    bt->manage_listener_handle = manage_listener_handle;

    return 0;
}

int rokidbt_set_discovery_listener(void *handle, discovery_cb_t listener, void *disc_listener_handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);

    bt->disc_listener = listener;
    bt->disc_listener_handle = disc_listener_handle;
    return 0;
}

// connect to bsa server
static int bsa_mgt_open(RKBluetooth *bt)
{
    tBSA_MGT_OPEN bsa_open_param;
    tBSA_STATUS bsa_status;
    int i;

    BSA_MgtOpenInit(&bsa_open_param);
    snprintf(bsa_open_param.uipc_path, sizeof(bsa_open_param.uipc_path), BT_DEFAULT_UIPC_PATH);

    /* Use the Generic Callback */
    bsa_open_param.callback = app_management_callback;

    /* Let's try to connect several time */
    for (i = 0 ; i < 5 ; i++)
    {
        /* Connect to BSA Server */
        bsa_status = BSA_MgtOpen(&bsa_open_param);
        if (bsa_status == BSA_SUCCESS)
        {
            break;
        }
        BT_LOGE("Connection to server unsuccessful (%d), retrying...", i);
        sleep(1);
    }
    if (bsa_status != BSA_SUCCESS)
    {
        BT_LOGE("Unable to connect to server");
        return -1;
    }

    return 0;
}

int rokidbt_get_module_addr(void *handle, BTAddr addr)
{
    RKBluetooth *bt = (RKBluetooth *)handle;
    BT_CHECK_HANDLE(handle);
    if (bt->mgt_opened)
        return app_get_module_addr(addr);
    else
        return -1;
}

int rokidbt_open(void *handle, const char *name)
{
    int ret;
    RKBluetooth *bt = (RKBluetooth *)handle;

    BT_CHECK_HANDLE(handle);
    if (bt->mgt_opened) {
        BT_LOGW("already open");
        return 0;
    }
    BT_LOGI("ampak bsa server");
    ret = app_mgt_init();
    ret = bsa_mgt_open(bt);
    if (ret) {
        BT_LOGE("connect to bsa server failed");
        bt->mgt_opened = false;
        return -1;
    }
    app_mgr_config(name);

    usleep(100 * 1000);
    bt->mgt_opened = true;

    return ret;
}

int rokidbt_close(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;

    BT_CHECK_HANDLE(handle);

    if (bt->mgt_opened) {
        hh_disable(bt->hh);
        rokid_ble_disable(bt->ble_ctx);
        rokid_ble_client_disable(bt->ble_client);
        app_hs_stop(bt->hs_ctx);
        a2dp_disable(bt->a2dp_ctx);
        a2dpk_disable(bt->a2dp_sink);
        rokidbt_set_visibility(bt, false, false);
        usleep(10 * 1000);
        tBSA_MGT_CLOSE bsa_close_param;
        BSA_MgtCloseInit(&bsa_close_param);
        BSA_MgtClose(&bsa_close_param);
        usleep(100 * 1000);
        bt->mgt_opened = false;
    }

    return 0;
}

/* COMMON MANAGE FUNC END */

int rokidbt_create(void **handle)
{
    RKBluetooth *bt = calloc(1, sizeof(*bt));

    if (!bt) {
        BT_LOGE("Can not allocate memory for the rokidbt_create");
        return -BT_STATUS_NOMEM;
    }
    bt->ble_ctx = ble_create(bt);
    bt->a2dp_ctx = a2dp_create(bt);
    bt->a2dp_sink = a2dpk_create(bt);
    bt->hs_ctx = hs_create(bt);
    bt->hh = hh_create(bt);
    bt->ble_client = blec_create(bt);
    pthread_mutex_init(&bt->bt_mutex, NULL);
    *handle = (void *)bt;
    _global_bt_ctx = bt;
    return 0;
}

int rokidbt_destroy(void *handle)
{
    RKBluetooth *bt = (RKBluetooth *)handle;

    BT_CHECK_HANDLE(handle);

    rokidbt_close(handle);
    if (bt->hs_ctx) {
        hs_destroy(bt->hs_ctx);
        bt->hs_ctx = NULL;
    }
    if (bt->a2dp_ctx) {
        a2dp_destroy(bt->a2dp_ctx);
        bt->a2dp_ctx = NULL;
    }
    if (bt->a2dp_sink) {
        a2dpk_destroy(bt->a2dp_sink);
        bt->a2dp_sink = NULL;
    }

    if (bt->ble_ctx) {
        ble_destroy(bt->ble_ctx);
        bt->ble_ctx = NULL;
    }

    if (bt->ble_client) {
        blec_destroy(bt->ble_client);
        bt->ble_client = NULL;
    }

    if (bt->hh) {
        hh_destroy(bt->hh);
        bt->hh = NULL;
    }
    pthread_mutex_destroy(&bt->bt_mutex);
    free(bt);
    _global_bt_ctx = NULL;

    return 0;
}

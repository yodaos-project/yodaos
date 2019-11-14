#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "app_manager.h"
#include "shared/ad.h"
#include "ble_server.h"

#if 0
static struct  ad_register_data ad_data;
static bool ad_complete = false;

int create_ble_net_services(BleServer *bles)
{
    const char *chrc_flag = "read,write,notify";
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;
    gatt_register_service(dbus_conn, NULL, BLE_SERVICE_UUID);
    gatt_register_chrc_full(dbus_conn, NULL, BLE_CHRC_UUID1, chrc_flag, NULL, 0);
    gatt_register_chrc_full(dbus_conn, NULL, BLE_CHRC_UUID2, chrc_flag, NULL, 0);
    return 0;
}

int destroy_ble_net_services(BleServer *bles)
{
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    gatt_unregister_chrc(dbus_conn, NULL, BLE_CHRC_UUID1);
    gatt_unregister_chrc(dbus_conn, NULL, BLE_CHRC_UUID2);
    gatt_unregister_service(dbus_conn, NULL, BLE_SERVICE_UUID);

    return 0;
}

int ad_register_complete_cb(void *userdata, int status)
{
    //BleServer *bles = userdata;

    BT_LOGI("entry");
    if (status)
        BT_LOGE("failed to ad_register");
    ad_complete = true;
    return status;
}

int ad_unregister_complete_cb(void *userdata, int status)
{
    //BleServer *bles = userdata;

    BT_LOGI("entry");
    if (status)
        BT_LOGE("failed to ad_unregister");
    ad_complete = false;
    return status;
}
#endif


static void ble_device_added(BleServer *bles, GDBusProxy *proxy, void *user_data)
{
    DBusMessageIter iter;
    struct adapter *adapter = NULL;
    manager_cxt *manager = user_data;

    if (!manager) {
        return;
    }
    if (!bles->enabled) return;
    if (bles->connected) return;
    adapter = find_parent(manager->ctrl_list, proxy);

    if (!adapter) {
        /* TODO: Error */
        return;
    }


    if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
        BT_BLE_MSG msg = {0};
        dbus_bool_t connected;

        dbus_message_iter_get_basic(&iter, &connected);
        if (bles->connected != connected && connected) {
            if (bles->listener) {
                msg.ser_open.reason = 0;
                msg.ser_open.conn_id = 0;
                bles->listener(bles->listener_handle,
                        BT_BLE_SE_OPEN_EVT, &msg);
            }
            bles->connected = connected;
        }
    }
}

void ble_gatt_add_service(BleServer *bles, GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *uuid;

    if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == TRUE) {
        dbus_message_iter_get_basic(&iter, &uuid);
        if (!strcmp(uuid, GATT_UUID)) {
            BT_LOGD("add remote gatt service");
            //bles->gatt_service_ready = true;
        }
    }
    gatt_add_service(proxy);
}

void ble_gatt_remove_service(BleServer *bles, GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *uuid;

    if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == TRUE) {
        dbus_message_iter_get_basic(&iter, &uuid);
        if (!strcmp(uuid, GATT_UUID)) {
            BT_LOGD("remove remote gatt service");
            //bles->gatt_service_ready = false;
        }
    }
    gatt_remove_service(proxy);
}

void ble_proxy_added(BleServer *bles, GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    manager_cxt *manager = user_data;

    if (!manager) return;
    if (!bles) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, DEVICE_INTERFACE)) {
        ble_device_added(bles, proxy, user_data);
    }
    else if (!strcmp(interface, SERVICE_INTERFACE)) {
        if (service_is_child(manager, proxy)) {
            ble_gatt_add_service(bles, proxy);
        }
    } else if (!strcmp(interface, CHRC_INTERFACE)) {
        gatt_add_characteristic(proxy);
    } else if (!strcmp(interface, DESC_INTERFACE)) {
        gatt_add_descriptor(proxy);
    }
}


void ble_proxy_removed(BleServer *bles, GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    manager_cxt *manager = user_data;

    if (!manager) return;
    if (!bles) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, SERVICE_INTERFACE)) {
        ble_gatt_remove_service(bles, proxy);
        if (manager->proxy.default_attr == proxy)
            set_default_attribute(manager, NULL);
    } else if (!strcmp(interface, CHRC_INTERFACE)) {
        gatt_remove_characteristic(proxy);
        if (manager->proxy.default_attr == proxy)
            set_default_attribute(manager, NULL);
    } else if (!strcmp(interface, DESC_INTERFACE)) {
        gatt_remove_descriptor(proxy);
        if (manager->proxy.default_attr == proxy)
            set_default_attribute(manager, NULL);
    }
}

void ble_property_changed(BleServer *bles, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
    const char *interface;
    manager_cxt *manager = user_data;
    BT_BLE_MSG msg = {0};

    if (!manager) return;
    if (!bles) return;
    if (!bles->enabled) return;

    interface = g_dbus_proxy_get_interface(proxy);
    if (!strcmp(interface, DEVICE_INTERFACE)) {
        if (manager->proxy.default_ctrl && device_is_child(proxy,
                    manager->proxy.default_ctrl->proxy) == TRUE) {
            if (strcmp(name, "Connected") == 0) {
        #if 0
                DBusMessageIter addr_iter;

                const char *address = NULL;

                if (g_dbus_proxy_get_property(proxy, "Address",
                                &addr_iter) == TRUE) {
                    dbus_message_iter_get_basic(&addr_iter,
                                        &address);
                }
        #endif
                dbus_bool_t connected;
                dbus_message_iter_get_basic(iter, &connected);

                if (bles->connected != connected) {
                    if (bles->listener) {
                        msg.ser_open.reason = 0;
                        msg.ser_open.conn_id = 0;
                        bles->listener(bles->listener_handle,
                                    connected ? BT_BLE_SE_OPEN_EVT : BT_BLE_SE_CLOSE_EVT,
                                    &msg);
                    }
                    bles->connected = connected;
                }
            }
        }
    }
    else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
    }
}

static size_t calc_max_adv_len(uint32_t flags)
{
	size_t max = MAX_ADV_DATA_LEN;

	/*
	 * Flags which reduce the amount of space available for advertising.
	 * See doc/mgmt-api.txt
	 */
	if (flags & MGMT_ADV_FLAG_TX_POWER)
		max -= 3;

	if (flags & (MGMT_ADV_FLAG_DISCOV | MGMT_ADV_FLAG_LIMITED_DISCOV |
						MGMT_ADV_FLAG_MANAGED_FLAGS))
		max -= 3;

	if (flags & MGMT_ADV_FLAG_APPEARANCE)
		max -= 4;

	return max;
}

static void ble_add_adv_callback(uint8_t status, uint16_t length,
					  const void *param, void *user_data)
{
    BleServer *bles = user_data;
    const struct mgmt_rp_add_advertising *rp = param;

    if (status)
        goto err_exit;

    if (!param || length < sizeof(*rp)) {
        status = MGMT_STATUS_FAILED;
        goto err_exit;
    }

    bles->ad_completed = true;
    bles->enabled = true;
    BT_LOGI("enable done");
    return;
err_exit:
    BT_LOGE("faile to add ble adv %d", status);
    bles->ad_completed = true;
    bles->enabled = false;
}

static int ble_adv_add(BleServer *bles, mgmt_request_func_t func)
{
    manager_cxt *manager = NULL;
    struct mgmt_cp_add_advertising *cp = NULL;
    struct bt_ad *data = NULL;
    struct bt_ad *scan = NULL;
    bt_uuid_t uuid;
    uint8_t param_len;
    uint8_t *adv_data = NULL;
    size_t adv_data_len;
    uint8_t *scan_rsp = NULL;
    size_t scan_rsp_len = -1;
    uint32_t flags = 0;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    flags = MGMT_ADV_FLAG_CONNECTABLE | MGMT_ADV_FLAG_DISCOV;

    data = bt_ad_new();
    if (!data)
        goto fail;

    scan = bt_ad_new();
    if (!scan)
        goto fail;

    uuid.type = BT_UUID16;
    uuid.value.u16 = 0xffe1;
    if (!bt_ad_add_service_uuid(data, &uuid))
        goto fail;
    adv_data = bt_ad_generate(data, &adv_data_len);
    if (!adv_data || (adv_data_len > calc_max_adv_len(flags))) {
        BT_LOGE("Advertising data too long or couldn't be generated.");
        goto fail;
    }

    BT_LOGI("%s", manager->bt_name);
    bt_ad_add_name(scan, manager->bt_name);
    scan_rsp = bt_ad_generate(scan, &scan_rsp_len);
    if (!scan_rsp && scan_rsp_len) {
        BT_LOGE("Scan data couldn't be generated.");
        goto fail;
    }

    param_len = sizeof(struct mgmt_cp_add_advertising) + adv_data_len +
							scan_rsp_len;

    cp = malloc0(param_len);
    if (!cp) {
        BT_LOGE("Couldn't allocate for MGMT!");
        goto fail;
    }

    cp->flags = htobl(flags);
    cp->instance = 1;
    cp->duration = 0;
    cp->adv_data_len = adv_data_len;
    cp->scan_rsp_len = scan_rsp_len;
    memcpy(cp->data, adv_data, adv_data_len);
    memcpy(cp->data + adv_data_len, scan_rsp, scan_rsp_len);

    free(adv_data);
    adv_data = NULL;
    free(scan_rsp);
    scan_rsp = NULL;

    if (!mgmt_send(manager->mgmt, MGMT_OP_ADD_ADVERTISING,
				0, param_len, cp, func, bles, NULL)) {
        BT_LOGE("Failed to add Advertising Data");
        goto fail;
    }
    free(cp);
    cp = NULL;

    bt_ad_unref(data);
    data = NULL;
    bt_ad_unref(scan);
    scan = NULL;

    return 0;
fail:

    if (cp) free(cp);
    if (adv_data) free(adv_data);
    if (scan_rsp) free(scan_rsp);
    bt_ad_unref(data);
    bt_ad_unref(scan);
    return -1;
}

static int ble_remove_advertising(BleServer *bles,
						uint8_t instance)
{
    struct mgmt_cp_remove_advertising cp;
    manager_cxt *manager = NULL;
 
    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    if (instance)
        BT_LOGD("instance %u", instance);
    else
        BT_LOGD("all instances");

    cp.instance = instance;

    return mgmt_send(manager->mgmt, MGMT_OP_REMOVE_ADVERTISING,
                    0, sizeof(cp), &cp, NULL, NULL, NULL);
}


static int ble_change_to_le(BleServer *bles)
{
    manager_cxt *manager = NULL;
    uint8_t val_true = 1;
    uint8_t val_false = 0;
    int times = 20;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    if (bles->le) return 0;
    do {
        //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
        //                sizeof(val_false), &val_false);
        set_power(manager, false);
        usleep(100 * 1000);//wait power off
    } while (manager->powered && times--);
    send_cmd(manager->mgmt, MGMT_OP_SET_BONDABLE, 0,
                        sizeof(val_false), &val_false);//bondable off
    usleep(30 * 1000);
    send_cmd(manager->mgmt, MGMT_OP_SET_LE, 0,
                        sizeof(val_true), &val_true);//le on
    usleep(30 * 1000);
    send_cmd(manager->mgmt, MGMT_OP_SET_SECURE_CONN, 0,
                    sizeof(val_true), &val_true);//sc on
    usleep(30 * 1000);

    send_cmd(manager->mgmt, MGMT_OP_SET_BREDR, 0,
                    sizeof(val_false), &val_false);//bredr off
    usleep(30 * 1000);
    times = 20;
    do {
        //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
        //            sizeof(val_true), &val_true);
        set_power(manager, true);
        usleep(100 * 1000);//wait power on
    } while (!manager->powered && times--);
    BT_LOGI("done");
    bles->le = true;
    return 0;
}

int ble_change_to_bredr(BleServer *bles)
{
    manager_cxt *manager = NULL;
    uint8_t val_true = 1;
    uint8_t val_false = 0;
    int times = 30;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    if (!bles->le) return 0;
    do {
        //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
        //                sizeof(val_false), &val_false);
        set_power(manager, false);
        usleep(100 * 1000);//wait power off
    } while (manager->powered && times--);
    send_cmd(manager->mgmt, MGMT_OP_SET_BREDR, 0,
                        sizeof(val_true), &val_true);//bredr on
    usleep(30 * 1000);
    send_cmd(manager->mgmt, MGMT_OP_SET_SSP, 0,
                        sizeof(val_true), &val_true);//ssp on
    usleep(30 * 1000);
    send_cmd(manager->mgmt, MGMT_OP_SET_LE, 0,
                        sizeof(val_false), &val_false);//le off
    usleep(30 * 1000);
    times = 20;
    do {
        //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
        //            sizeof(val_true), &val_true);
        set_power(manager, true);
        usleep(100 * 1000);//wait power on
    } while (!manager->powered && times--);
    BT_LOGI("done");
    bles->le = false;
    return 0;
}

int ble_server_enable(BleServer *bles)
{
    manager_cxt *manager = NULL;
    int ret;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    if (bles->enabled) return 0;
    if (!manager->init || !check_default_ctrl(manager))
        return -1;

    ble_remove_advertising(bles, 0);//clear existing
    ble_change_to_le(bles);
    // register advertising
    //BT_LOGD("do register adv");
#if 0
    ad_advertise_uuids(dbus_conn, BLE_SERVICE_UUID);
    ad_advertise_tx_power(dbus_conn, true);
    ad_advertise_local_name(dbus_conn, manager->bt_name);

    ad_data.complete = ad_register_complete_cb;
    ad_data.conn= dbus_conn;
    ad_data.userdata = bles;

    ad_register(dbus_conn, manager->proxy.default_ctrl->ad_proxy, "peripheral", &ad_data);
    while(!ad_complete && times--)
    //while(!ad_complete)
        usleep(100 * 1000);//wait ad_register

    //system("hcitool -i hci0 cmd 0x08 0x0008 07 02 01 06 03 03 E1 FF");//set ad data
    bles->enabled = true;
    return 0;
#else
    bles->ad_completed = false;
    ret = ble_adv_add(bles, ble_add_adv_callback);
    if (ret) {
        ble_change_to_bredr(bles);
        return ret;
    }
    while (!bles->ad_completed) {
        usleep(30 * 1000);//wait adv add completed
    }
    return bles->enabled ? 0 : -1;
#endif
}

int ble_server_disable(BleServer *bles)
{
    int times =50;
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(bles);
    manager = bles->caller;

    if (bles->enabled) {
        if (!check_default_ctrl(manager))
            return -1;
        BT_LOGI("disable");
        if (bles->connected) {
            disconn_addr(manager, "");
            while (bles->connected && times--)
                usleep(100 * 1000);
            bles->connected = false;
        }
#if 0
        ad_data.complete = ad_unregister_complete_cb;
        ad_data.conn= dbus_conn;
        ad_data.userdata = bles;

        ad_unregister(dbus_conn, manager->proxy.default_ctrl->ad_proxy, &ad_data);
        times = 20;
        while(ad_complete && times--)
        //while(ad_complete)
            usleep(100 * 1000);//wait ad_unregister
        if (ad_complete) {
            BT_LOGW("ad_unregister but not repply");
            ad_unregister(dbus_conn, NULL, NULL);
            ad_complete = false;
        }
#else
        ble_remove_advertising(bles, 0);
        ble_change_to_bredr(bles);
#endif
        bles->enabled = false;
    }
    return 0;
}

int ble_send_uuid_buff(BleServer *bles, uint16_t uuid, char *buff, int len)
{
    int ret = -BT_STATUS_NOT_READY;

    BT_LOGD("uuid: 0x%x,buf: %s", uuid, buff);
    BT_CHECK_HANDLE(bles);
    if (uuid == 0x2a06)
        ret = chrc_send(BLE_CHRC_UUID1, (const uint8_t *)buff, len);
    else if (uuid == 0x2a07)
        ret = chrc_send(BLE_CHRC_UUID2, (const uint8_t *)buff, len);

    return ret;
}

BleServer *ble_create(void *caller)
{
    BleServer *bles = calloc(1, sizeof(*bles));

    bles->caller = caller;

    return bles;
}

int ble_destroy(BleServer *bles)
{
    if (bles) {
        //ble_server_disable(bles);
        free(bles);
    }

    return 0;
}

int ble_server_set_listener(BleServer *bles, ble_callbacks_t listener, void *listener_handle)
{

    BT_CHECK_HANDLE(bles);
    gatt_set_event_listener(listener, listener_handle);
    bles->listener = listener;
    bles->listener_handle = listener_handle;
   return 0;
}

int ble_get_enabled(BleServer *bles)
{
    return bles ? bles->enabled : 0;
}

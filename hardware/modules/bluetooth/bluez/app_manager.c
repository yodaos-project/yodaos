/*****************************************************************************
**
**  Name:           app_manager.c
**
**  Description:    Bluetooth Manager application
**
**  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>
#include <sys/time.h>
#include <hardware/bt_common.h>
#include <bluetooth/bluetooth.h>

#include "shared/util.h"
#include "gdbus/gdbus.h"
//#include "client/display.h"
#include "lib/uuid.h"
#include "client/agent.h"
#include "client/gatt.h"
#include "client/advertising.h"

#include "app_manager.h"


manager_cxt *_global_manager_ctx = NULL;

static pthread_t main_tid;
static bool       main_thread_running;
static bool       main_thread_close_normal;
static GMainLoop *main_loop;
static BTAddr MODULE_BD_ADDR;
static connect_fail_cb failed_connect_callback = NULL;
static void  *failed_connect_callback_data = NULL;

#define AUTO_REGISTER_AGENT  "NoInputNoOutput"

static void show_version(void)
{
    BT_LOGI("Version %s\n", VERSION);
}

gboolean check_default_ctrl(manager_cxt *manager)
{

    if (!manager || !manager->proxy.default_ctrl) {
        BT_LOGI("No default controller available\n");
        return FALSE;
    }

    return TRUE;
}

struct GDBusProxy *find_device(manager_cxt *manager, const char *address)
{
    GDBusProxy *proxy;

    if (!manager) return NULL;
    if (!address || !strlen(address)) {
        if (manager->proxy.default_dev)
            return manager->proxy.default_dev;
        BT_LOGI("Missing device address argument\n");
        return NULL;
    }

    if (check_default_ctrl(manager) == FALSE)
        return NULL;

    proxy = find_proxy_by_address(manager->proxy.default_ctrl->devices, address);
    if (!proxy) {
        BT_LOGI("Device %s not available\n", address);
        return NULL;
    }

    return proxy;
}

struct adapter *find_ctrl_by_address(GList *source, const char *address)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = list->data;
        DBusMessageIter iter;
        const char *str;

        if (g_dbus_proxy_get_property(adapter->proxy,
                        "Address", &iter) == FALSE)
        continue;

        dbus_message_iter_get_basic(&iter, &str);

        if (!strcasecmp(str, address))
            return adapter;
    }

    return NULL;
}


void show_controller_info(manager_cxt *manager, const char *addr)
{
    struct adapter *adapter;
    GDBusProxy *proxy;
    DBusMessageIter iter;
    const char *address;

    if (!manager) return;
    if (!addr || !strlen(addr)) {
        if (check_default_ctrl(manager) == FALSE)
            return;

        proxy = manager->proxy.default_ctrl->proxy;
    } else {
        adapter = find_ctrl_by_address(manager->ctrl_list, addr);
        if (!adapter) {
            BT_LOGI("Controller %s not available\n", addr);
            return;
        }
        proxy = adapter->proxy;
    }

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &address);
    BT_LOGI("Controller %s\n", address);

    print_property(proxy, "Name");
    print_property(proxy, "Alias");
    print_property(proxy, "Class");
    print_property(proxy, "Powered");
    print_property(proxy, "Discoverable");
    print_property(proxy, "Pairable");
    print_uuids(proxy);
    print_property(proxy, "Modalias");
    print_property(proxy, "Discovering");
}

void show_devices(manager_cxt *manager)
{
    GList *ll;

    if (check_default_ctrl(manager) == FALSE)
        return;

    for (ll = g_list_first(manager->proxy.default_ctrl->devices);
                    ll; ll = g_list_next(ll)) {
        GDBusProxy *proxy = ll->data;
        print_device(proxy, NULL);
    }
}

int get_paired_devices(manager_cxt *manager, BTDevice *dev_array, int arr_len)
{
    GList *ll;
    int count = 0;

    if (check_default_ctrl(manager) == FALSE)
        return 0 ;

    for (ll = g_list_first(manager->proxy.default_ctrl->devices);
                    ll; ll = g_list_next(ll)) {
        GDBusProxy *proxy = ll->data;
        DBusMessageIter iter;
        dbus_bool_t paired;
        const char *address, *name;
        BTAddr bd_addr = {0};
        BTDevice *dev = dev_array + count;

        if (g_dbus_proxy_get_property(proxy, "Paired", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &paired);
        if (!paired)
            continue;

        if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &address);
        if (bd_strtoba(bd_addr, address) != 0)
            continue;

        if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
            dbus_message_iter_get_basic(&iter, &name);
        else
            name = "";

        snprintf(dev->name, sizeof(dev->name), "%s", name);
        memcpy(dev->bd_addr, bd_addr, sizeof(dev->bd_addr));
        BT_LOGD("Device %s %s\n",
                    address, name);
        count++;
        if (count >= arr_len) break;
    }

    return count;
}

void show_device_info(manager_cxt *manager, const char *addr)
{
    GDBusProxy *proxy;
    DBusMessageIter iter;
    const char *address;

    proxy = find_device(manager, addr);
    if (!proxy)
        return;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &address);
    BT_LOGI("Device %s\n", address);

    print_property(proxy, "Name");
    print_property(proxy, "Alias");
    print_property(proxy, "Class");
    print_property(proxy, "Appearance");
    print_property(proxy, "Icon");
    print_property(proxy, "Paired");
    print_property(proxy, "Trusted");
    print_property(proxy, "Blocked");
    print_property(proxy, "Connected");
    print_property(proxy, "LegacyPairing");
    print_uuids(proxy);
    print_property(proxy, "Modalias");
    print_property(proxy, "ManufacturerData");
    print_property(proxy, "ServiceData");
    print_property(proxy, "RSSI");
    print_property(proxy, "TxPower");
}

void show_list_attributes(manager_cxt *manager, const char *addr)
{
	GDBusProxy *proxy;

	proxy = find_device(manager, addr);
	if (!proxy)
		return;

	gatt_list_attributes(g_dbus_proxy_get_path(proxy));
}

gboolean service_is_child(manager_cxt *manager, GDBusProxy *service)
{
    GList *l;
    DBusMessageIter iter;
    const char *device, *path;

    if (g_dbus_proxy_get_property(service, "Device", &iter) == FALSE)
        return FALSE;

    dbus_message_iter_get_basic(&iter, &device);

    if (!manager->proxy.default_ctrl)
        return FALSE;

    for (l = manager->proxy.default_ctrl->devices; l; l = g_list_next(l)) {
        struct GDBusProxy *proxy = l->data;

        path = g_dbus_proxy_get_path(proxy);

        if (!strcmp(path, device))
            return TRUE;
    }

    return FALSE;
}

static void select_adapter(manager_cxt *manager, const char *addr)
{
    struct adapter *adapter;

    if (!manager || !addr || !strlen(addr)) {
        BT_LOGI("Missing controller address argument\n");
        return;
    }

    adapter = find_ctrl_by_address(manager->ctrl_list, addr);
    if (!adapter) {
        BT_LOGI("Controller %s not available\n", addr);
        return;
    }

    if (manager->proxy.default_ctrl && manager->proxy.default_ctrl->proxy == adapter->proxy)
        return;

    manager->proxy.default_ctrl = adapter;
    print_adapter(adapter->proxy, NULL, "default");
}

static void generic_callback(const DBusError *error, void *user_data)
{
    char *str = user_data;

    if (!_global_manager_ctx) return;
    if (dbus_error_is_set(error))
        BT_LOGE("Failed to set %s: %s\n", str, error->name);
    else {
        if (strcmp(str, "power on") == 0) {
            _global_manager_ctx->powered = true;
            BT_LOGI("power on");
        } else if (strcmp(str, "power off") == 0) {
            _global_manager_ctx->powered = false;
            BT_LOGI("power  off");
        } else
            BT_LOGI("Changing %s succeeded\n", str);
    }
}

int set_system_alias(manager_cxt *manager, const char *arg)
{
    int ret = -BT_STATUS_FAIL;
    char *name;

    if (!arg || !strlen(arg)) {
        BT_LOGI("Missing name argument\n");
        return ret;
    }

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    name = g_strdup(arg);

    BT_LOGI("name %s", arg);
    if (g_dbus_proxy_set_property_basic(manager->proxy.default_ctrl->proxy, "Alias",
                    DBUS_TYPE_STRING, &name,
                    generic_callback, name, g_free) == TRUE)
        return BT_STATUS_SUCCESS;

    g_free(name);
    return ret;
}

int set_power(manager_cxt *manager, bool on)
{
    int ret = -BT_STATUS_FAIL;
    char *str;
    dbus_bool_t powered;

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    powered = on;
    str = g_strdup_printf("power %s", powered == TRUE ? "on" : "off");

    if (g_dbus_proxy_set_property_basic(manager->proxy.default_ctrl->proxy, "Powered",
                    DBUS_TYPE_BOOLEAN, &powered, generic_callback, str, g_free) == TRUE)
        return BT_STATUS_SUCCESS;

    g_free(str);
    return ret;
}

int set_pairable(manager_cxt *manager, bool on)
{
    int ret = -BT_STATUS_FAIL;
    char *str;
    dbus_bool_t pairable;

    if (check_default_ctrl(manager) == FALSE)
        return -BT_STATUS_FAIL;

    pairable = on;
    str = g_strdup_printf("pairable %s", pairable == TRUE ? "on" : "off");

    if (g_dbus_proxy_set_property_basic(manager->proxy.default_ctrl->proxy, "Pairable",
					DBUS_TYPE_BOOLEAN, &pairable,
					generic_callback, str, g_free) == TRUE)
        return BT_STATUS_SUCCESS;

    g_free(str);
    return ret;
}

int set_discoverable(manager_cxt *manager, bool on)
{
    int ret = -BT_STATUS_FAIL;
    char *str;
    dbus_bool_t discoverable;

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    discoverable = on;
    str = g_strdup_printf("discoverable %s", discoverable == TRUE ? "on" : "off");

    if (g_dbus_proxy_set_property_basic(manager->proxy.default_ctrl->proxy, "Discoverable",
					DBUS_TYPE_BOOLEAN, &discoverable,
					generic_callback, str, g_free) == TRUE)
        return BT_STATUS_SUCCESS;

    g_free(str);
    return ret;
}

static void start_discovery_reply(DBusMessage *message, void *user_data)
{
    dbus_bool_t enable = GPOINTER_TO_UINT(user_data);
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGI("Failed to %s discovery: %s\n",
                        enable == TRUE ? "start" : "stop", error.name);
        dbus_error_free(&error);
        return;
    }

    BT_LOGI("Discovery %s\n", enable == TRUE ? "started" : "stopped");
}

static void append_variant(DBusMessageIter *iter, int type, void *val)
{
    DBusMessageIter value;
    char sig[2] = { type, '\0' };

    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, sig, &value);

    dbus_message_iter_append_basic(&value, type, val);

    dbus_message_iter_close_container(iter, &value);
}

static void append_array_variant(DBusMessageIter *iter, int type, void *val,
							int n_elements)
{
    DBusMessageIter variant, array;
    char type_sig[2] = { type, '\0' };
    char array_sig[3] = { DBUS_TYPE_ARRAY, type, '\0' };

    dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT,
						array_sig, &variant);

    dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY,
						type_sig, &array);

    if (dbus_type_is_fixed(type) == TRUE) {
        dbus_message_iter_append_fixed_array(&array, type, val,
							n_elements);
    } else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
        const char ***str_array = val;
        int i;

        for (i = 0; i < n_elements; i++)
            dbus_message_iter_append_basic(&array, type,
							&((*str_array)[i]));
    }

    dbus_message_iter_close_container(&variant, &array);

    dbus_message_iter_close_container(iter, &variant);
}

static void dict_append_entry(DBusMessageIter *dict, const char *key,
							int type, void *val)
{
    DBusMessageIter entry;

    if (type == DBUS_TYPE_STRING) {
        const char *str = *((const char **) val);

        if (str == NULL)
            return;
    }

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
							NULL, &entry);

    dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING, &key);

    append_variant(&entry, type, val);

    dbus_message_iter_close_container(dict, &entry);
}

static void dict_append_basic_array(DBusMessageIter *dict, int key_type,
					const void *key, int type, void *val,
					int n_elements)
{
    DBusMessageIter entry;

    dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY,
						NULL, &entry);

    dbus_message_iter_append_basic(&entry, key_type, key);

    append_array_variant(&entry, type, val, n_elements);

    dbus_message_iter_close_container(dict, &entry);
}

static void dict_append_array(DBusMessageIter *dict, const char *key, int type,
						void *val, int n_elements)
{
    dict_append_basic_array(dict, DBUS_TYPE_STRING, &key, type, val,
								n_elements);
}

#define	DISTANCE_VAL_INVALID	0x7FFF

static struct set_discovery_filter_args {
	char *transport;
	dbus_uint16_t rssi;
	dbus_int16_t pathloss;
	char **uuids;
	size_t uuids_len;
	dbus_bool_t duplicate;
	bool set;
} filter = {
    .transport = "bredr",
    .rssi = DISTANCE_VAL_INVALID,
    .pathloss = DISTANCE_VAL_INVALID,
    .set = false,
};

static void set_discovery_filter_setup(DBusMessageIter *iter, void *user_data)
{
	struct set_discovery_filter_args *args = user_data;
	DBusMessageIter dict;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING
				DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);

	dict_append_array(&dict, "UUIDs", DBUS_TYPE_STRING, &args->uuids,
							args->uuids_len);

	if (args->pathloss != DISTANCE_VAL_INVALID)
		dict_append_entry(&dict, "Pathloss", DBUS_TYPE_UINT16,
						&args->pathloss);

	if (args->rssi != DISTANCE_VAL_INVALID)
		dict_append_entry(&dict, "RSSI", DBUS_TYPE_INT16, &args->rssi);

	if (args->transport != NULL)
		dict_append_entry(&dict, "Transport", DBUS_TYPE_STRING,
						&args->transport);

	if (args->duplicate)
		dict_append_entry(&dict, "DuplicateData", DBUS_TYPE_BOOLEAN,
						&args->duplicate);

	dbus_message_iter_close_container(iter, &dict);
}


static void set_discovery_filter_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);
    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGE("SetDiscoveryFilter failed: %s", error.name);
        dbus_error_free(&error);
        return;
    }

    filter.set = true;
    BT_LOGD("SetDiscoveryFilter success\n");
}

static int set_discovery_filter(manager_cxt *manager)
{
    if (filter.set)
        return 0;

    if (g_dbus_proxy_method_call(manager->proxy.default_ctrl->proxy, "SetDiscoveryFilter",
		set_discovery_filter_setup, set_discovery_filter_reply,
		&filter, NULL) == FALSE) {
        BT_LOGE("Failed to set discovery filter");
        return -1;
    }

    filter.set = true;
    return 0;
}

static void* scan_timer_thread(void *arg)
{
    manager_cxt *manager = arg;

    usleep(5 * 1000 * 1000);
    set_scan(manager, false);
    if (manager->disc_listener)
        manager->disc_listener(manager->disc_listener_handle, NULL, NULL, 1, NULL);

    return NULL;
}

int set_scan(manager_cxt *manager, bool start)
{
    int ret = -BT_STATUS_FAIL;
    const char *method;
    dbus_bool_t enable;

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    enable = start;
    if (enable) {
        method = "StartDiscovery";

        set_discovery_filter(manager);
        pthread_mutex_lock(&manager->mutex);
        memset(manager->discovery_devs, 0, sizeof(manager->discovery_devs));
        if (manager->proxy.default_dev) {
            manager->discovery_devs[0].in_use = 1;
            memcpy(&manager->discovery_devs[0].device, &manager->default_device, sizeof(BTDevice));
        }
        pthread_mutex_unlock(&manager->mutex);
    }
    else
        method = "StopDiscovery";

    if (g_dbus_proxy_method_call(manager->proxy.default_ctrl->proxy, method,
				NULL, start_discovery_reply,
				GUINT_TO_POINTER(enable), NULL) == FALSE) {
        BT_LOGI("Failed to %s discovery\n",
					enable == TRUE ? "start" : "stop");
        return ret;
    }
    manager->disc_start = start;
    if (start) {
        pthread_attr_t     attr;
        pthread_t timer_thread;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&timer_thread, &attr, scan_timer_thread, manager);
        pthread_attr_destroy(&attr);
     }

    return BT_STATUS_SUCCESS;
}

static void pair_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGI("Failed to pair: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }

    BT_LOGI("Pairing successful\n");
}

int set_pair_device(manager_cxt *manager, const char *addr)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;

    proxy = find_device(manager, addr);
    if (!proxy)
        return ret;

    if (g_dbus_proxy_method_call(proxy, "Pair", NULL, pair_reply,
							NULL, NULL) == FALSE) {
        BT_LOGI("Failed to pair\n");
        return ret;
    }

    BT_LOGI("Attempting to pair with %s\n", addr);
    return BT_STATUS_SUCCESS;
}

int set_trust_device(manager_cxt *manager, const char *addr, bool on)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;
    char *str;
    dbus_bool_t trusted;

    proxy = find_device(manager, addr);
    if (!proxy)
        return ret;

    trusted = on;
    str = g_strdup_printf("%s %s", addr, trusted ? "trust" : "untrust");

    if (g_dbus_proxy_set_property_basic(proxy, "Trusted",
					DBUS_TYPE_BOOLEAN, &trusted,
					generic_callback, str, g_free) == TRUE)
        return BT_STATUS_SUCCESS;

    g_free(str);
    return ret;
}

static void remove_device_reply(DBusMessage *message, void *user_data)
{
    DBusError error;
    const char *str = user_data;
    char *temp = user_data;
    int i = 0;
    char *addr;
    manager_cxt *manager = _global_manager_ctx;
    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGI("Failed to remove device: %s\n", error.name);
        dbus_error_free(&error);
        while (*temp) {
            if (*temp == ',') break;
            i++;
            temp++;
        }
        addr = malloc(sizeof(char) * i + 1);
        strncpy(addr, str, i);
        addr[i] = 0;
        if (manager->manage_listener) {
            BT_MGT_MSG msg = {0};
            msg.remove.status = -2;
            msg.remove.address = addr;
            manager->manage_listener(manager->manage_listener_handle,
                        BT_EVENT_MGR_REMOVE_DEVICE, &msg);
        }
        free(addr);
    } else {
        BT_LOGI("Device has been removed\n");
    }
}

static void remove_device_setup(DBusMessageIter *iter, void *user_data)
{
    const char *str = user_data;
    const char *path = strrchr(str, ',') + 1;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static int remove_device(manager_cxt *manager, GDBusProxy *proxy, const char *addr)
{
    int ret = -BT_STATUS_FAIL;
    char *str;
    //char *path;

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    str = g_strdup_printf("%s,%s", addr, g_dbus_proxy_get_path(proxy));
    //path = g_strdup(g_dbus_proxy_get_path(proxy));
    if (g_dbus_proxy_method_call(manager->proxy.default_ctrl->proxy, "RemoveDevice",
						remove_device_setup,
						remove_device_reply,
						str, g_free) == FALSE) {
        BT_LOGI("Failed to remove device\n");
        g_free(str);
        return ret;
    }
    return BT_STATUS_SUCCESS;
}

int set_remove_device(manager_cxt *manager, const char *addr)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;

    if (!manager || !addr || !strlen(addr)) {
        BT_LOGI("Missing device address argument\n");
        return ret;
    }

    if (check_default_ctrl(manager) == FALSE)
        return ret;

    if (strcmp(addr, "*") == 0) {
        GList *list;

        if (manager->proxy.default_ctrl->devices == NULL) {
            if (manager->manage_listener) {
                BT_MGT_MSG msg = {0};
                msg.remove.status = 99;
                msg.remove.address = addr;
                manager->manage_listener(manager->manage_listener_handle,
                            BT_EVENT_MGR_REMOVE_DEVICE, &msg);
            }
        } else {
            for (list = manager->proxy.default_ctrl->devices; list;
    						list = g_list_next(list)) {
                GDBusProxy *proxy = list->data;

                if (proxy != manager->proxy.default_dev)//not remove connect devices
                    remove_device(manager, proxy, addr);
            }
        }
        return 0;
    }

    proxy = find_proxy_by_address(manager->proxy.default_ctrl->devices, addr);
    if (!proxy) {
        BT_LOGI("Device %s not available\n", addr);
        return ret;
    }

    return remove_device(manager, proxy, addr);
}

static int set_device_alias(manager_cxt *manager, const char *arg)
{
    int ret = -BT_STATUS_FAIL;
    char *name;

    if (!arg || !strlen(arg)) {
        BT_LOGI("Missing name argument\n");
        return ret;
    }

    if (!manager || !manager->proxy.default_dev) {
        BT_LOGI("No device connected\n");
        return ret;
    }

    name = g_strdup(arg);

    if (g_dbus_proxy_set_property_basic(manager->proxy.default_dev, "Alias",
					DBUS_TYPE_STRING, &name,
					generic_callback, name, g_free) == TRUE)
        return 0;

    g_free(name);
    return ret;
}
static void proxy_leak(gpointer data)
{
    BT_LOGI("Leaking proxy %p\n", data);
}

static void set_default_device(manager_cxt *manager, GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *path;
    const char *address = NULL;
    const char *name = NULL;
    dbus_int16_t rssi = -100;

    manager->proxy.default_dev = proxy;

    if (proxy == NULL) {
        manager->proxy.default_attr = NULL;
        memset(&manager->default_device, 0, sizeof(manager->default_device));
        goto done;
    }

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter)) {
        dbus_message_iter_get_basic(&iter, &name);
    } else
        goto done;
    if (g_dbus_proxy_get_property(proxy, "Address", &iter))
        dbus_message_iter_get_basic(&iter, &address);
    else
        goto done;

    if (g_dbus_proxy_get_property(proxy, "RSSI", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &rssi);

    bd_strtoba(manager->default_device.bd_addr, address);
    snprintf(manager->default_device.name, sizeof(manager->default_device.name),"%s", name);
    manager->default_device.rssi = rssi;
    //set_trust_device(manager, address, true);
    path = g_dbus_proxy_get_path(proxy);

    BT_LOGI("[path:%s]# ", path);

done:

    return;
}

static void connect_profile_setup(DBusMessageIter *iter, void *user_data)
{
    const char *uuid = user_data;

    dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &uuid);
}

static void connect_reply(DBusMessage *message, void *user_data)
{
    DBusError error;
    manager_cxt *manager = _global_manager_ctx;

    if (!manager) return;
    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGI("Failed to connect: %s\n", error.name);
        dbus_error_free(&error);
        if (failed_connect_callback) {
            failed_connect_callback(failed_connect_callback_data, -1);
        }
    } else
        BT_LOGI("Connection successful\n");

    failed_connect_callback = NULL;
    failed_connect_callback_data = NULL;
}

int connect_addr(manager_cxt *manager, const char *addr)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;

    if (!addr || !strlen(addr)) {
        BT_LOGE("Missing device address argument\n");
        return ret;
    }

    if (manager->proxy.default_dev) {
        BT_LOGW("Only support connected one device");
        return -1;
    }
    if (check_default_ctrl(manager) == FALSE)
        return ret;

    proxy = find_proxy_by_address(manager->proxy.default_ctrl->devices, addr);
    if (!proxy) {
        BT_LOGI("Device %s not available\n", addr);
        return ret;
    }

    failed_connect_callback = NULL;
    failed_connect_callback_data = NULL;
    if (g_dbus_proxy_method_call(proxy, "Connect", NULL, connect_reply,
							proxy, NULL) == FALSE) {
        BT_LOGI("Failed to connect\n");
        return ret;
    }

    BT_LOGI("Attempting to connect to %s\n", addr);
    return 0;
}


int connect_profile(manager_cxt *manager, const char *addr, const char *uuid, connect_fail_cb cb, void *userdata)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;

    if (!addr || !strlen(addr)) {
        BT_LOGE("Missing device address argument\n");
        return ret;
    }

#if 0
    if (manager->proxy.default_dev) {
        BT_LOGW("Only support connected one device");
        return -1;
    }
#endif
    if (check_default_ctrl(manager) == FALSE)
        return ret;

    proxy = find_proxy_by_address(manager->proxy.default_ctrl->devices, addr);
    if (!proxy) {
        BT_LOGI("Device %s not available\n", addr);
        return ret;
    }

    failed_connect_callback = NULL;
    failed_connect_callback_data = NULL;
    if (g_dbus_proxy_method_call(proxy, "ConnectProfile", connect_profile_setup, connect_reply,
							(void *)uuid, NULL) == FALSE) {
        BT_LOGI("Failed to connect\n");
        return ret;
    }
    failed_connect_callback = cb;
    failed_connect_callback_data = userdata;

    BT_LOGI("Attempting to connect to %s %s", addr, uuid);
    return 0;
}

int connect_profile_addr_array(manager_cxt *manager, BTAddr addr, const char *uuid, connect_fail_cb cb, void *userdata)
{
    int ret = -BT_STATUS_FAIL;
    char address[18] = {0};

    if (!manager) return ret;

    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    return connect_profile(manager, address, uuid, cb, userdata);
}

int connect_addr_array(manager_cxt *manager, BTAddr addr)
{
    int ret = -BT_STATUS_FAIL;
    char address[18] = {0};

    if (!manager) return ret;

    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    return connect_addr(manager, address);
}

static void disconn_reply(DBusMessage *message, void *user_data)
{
    //GDBusProxy *proxy = user_data;
    manager_cxt *manager = _global_manager_ctx;
    DBusError error;

    if (!manager) return;
    dbus_error_init(&error);

    manager->disconnect_completed = 1;
    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGI("Failed to disconnect: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }

    BT_LOGI("Successful disconnected\n");
}

int disconn_addr(manager_cxt *manager, const char *address)
{
    int ret = -BT_STATUS_FAIL;
    GDBusProxy *proxy;

    if (!manager) return ret;
    proxy = find_device(manager, address);
    if (!proxy)
        return ret;
    manager->disconnect_completed = 0;
    if (g_dbus_proxy_method_call(proxy, "Disconnect", NULL, disconn_reply,
                        proxy, NULL) == FALSE) {
        manager->disconnect_completed = 1;
        BT_LOGE("Failed to disconnect");
        return ret;
    }
    if (strlen(address) == 0) {
        DBusMessageIter iter;

        if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE)
            dbus_message_iter_get_basic(&iter, &address);
    }
    BT_LOGI("Attempting to disconnect from %s\n", address);
    return 0;
}


int disconn_addr_array(manager_cxt *manager, BTAddr addr)
{
    int ret = -BT_STATUS_FAIL;
    char address[18] = {0};

    if (!manager) return ret;

    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return disconn_addr(manager, address);
}


static void connect_handler(DBusConnection *connection, void *user_data)
{
    manager_cxt *manager = user_data;
    BT_MGT_MSG msg = {0};

    BT_LOGD("entry");

    if (manager->manage_listener) {
        msg.connect.enable = 1;
        manager->manage_listener(manager->manage_listener_handle,
        BT_EVENT_MGR_CONNECT, &msg);
    }

}

static void disconnect_handler(DBusConnection *connection, void *user_data)
{
    manager_cxt *manager = user_data;
    BT_MGT_MSG msg = {0};

    g_list_free_full(manager->ctrl_list, proxy_leak);
    manager->ctrl_list = NULL;

    if (manager->manage_listener && !main_thread_close_normal) {
        BT_LOGE("abnormal  disconnect bluetoothd!!!!");
        msg.disconnect.reason = 0;
        manager->manage_listener(manager->manage_listener_handle,
            BT_EVENT_MGR_DISCONNECT, &msg);
    }

}

static void message_handler(DBusConnection *connection,
					DBusMessage *message, void *user_data)
{
    //manager_cxt *manager = user_data;
    BT_LOGI("[SIGNAL] %s.%s\n", dbus_message_get_interface(message),
                    dbus_message_get_member(message));
}

void disconnect_device(manager_cxt *manager, GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *address, *name;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &address);

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "<unknown>";
    manager->disconnect_completed = 0;
    if (g_dbus_proxy_method_call(proxy,
                "Disconnect", NULL, disconn_reply,
                proxy, NULL) == FALSE) {
        BT_LOGE("Failed to disconnect\n");
        manager->disconnect_completed = 1;
        return;
    }
    BT_LOGI("Attempting to disconnect from %s %s\n",
            address, name);
}

static void device_added(manager_cxt *manager, GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *address, *name;
    dbus_int16_t rssi = -100;
    BTAddr bd_addr = {0};
    struct adapter *adapter = NULL;
    int index;

    if (!manager) {
        return;
    }
    adapter = find_parent(manager->ctrl_list, proxy);

    if (!adapter) {
        /* TODO: Error */
        return;
    }

    adapter->devices = g_list_append(adapter->devices, proxy);

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE) {
        dbus_message_iter_get_basic(&iter, &address);
        bd_strtoba(bd_addr, address);
    } else
        address = "";

    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "";
    if (g_dbus_proxy_get_property(proxy, "RSSI", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &rssi);

    pthread_mutex_lock(&manager->mutex);
    /* If this is a new device */
    for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
        if (manager->discovery_devs[index].in_use == 0) {
            BT_LOGI("[NEW] Discovered device:%d", index);
            manager->discovery_devs[index].in_use = 1;
            memcpy(manager->discovery_devs[index].device.bd_addr,
                        bd_addr, sizeof(BTAddr));
            snprintf(manager->discovery_devs[index].device.name,
                            sizeof(manager->discovery_devs[index].device.name), "%s", name);
            manager->discovery_devs[index].device.rssi = rssi;
            break;
        }
    }

    /* If this is a new device but now room to save, save the rssi bigger one*/
    if (index >= BT_DISC_NB_DEVICES && rssi > -100) {
        int min_rssi;
        int min_index;
        min_index = 0;
        min_rssi = manager->discovery_devs[0].device.rssi;
        for (index = 1; index < BT_DISC_NB_DEVICES; index++) {
            if (min_rssi > manager->discovery_devs[index].device.rssi) {
                min_rssi = manager->discovery_devs[index].device.rssi;
                min_index = index;
            }
        }
        if (rssi != 0 && rssi > min_rssi) {
                BT_LOGI("[REPLACE] Discovered device:%d, old min_rssi : %d", min_index, min_rssi);
                //manager->discovery_devs[min_index].in_use = 1;
                memcpy(manager->discovery_devs[min_index].device.bd_addr,
                        bd_addr, sizeof(BTAddr));
                snprintf(manager->discovery_devs[min_index].device.name,
                            sizeof(manager->discovery_devs[min_index].device.name), "%s", name);
                manager->discovery_devs[min_index].device.rssi = rssi;
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    /* If this is a new device but now room to save*/
    if (manager->disc_start) {
        if (manager->disc_listener) {
            manager->disc_listener(manager->disc_listener_handle, name, bd_addr, 0, NULL);
        }
    }
    BT_LOGI("[%s]Device %s %s RSSI:%d\n", COLORED_NEW,
                    address, name, rssi);
    //print_device(proxy, COLORED_NEW);

    if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
        dbus_bool_t connected;

        dbus_message_iter_get_basic(&iter, &connected);

        if (connected) {
            if (manager->init && !manager->proxy.default_dev) {
                set_default_device(manager, proxy);
            } else {
                BT_LOGW("already have connect device, disconnect this devices");
                disconnect_device(manager, proxy);
            }
        }
    }
}

static void device_removed(manager_cxt *manager, GDBusProxy *proxy)
{
    struct adapter *adapter = NULL;
    DBusMessageIter iter;
    const char *address, *name;
    BTAddr bd_addr = {0};
    int index;

    if (!manager) {
        return;
    }
    adapter = find_parent(manager->ctrl_list, proxy);
    if (!adapter) {
        /* TODO: Error */
        return;
    }

    adapter->devices = g_list_remove(adapter->devices, proxy);

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE) {
        dbus_message_iter_get_basic(&iter, &address);
        bd_strtoba(bd_addr, address);
    } else
        address = "";
    if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "";
    pthread_mutex_lock(&manager->mutex);
    for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
        if ((manager->discovery_devs[index].in_use) &&
                    (!bdcmp(manager->discovery_devs[index].device.bd_addr, bd_addr))) {
            /* removed device */
            BT_LOGI("removed device:%d", index);
            manager->discovery_devs[index].in_use = 0;
            break;
        }
    }
    pthread_mutex_unlock(&manager->mutex);
    BT_LOGI("[%s]Device %s %s", COLORED_DEL,
                    address, name);
    //print_device(proxy, COLORED_DEL);

    if (manager->proxy.default_dev == proxy)
        set_default_device(manager, NULL);

    if (manager->manage_listener) {
        BT_MGT_MSG msg = {0};
        if (adapter->devices == NULL) {
            msg.remove.status = 99;//full removed
        } else {
            msg.remove.status = 0;
        }
        msg.remove.address = address;
        manager->manage_listener(manager->manage_listener_handle,
            BT_EVENT_MGR_REMOVE_DEVICE, &msg);
    }
}


static struct adapter *adapter_new(manager_cxt *manager, GDBusProxy *proxy)
{
    struct adapter *adapter = g_malloc0(sizeof(struct adapter));

    manager->ctrl_list = g_list_append(manager->ctrl_list, adapter);

    if (!manager->proxy.default_ctrl)
        manager->proxy.default_ctrl = adapter;

    return adapter;
}

static void adapter_added(manager_cxt *manager, GDBusProxy *proxy)
{
    struct adapter *adapter;
    DBusMessageIter iter;
    const char *address, *name;

    adapter = find_ctrl(manager->ctrl_list, g_dbus_proxy_get_path(proxy));
    if (!adapter) {
        adapter = adapter_new(manager, proxy);
        if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
            dbus_message_iter_get_basic(&iter, &name);
        else
            name = "<unknown>";
        //snprintf(manager->bt_name, sizeof(manager->bt_name), "%s", name);
    }

    adapter->proxy = proxy;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == TRUE)
        dbus_message_iter_get_basic(&iter, &address);
    else
        return;
    bd_strtoba(MODULE_BD_ADDR, address);

    BT_LOGI("[%s]Controller %s %s %s\n", COLORED_NEW,
				address, name, "default");
}

static void adapter_removed(manager_cxt *manager, GDBusProxy *proxy)
{
    GList *ll;

    for (ll = g_list_first(manager->ctrl_list); ll; ll = g_list_next(ll)) {
        struct adapter *adapter = ll->data;

        if (adapter->proxy == proxy) {
            print_adapter(proxy, COLORED_DEL, "");

            if (manager->proxy.default_ctrl && manager->proxy.default_ctrl->proxy == proxy) {
                manager->proxy.default_ctrl = NULL;
                set_default_device(manager, NULL);
            }

            manager->ctrl_list = g_list_remove_link(manager->ctrl_list, ll);
            g_list_free(adapter->devices);
            g_free(adapter);
            g_list_free(ll);
            return;
        }
    }
}


static void ad_manager_added(manager_cxt *manager, GDBusProxy *proxy)
{
    struct adapter *adapter;
    adapter = find_ctrl(manager->ctrl_list, g_dbus_proxy_get_path(proxy));
    if (!adapter)
        adapter = adapter_new(manager, proxy);

    adapter->ad_proxy = proxy;
}

static void ad_manager_removed(manager_cxt *manager, GDBusProxy *proxy)
{
    struct adapter *adapter;
    adapter = find_ctrl(manager->ctrl_list, g_dbus_proxy_get_path(proxy));
    if (!adapter)
        return;

    adapter->ad_proxy = NULL;
}

void set_default_attribute(manager_cxt *manager, GDBusProxy *proxy)
{
    //const char *path;

    manager->proxy.default_attr = proxy;

    //path = g_dbus_proxy_get_path(proxy);

    //set_default_device(manager, manager->proxy.default_dev);
}

static void proxy_added(GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    manager_cxt *manager = user_data;

    interface = g_dbus_proxy_get_interface(proxy);

    BT_LOGI("interface %s", interface);
    if (!strcmp(interface, DEVICE_INTERFACE)) {
        device_added(manager, proxy);
    } else if (!strcmp(interface, "org.bluez.Adapter1")) {
        adapter_added(manager, proxy);
    } else if (!strcmp(interface, "org.bluez.AgentManager1")) {
        if (!manager->proxy.agent_manager) {
            manager->proxy.agent_manager = proxy;
            agent_register(dbus_conn, proxy,
                                        AUTO_REGISTER_AGENT);
        }
    } else if (!strcmp(interface, "org.bluez.GattManager1")) {
        if (!manager->proxy.GattManager) {
            manager->proxy.GattManager = proxy;
            gatt_add_manager(proxy);
            gatt_register_manager(dbus_conn, proxy);
        }
    } else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        ad_manager_added(manager, proxy);
    }
    ble_proxy_added(manager->ble_ctx, proxy, manager);
    a2dpk_proxy_added(manager->a2dp_sink, proxy, manager);
    a2dp_proxy_added(manager->a2dp_ctx, proxy, manager);
}

static void proxy_removed(GDBusProxy *proxy, void *user_data)
{
    const char *interface;
    manager_cxt *manager = user_data;

    interface = g_dbus_proxy_get_interface(proxy);
    BT_LOGI("interface %s\n", interface);

    ble_proxy_removed(manager->ble_ctx, proxy, manager);
    a2dpk_proxy_removed(manager->a2dp_sink, proxy, manager);
    a2dp_proxy_removed(manager->a2dp_ctx, proxy, manager);
    if (!strcmp(interface, DEVICE_INTERFACE)) {
        device_removed(manager, proxy);
    } else if (!strcmp(interface, "org.bluez.Adapter1")) {
        adapter_removed(manager, proxy);
    } else if (!strcmp(interface, "org.bluez.AgentManager1")) {
        if (manager->proxy.agent_manager == proxy) {
            manager->proxy.agent_manager = NULL;
            agent_unregister(dbus_conn, proxy);
        }
    } else if (!strcmp(interface, "org.bluez.GattManager1")) {
        if (manager->proxy.GattManager == proxy) {
            gatt_remove_manager(proxy);
            manager->proxy.GattManager = NULL;
        }
    } else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        //BT_LOGD("proxy %p", proxy);
        ad_manager_removed(manager, proxy);
        ad_unregister(dbus_conn, NULL, NULL);
    }
}

static void update_disc_name(manager_cxt *manager, const char *name, const char *address)
{
    int index;
    BTAddr bd_addr = {0};

    if (bd_strtoba(bd_addr, address))
        return;
    pthread_mutex_lock(&manager->mutex);
    for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
        if ((manager->discovery_devs[index].in_use) &&
                    (!bdcmp(manager->discovery_devs[index].device.bd_addr, bd_addr))) {
            /* Update device */
            BT_LOGI("[CHG] device:%d", index);
            snprintf(manager->discovery_devs[index].device.name,
                                sizeof(manager->discovery_devs[index].device.name), "%s", name);
            break;
        }
    }
    /* If this is a new device */
    if (index >= BT_DISC_NB_DEVICES) {
        for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
            if (manager->discovery_devs[index].in_use == 0) {
                BT_LOGI("[NEW] Discovered device:%d", index);
                manager->discovery_devs[index].in_use = 1;
                memcpy(manager->discovery_devs[index].device.bd_addr,
                        bd_addr, sizeof(BTAddr));
                snprintf(manager->discovery_devs[index].device.name,
                                sizeof(manager->discovery_devs[index].device.name), "%s", name);
                manager->discovery_devs[index].device.rssi = -100;
                break;
            }
        }
    }
    pthread_mutex_unlock(&manager->mutex);

    if (manager->disc_start) {
        if (manager->disc_listener) {
            manager->disc_listener(manager->disc_listener_handle, name, bd_addr, 0, NULL);
        }
    }
}

static void update_disc_info(manager_cxt *manager, const char *name, int rssi, const char *address)
{
    int index;
    BTAddr bd_addr = {0};

    if (bd_strtoba(bd_addr, address))
        return;
    pthread_mutex_lock(&manager->mutex);
    for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
        if ((manager->discovery_devs[index].in_use) &&
                    (!bdcmp(manager->discovery_devs[index].device.bd_addr, bd_addr))) {
            /* Update device */
            BT_LOGI("[CHG] device:%d", index);
            snprintf(manager->discovery_devs[index].device.name,
                                sizeof(manager->discovery_devs[index].device.name), "%s", name);
            manager->discovery_devs[index].device.rssi = rssi;
            break;
        }
    }
    /* If this is a new device */
    if (index >= BT_DISC_NB_DEVICES) {
        for (index = 0; index < BT_DISC_NB_DEVICES; index++) {
            if (manager->discovery_devs[index].in_use == 0) {
                BT_LOGI("[NEW] Discovered device:%d", index);
                manager->discovery_devs[index].in_use = 1;
                memcpy(manager->discovery_devs[index].device.bd_addr,
                        bd_addr, sizeof(BTAddr));
                snprintf(manager->discovery_devs[index].device.name,
                                sizeof(manager->discovery_devs[index].device.name), "%s", name);
                manager->discovery_devs[index].device.rssi = rssi;
                break;
            }
        }
    }
    /* If this is a new device but no room to save, save the rssi bigger one*/
    if (index >= BT_DISC_NB_DEVICES && rssi > -80) {
        int min_rssi;
        int min_index;
        min_index = 0;
        min_rssi = manager->discovery_devs[0].device.rssi;
        for (index = 1; index < BT_DISC_NB_DEVICES; index++) {
            if (min_rssi > manager->discovery_devs[index].device.rssi) {
                min_rssi = manager->discovery_devs[index].device.rssi;
                min_index = index;
            }
        }
        if (rssi != 0 && rssi > min_rssi) {
            BT_LOGI("[REPLACE] Discovered device:%d, old min_rssi %d", min_index, min_rssi);
            //manager->discovery_devs[min_index].in_use = 1;
            memcpy(manager->discovery_devs[min_index].device.bd_addr,
                                bd_addr, sizeof(BTAddr));
            snprintf(manager->discovery_devs[min_index].device.name,
                                sizeof(manager->discovery_devs[min_index].device.name), "%s", name);
            manager->discovery_devs[min_index].device.rssi = rssi;
        }
    }
    pthread_mutex_unlock(&manager->mutex);

    if (manager->disc_start) {
        if (manager->disc_listener) {
            manager->disc_listener(manager->disc_listener_handle, name, bd_addr, 0, NULL);
        }
    }
}

static void property_changed(GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
    const char *interface;
    struct adapter *ctrl;
    manager_cxt *manager = user_data;
    int notify_flag = 1;

    if (!iter) return;
    interface = g_dbus_proxy_get_interface(proxy);
    BT_LOGI("interface %s, name %s", interface, name);

    if (!strcmp(interface, DEVICE_INTERFACE)) {
        if (manager->proxy.default_ctrl && device_is_child(proxy,
                    manager->proxy.default_ctrl->proxy) == TRUE) {
            DBusMessageIter get_iter;
            char *str;
            const char *address = NULL;
            const char *alias = NULL;
            dbus_int16_t rssi = -100;
            if (g_dbus_proxy_get_property(proxy, "Address",
                                &get_iter) == TRUE) {
                dbus_message_iter_get_basic(&get_iter, &address);
                str = g_strdup_printf("[" COLORED_CHG
                                        "] Device %s ", address);
            } else
                str = g_strdup("");
            print_iter(str, name, iter);
            g_free(str);

            if (g_dbus_proxy_get_property(proxy, "Alias",
                                &get_iter) == TRUE) {
                dbus_message_iter_get_basic(&get_iter, &alias);
            }
            if (!strcmp(name, "Alias")) {
                 if (g_dbus_proxy_get_property(proxy, "RSSI",
                                &get_iter) == TRUE) {
                    dbus_message_iter_get_basic(&get_iter, &rssi);
                }
                update_disc_name(manager, alias, address);
                if (manager->proxy.default_dev == proxy) {
                    snprintf(manager->default_device.name,
                                sizeof(manager->default_device.name), "%s", alias);
                }
            } else if(!strcmp(name, "RSSI") && iter) {
                dbus_message_iter_get_basic(iter, &rssi);
                update_disc_info(manager, alias, rssi, address);
                if (manager->proxy.default_dev == proxy) {
                    manager->default_device.rssi = rssi;
                }
            }

            if (strcmp(name, "Connected") == 0) {
                dbus_bool_t connected;
                dbus_message_iter_get_basic(iter, &connected);

                if (connected && manager->proxy.default_dev == NULL) {
                    BT_LOGI("set default devices");
                    set_default_device(manager, proxy);
                } else if (!connected && manager->proxy.default_dev == proxy) {
                    BT_LOGI("clear default devices");
                    set_default_device(manager, NULL);
                } else if (connected && manager->proxy.default_dev) {//only support connected 1 device
                    BT_LOGW("already have connect device, disconnect this devices");
                    disconnect_device(manager, proxy);
                    notify_flag = 0;
                }
                else if (!connected) {
                    BT_LOGW("not default device ignored this disconnected");
                    notify_flag = 0;
                }
            }
        }
    }
    else if (!strcmp(interface, "org.bluez.Adapter1")) {
        DBusMessageIter addr_iter;
        char *str;

        if (strcmp(name, "Powered") == 0) {
            dbus_bool_t powered;
            dbus_message_iter_get_basic(iter, &powered);
            manager->powered = powered;
        }
        if (g_dbus_proxy_get_property(proxy, "Address",
                            &addr_iter) == TRUE) {
            const char *address;

            dbus_message_iter_get_basic(&addr_iter, &address);
            str = g_strdup_printf("[" COLORED_CHG
                            "] Controller %s ", address);
        } else
            str = g_strdup("");

        print_iter(str, name, iter);
        g_free(str);
    }
    else if (!strcmp(interface, "org.bluez.LEAdvertisingManager1")) {
        DBusMessageIter addr_iter;
        char *str;

        ctrl = find_ctrl(manager->ctrl_list, g_dbus_proxy_get_path(proxy));
        if (!ctrl)
            return;

        if (g_dbus_proxy_get_property(ctrl->proxy, "Address",
                            &addr_iter) == TRUE) {
            const char *address;
            dbus_message_iter_get_basic(&addr_iter, &address);
            str = g_strdup_printf("[" COLORED_CHG
                            "] Controller %s ", address);
        } else
            str = g_strdup("");

        print_iter(str, name, iter);
        g_free(str);
    }
    else if (proxy == manager->proxy.default_attr) {
        char *str;
        str = g_strdup_printf("[" COLORED_CHG "] Attribute %s ",
                            g_dbus_proxy_get_path(proxy));

        print_iter(str, name, iter);
        g_free(str);
    }
    if (notify_flag) {
        ble_property_changed(manager->ble_ctx, proxy, name, iter, manager);
        a2dpk_property_changed(manager->a2dp_sink, proxy, name, iter, manager);
        a2dp_property_changed(manager->a2dp_ctx, proxy, name, iter, manager);
    }
}

static void client_ready(GDBusClient *client, void *user_data)
{
    manager_cxt *manager = user_data;

    ble_change_to_bredr(manager->ble_ctx);
    set_power(manager, true);
    set_pairable(manager, false);
    set_discoverable(manager, false);
    manager->init = true;
    BT_LOGD("init ok");
}

int create_ble_net_services(manager_cxt *manager)
{
    const char *chrc_flag = "read,write,notify";

    BT_CHECK_HANDLE(manager);

    gatt_register_service(dbus_conn, NULL, BLE_SERVICE_UUID);
    gatt_register_chrc_full(dbus_conn, NULL, BLE_CHRC_UUID1, chrc_flag, NULL, 0);
    gatt_register_chrc_full(dbus_conn, NULL, BLE_CHRC_UUID2, chrc_flag, NULL, 0);
    return 0;
}

int destroy_ble_net_services(manager_cxt *manager)
{

    BT_CHECK_HANDLE(manager);

    gatt_unregister_chrc(dbus_conn, NULL, BLE_CHRC_UUID1);
    gatt_unregister_chrc(dbus_conn, NULL, BLE_CHRC_UUID2);
    gatt_unregister_service(dbus_conn, NULL, BLE_SERVICE_UUID);

    return 0;
}




static void mgmt_debug(const char *str, void *user_data)
{
	const char *prefix = user_data;

	BT_LOGD("%s%s", prefix, str);
}

#if 0
static void register_mgmt_callbacks(struct mgmt *mgmt, uint16_t index)
{
	mgmt_register(mgmt, MGMT_EV_CONTROLLER_ERROR, index, controller_error,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_INDEX_ADDED, index, index_added,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_INDEX_REMOVED, index, index_removed,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_NEW_SETTINGS, index, new_settings,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_DISCOVERING, index, discovering,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_NEW_LINK_KEY, index, new_link_key,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_DEVICE_CONNECTED, index, connected,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_DEVICE_DISCONNECTED, index, disconnected,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_CONNECT_FAILED, index, conn_failed,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_AUTH_FAILED, index, auth_failed,
								NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_CLASS_OF_DEV_CHANGED, index,
					class_of_dev_changed, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_LOCAL_NAME_CHANGED, index,
					local_name_changed, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_DEVICE_FOUND, index, device_found,
								mgmt, NULL);
	mgmt_register(mgmt, MGMT_EV_PIN_CODE_REQUEST, index, request_pin,
								mgmt, NULL);
	mgmt_register(mgmt, MGMT_EV_USER_CONFIRM_REQUEST, index, user_confirm,
								mgmt, NULL);
	mgmt_register(mgmt, MGMT_EV_USER_PASSKEY_REQUEST, index,
						request_passkey, mgmt, NULL);
	mgmt_register(mgmt, MGMT_EV_PASSKEY_NOTIFY, index,
						passkey_notify, mgmt, NULL);
	mgmt_register(mgmt, MGMT_EV_UNCONF_INDEX_ADDED, index,
					unconf_index_added, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_UNCONF_INDEX_REMOVED, index,
					unconf_index_removed, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_NEW_CONFIG_OPTIONS, index,
					new_config_options, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_EXT_INDEX_ADDED, index,
					ext_index_added, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_EXT_INDEX_REMOVED, index,
					ext_index_removed, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_LOCAL_OOB_DATA_UPDATED, index,
					local_oob_data_updated, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_ADVERTISING_ADDED, index,
						advertising_added, NULL, NULL);
	mgmt_register(mgmt, MGMT_EV_ADVERTISING_REMOVED, index,
					advertising_removed, NULL, NULL);
}
#endif

static const char *settings_str[] = {
				"powered",
				"connectable",
				"fast-connectable",
				"discoverable",
				"bondable",
				"link-security",
				"ssp",
				"br/edr",
				"hs",
				"le",
				"advertising",
				"secure-conn",
				"debug-keys",
				"privacy",
				"configuration",
				"static-addr",
};

static const char *settings2str(uint32_t settings)
{
	static char str[256];
	unsigned i;
	int off;

	off = 0;
	str[0] = '\0';

	for (i = 0; i < NELEM(settings_str); i++) {
		if ((settings & (1 << i)) != 0)
			off += snprintf(str + off, sizeof(str) - off, "%s ",
							settings_str[i]);
	}

	return str;
}

static void cmd_rsp(uint8_t status, uint16_t len, const void *param,
							void *user_data)
{
    const uint32_t *rp = param;

    if (status != 0) {
        BT_LOGE("failed with status 0x%02x (%s)",
			status, mgmt_errstr(status));
        goto done;
    }

    BT_LOGD("settings: %s", settings2str(get_le32(rp)));

done:
	return;
}

unsigned int send_cmd(struct mgmt *mgmt, uint16_t op, uint16_t id,
				uint16_t len, const void *param)
{
    unsigned int send_id;
    send_id = mgmt_send(mgmt, op, id, len, param, cmd_rsp, NULL, NULL);
    if (send_id == 0) {
        BT_LOGE("faile to send op %d", op);
    }

    return send_id;
}

int mgmt_init(manager_cxt *manager)
{
    if (!manager->mgmt) {
        manager->mgmt = mgmt_new_default();
        if (getenv("MGMT_DEBUG"))
            mgmt_set_debug(manager->mgmt, mgmt_debug, "mgmt: ", NULL);

        //register_mgmt_callbacks(mgmt, mgmt_index);
    }
    return 0;
}

int mgmt_deinit(manager_cxt *manager)
{
    if (manager->mgmt) {
        mgmt_unref(manager->mgmt);
        manager->mgmt = NULL;
    }
    return 0;
}

static void *main_thread(void *arg)
{
    GDBusClient *client;
    manager_cxt *manager = arg;

    show_version();
    main_loop = g_main_loop_new(NULL, FALSE);
    if (!dbus_conn) {
        dbus_conn = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
        g_dbus_attach_object_manager(dbus_conn);
    }

    create_ble_net_services(manager);
    client = g_dbus_client_new(dbus_conn, "org.bluez", "/org/bluez");

    g_dbus_client_set_connect_watch(client, connect_handler, manager);
    g_dbus_client_set_disconnect_watch(client, disconnect_handler, manager);
    g_dbus_client_set_signal_watch(client, message_handler, manager);

    g_dbus_client_set_proxy_handlers(client, proxy_added, proxy_removed,
							property_changed, manager);

    g_dbus_client_set_ready_watch(client, client_ready, manager);

    g_main_loop_run(main_loop);
    if (manager->proxy.GattManager) {
        gatt_unregister_app(dbus_conn, manager->proxy.GattManager);
    }
    g_dbus_client_unref(client);
    destroy_ble_net_services(manager);
    //g_dbus_detach_object_manager(dbus_conn);
    //dbus_connection_unref(dbus_conn);
    //dbus_conn = NULL;

    g_main_loop_unref(main_loop);
    main_loop = NULL;
    g_list_free_full(manager->ctrl_list, proxy_leak);
    memset(manager->discovery_devs, 0, sizeof(manager->discovery_devs));
    manager->ctrl_list = NULL;
    manager->init = false;
    main_thread_running = false;
    BT_LOGI("exit");
    return NULL;
}

int manager_init(manager_cxt *manager)
{
    int r;
    int times = 30;

    BT_CHECK_HANDLE(manager);
    if (main_thread_running) {
        //uint8_t val_true = 1;
        do {
            ble_change_to_bredr(manager->ble_ctx);
            set_power(manager, true);
            //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
            //        sizeof(val_true), &val_true);
            usleep(100 * 1000);//wait power on
        } while (!manager->powered && times--);
        return 0;
    }
    r = pthread_create(&main_tid, NULL, main_thread, (void *)manager);
    main_thread_running = (r == 0);
    if (r) {
        BT_LOGE("create thread failed");
        return r;
    }

    while (!manager->init && times--) {
        usleep(100 * 1000);
    }
    if (!manager->init) {
        g_main_loop_quit(main_loop);
        pthread_join(main_tid, NULL);
        main_thread_running = false;
        return -1;
    }
    main_thread_close_normal = false;
    usleep(200 * 1000);//wait for ready
    return 0;
}


int manager_deinit(manager_cxt *manager)
{
    //uint8_t val_false = 0;
    int times = 3;
    BT_CHECK_HANDLE(manager);

    if (manager->ble_ctx) {
        ble_server_disable(manager->ble_ctx);
    }
    if (manager->a2dp_sink) {
        a2dpk_disable(manager->a2dp_sink);
    }
    if (manager->a2dp_ctx) {
        a2dp_disable(manager->a2dp_ctx);
    }
    do {
        //send_cmd(manager->mgmt, MGMT_OP_SET_POWERED, 0,
        //                sizeof(val_false), &val_false);
        set_power(manager, false);
        usleep(100 * 1000);//wait power off
    } while (manager->powered && times--);
    /*not quit main loop*/
    return 0;
}

manager_cxt *manager_create(void)
{
    manager_cxt *manager = calloc(1, sizeof(*manager));

    mgmt_init(manager);
    manager->ble_ctx = ble_create(manager);
    manager->a2dp_sink = a2dpk_create(manager);
    manager->a2dp_ctx = a2dp_create(manager);
    pthread_mutex_init(&manager->mutex, NULL);
    _global_manager_ctx = manager;
    return manager;
}

int manager_destroy(manager_cxt *manager)
{
    if (manager) {
        manager_deinit(manager);
        if (main_thread_running) {
            main_thread_close_normal = true;
            g_main_loop_quit(main_loop);
            pthread_join(main_tid, NULL);
            main_thread_running = false;
        }
        if (manager->ble_ctx) {
            ble_destroy(manager->ble_ctx);
            manager->ble_ctx = NULL;
        }
        if (manager->a2dp_sink) {
            a2dpk_destroy(manager->a2dp_sink);
            manager->a2dp_sink = NULL;
        }
        if (manager->a2dp_ctx) {
            a2dp_destroy(manager->a2dp_ctx);
            manager->a2dp_ctx = NULL;
        }
        mgmt_deinit(manager);
        pthread_mutex_destroy(&manager->mutex);
        free(manager);
    }
    _global_manager_ctx = NULL;

    return 0;
}

int bt_set_visibility(manager_cxt *manager, bool discoverable, bool connectable)
{
#if 0
    uint8_t connect_mode;
    struct mgmt_cp_set_discoverable cp;

    cp.timeout = 0;
    cp.val = discoverable;
    connect_mode = connectable;

    send_cmd(manager->mgmt, MGMT_OP_SET_CONNECTABLE, 0,
                    sizeof(connect_mode), &connect_mode);
    send_cmd(manager->mgmt, MGMT_OP_SET_DISCOVERABLE, 0,
                    sizeof(cp), &cp);
    #if 0
        if (discoverable) {
            system("hcitool cmd 0x03 0x001c 20 03 14 00");//page scan interval:500ms, window 12.5ms
            system("hcitool cmd 0x03 0x001e 20 03 14 00");//Inquiry scan interval:500ms, window 12.5ms
        }
    #endif
#else
    set_discoverable(manager, discoverable);
    if (discoverable) {
        system("hciconfig hci0 pageparms 18:1024");//page scan window/interval: 11.25ms/640ms interlaced
        system("hciconfig hci0 inqparms 18:2048");//inquiry scan window/interval: 11.25ms/1.28s interlaced
        system("hcitool cmd 0x03 0x43 0x01");
        system("hcitool cmd 0x03 0x47 0x01");
        //system("hcitool cmd 0x03 0x001c 20 03 14 00");//page scan interval:500ms, window 12.5ms
        //system("hcitool cmd 0x03 0x001e 20 03 14 00");//Inquiry scan interval:500ms, window 12.5ms
    }
#endif
    return 0;
}

int bt_set_name(manager_cxt *manager, const char *name)
{
    int ret = -BT_STATUS_NOT_READY;
    //char cmd[250];

    //snprintf(cmd, sizeof(cmd), "btmgmt -i hci0 name \"%s\"", name);
    //ret = system(cmd);
    //memset(cmd, 0, sizeof(cmd));
    //snprintf(cmd, sizeof(cmd), "hciconfig hci0 name \"%s\"", name);
    //ret = system(cmd);
    ret = set_system_alias(manager, name);
    snprintf(manager->bt_name, sizeof(manager->bt_name), "%s", name);
    return ret;
}

int app_get_module_addr(manager_cxt *manager, BTAddr addr)
{
    if (manager ->init) {
        memcpy(addr, MODULE_BD_ADDR, sizeof(BTAddr));
        return 0;
    } else
        return -1;
}

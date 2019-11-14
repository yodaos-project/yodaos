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
#include <string.h>
#include <stdbool.h>

#include <glib.h>
#include <hardware/bt_common.h>
#include <bluetooth/bluetooth.h>

#include "shared/util.h"
#include "gdbus/gdbus.h"
//#include "client/display.h"
#include "lib/uuid.h"
//#include "client/agent.h"
//#include "client/gatt.h"
//#include "client/advertising.h"
//#include <sys/time.h>
#include "app_common.h"


int bdcmp(BTAddr bd_addr1, BTAddr bd_addr2)
{
    int i;
    for (i = 0; i < sizeof(BTAddr); i++) {
        if (bd_addr1[i] != bd_addr2[i])
            return bd_addr1[i] - bd_addr2[i];
    }
    return 0;
}


int bd_strtoba(BTAddr addr, const char *address)
{
    int i;
    int len = strlen(address);
    char *dest = (char *)addr;
    char *begin = (char *)address;
    char *end_ptr;

    if (!address || !addr || len != 17) {
        BT_LOGE("faile to addr:%s, len:%d", address, len);
        return -1;
    }
    for (i = 0; i < 6; i++) {
        dest[i] = (char)strtoul(begin, &end_ptr, 16);
        if (!end_ptr) break;
        if (*end_ptr == '\0') break;
        if (*end_ptr != ':') {
            BT_LOGE("faile to addr:%s, len:%d, end_ptr: %c, %s", address, len, *end_ptr, end_ptr);
            return -1;
        }
        begin = end_ptr +1;
        end_ptr = NULL;
    }
    if (i != 5) {
        BT_LOGE("faile to addr:%s, len:%d", address, len);
        return -1;
    }
    BT_LOGD("%02X:%02X:%02X:%02X:%02X:%02X",
                        dest[0], dest[1], dest[2], dest[3], dest[4], dest[5]);
    return 0;
}

char *proxy_description(GDBusProxy *proxy, const char *title,
						const char *description)
{
    const char *path;

    path = g_dbus_proxy_get_path(proxy);

    return g_strdup_printf("%s%s%s%s %s ",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					title, path);
}

void print_fixed_iter(const char *label, const char *name,
						DBusMessageIter *iter)
{
    dbus_bool_t *valbool;
    dbus_uint32_t *valu32;
    dbus_uint16_t *valu16;
    dbus_int16_t *vals16;
    unsigned char *byte;
    int len;

    switch (dbus_message_iter_get_arg_type(iter)) {
    case DBUS_TYPE_BOOLEAN:
        dbus_message_iter_get_fixed_array(iter, &valbool, &len);

        if (len <= 0)
            return;

        BT_LOGI("%s%s:\n", label, name);
        //rl_hexdump((void *)valbool, len * sizeof(*valbool));

        break;
    case DBUS_TYPE_UINT32:
        dbus_message_iter_get_fixed_array(iter, &valu32, &len);

        if (len <= 0)
            return;

        BT_LOGI("%s%s:\n", label, name);
        //rl_hexdump((void *)valu32, len * sizeof(*valu32));

        break;
    case DBUS_TYPE_UINT16:
        dbus_message_iter_get_fixed_array(iter, &valu16, &len);

        if (len <= 0)
            return;

        BT_LOGI("%s%s:\n", label, name);
        //rl_hexdump((void *)valu16, len * sizeof(*valu16));

        break;
    case DBUS_TYPE_INT16:
        dbus_message_iter_get_fixed_array(iter, &vals16, &len);

        if (len <= 0)
            return;

        BT_LOGI("%s%s:\n", label, name);
        //rl_hexdump((void *)vals16, len * sizeof(*vals16));

        break;
    case DBUS_TYPE_BYTE:
        dbus_message_iter_get_fixed_array(iter, &byte, &len);

        if (len <= 0)
            return;

        BT_LOGI("%s%s:\n", label, name);
        //rl_hexdump((void *)byte, len * sizeof(*byte));

        break;
    default:
        return;
    };
}

void print_iter(const char *label, const char *name,
						DBusMessageIter *iter)
{
	dbus_bool_t valbool;
	dbus_uint32_t valu32;
	dbus_uint16_t valu16;
	dbus_int16_t vals16;
	unsigned char byte;
	const char *valstr;
	DBusMessageIter subiter;
	char *entry;

	if (iter == NULL) {
		//BT_LOGI("%s%s is nil\n", label, name);
		return;
	}

	switch (dbus_message_iter_get_arg_type(iter)) {
	case DBUS_TYPE_INVALID:
		BT_LOGI("%s%s is invalid\n", label, name);
		break;
	case DBUS_TYPE_STRING:
	case DBUS_TYPE_OBJECT_PATH:
		dbus_message_iter_get_basic(iter, &valstr);
		BT_LOGI("%s%s: %s\n", label, name, valstr);
		break;
	case DBUS_TYPE_BOOLEAN:
		dbus_message_iter_get_basic(iter, &valbool);
		BT_LOGI("%s%s: %s\n", label, name,
					valbool == TRUE ? "yes" : "no");
		break;
	case DBUS_TYPE_UINT32:
		dbus_message_iter_get_basic(iter, &valu32);
		BT_LOGI("%s%s: 0x%08x\n", label, name, valu32);
		break;
	case DBUS_TYPE_UINT16:
		dbus_message_iter_get_basic(iter, &valu16);
		BT_LOGI("%s%s: 0x%04x\n", label, name, valu16);
		break;
	case DBUS_TYPE_INT16:
		dbus_message_iter_get_basic(iter, &vals16);
		BT_LOGI("%s%s: %d\n", label, name, vals16);
		break;
	case DBUS_TYPE_BYTE:
		dbus_message_iter_get_basic(iter, &byte);
		BT_LOGI("%s%s: 0x%02x\n", label, name, byte);
		break;
	case DBUS_TYPE_VARIANT:
		dbus_message_iter_recurse(iter, &subiter);
		print_iter(label, name, &subiter);
		break;
	case DBUS_TYPE_ARRAY:
		dbus_message_iter_recurse(iter, &subiter);

		if (dbus_type_is_fixed(
				dbus_message_iter_get_arg_type(&subiter))) {
			print_fixed_iter(label, name, &subiter);
			break;
		}

		while (dbus_message_iter_get_arg_type(&subiter) !=
							DBUS_TYPE_INVALID) {
			print_iter(label, name, &subiter);
			dbus_message_iter_next(&subiter);
		}
		break;
	case DBUS_TYPE_DICT_ENTRY:
		dbus_message_iter_recurse(iter, &subiter);
		entry = g_strconcat(name, " Key", NULL);
		print_iter(label, entry, &subiter);
		g_free(entry);

		entry = g_strconcat(name, " Value", NULL);
		dbus_message_iter_next(&subiter);
		print_iter(label, entry, &subiter);
		g_free(entry);
		break;
	default:
		BT_LOGI("%s%s has unsupported type\n", label, name);
		break;
	}
}

void print_proxy(GDBusProxy *proxy, const char *title, const char *description, const char *type)
{
    const char *path;

    path = g_dbus_proxy_get_path(proxy);

    BT_LOGI("%s%s%s%s %s%s\n", description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					title, path,
					type);
}

void print_property(GDBusProxy *proxy, const char *name)
{
    DBusMessageIter iter;

    if (g_dbus_proxy_get_property(proxy, name, &iter) == FALSE)
        return;

    print_iter("\t", name, &iter);
}

void print_property_changed(GDBusProxy *proxy, const char *title, const char *name,
						DBusMessageIter *iter)
{
    char *str;

    str = proxy_description(proxy, title, COLORED_CHG);
    print_iter(str, name, iter);
    g_free(str);
}

void print_device(GDBusProxy *proxy, const char *description)
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

    BT_LOGI("[%s]Device %s %s\n", description,
                    address, name);
}

void print_uuids(GDBusProxy *proxy)
{
    DBusMessageIter iter, value;

    if (g_dbus_proxy_get_property(proxy, "UUIDs", &iter) == FALSE)
        return;

    dbus_message_iter_recurse(&iter, &value);

    while (dbus_message_iter_get_arg_type(&value) == DBUS_TYPE_STRING) {
        const char *uuid, *text;

        dbus_message_iter_get_basic(&value, &uuid);

        text = bt_uuidstr_to_str(uuid);
        if (text) {
            char str[26];
            unsigned int n;

            str[sizeof(str) - 1] = '\0';

            n = snprintf(str, sizeof(str), "%s", text);
            if (n > sizeof(str) - 1) {
                str[sizeof(str) - 2] = '.';
                str[sizeof(str) - 3] = '.';
                if (str[sizeof(str) - 4] == ' ')
                    str[sizeof(str) - 4] = '.';

                n = sizeof(str) - 1;
            }

            BT_LOGI("\tUUID: %s%*c(%s)\n",
						str, 26 - n, ' ', uuid);
        } else
            BT_LOGI("\tUUID: %*c(%s)\n", 26, ' ', uuid);

        dbus_message_iter_next(&value);
    }
}

void print_adapter(GDBusProxy *proxy, const char *description, const char *type)
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

    BT_LOGI("[%s]Controller %s %s %s\n", description,
				address, name, type);

}

gboolean parse_argument_on_off(const char *arg, dbus_bool_t *value)
{
    if (!arg || !strlen(arg)) {
        BT_LOGI("Missing on/off argument\n");
        return FALSE;
    }

    if (!strcmp(arg, "on") || !strcmp(arg, "yes")) {
        *value = TRUE;
        return TRUE;
    }

    if (!strcmp(arg, "off") || !strcmp(arg, "no")) {
        *value = FALSE;
        return TRUE;
    }

    BT_LOGI("Invalid argument %s\n", arg);
    return FALSE;
}

GDBusProxy *find_proxy_by_address(GList *source, const char *address)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        GDBusProxy *proxy = list->data;
        DBusMessageIter iter;
        const char *str;

        if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
            continue;

        dbus_message_iter_get_basic(&iter, &str);

        if (!strcasecmp(str, address))
            return proxy;
    }

    return NULL;
}

struct adapter *find_parent(GList *source, GDBusProxy *device)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = list->data;

        if (device_is_child(device, adapter->proxy) == TRUE)
            return adapter;
    }
    return NULL;
}

struct adapter *find_ctrl(GList *source, const char *path)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = list->data;

        if (!strcasecmp(g_dbus_proxy_get_path(adapter->proxy), path))
            return adapter;
    }

    return NULL;
}

void show_adapter_list(GList *source)
{
    GList *list;

    for (list = g_list_first(source); list; list = g_list_next(list)) {
        struct adapter *adapter = list->data;
        print_adapter(adapter->proxy, NULL, "");
    }
}

gboolean device_is_child(GDBusProxy *device, GDBusProxy *master)
{
    DBusMessageIter iter;
    const char *adapter, *path;

    if (!master)
        return FALSE;

    if (g_dbus_proxy_get_property(device, "Adapter", &iter) == FALSE)
        return FALSE;

    dbus_message_iter_get_basic(&iter, &adapter);
    path = g_dbus_proxy_get_path(master);

    if (!strcmp(path, adapter))
        return TRUE;

    return FALSE;
}

bool find_uuid(GDBusProxy *proxy, char *uuid)
{
    bool ret = false;
    DBusMessageIter iter;
    if (g_dbus_proxy_get_property(proxy, "UUIDs", &iter)) {
        DBusMessageIter subiter;
        const char *str;
        dbus_message_iter_recurse(&iter, &subiter);
        while (dbus_message_iter_get_arg_type(&subiter) !=
                    DBUS_TYPE_INVALID) {
            dbus_message_iter_get_basic(&subiter, &str);
            BT_LOGI("%s", str);
            if (!strcmp(uuid, str)) {
                ret = true;
                //break;
            }
            dbus_message_iter_next(&subiter);
        }
    }
    return ret;
}

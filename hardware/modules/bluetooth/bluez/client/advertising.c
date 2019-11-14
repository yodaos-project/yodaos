/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2016  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <wordexp.h>

#include "gdbus/gdbus.h"
//#include "display.h"
#include "advertising.h"

#define AD_PATH "/org/bluez/advertising"
#define AD_IFACE "org.bluez.LEAdvertisement1"

struct ad_data {
	uint8_t data[25];
	uint8_t len;
};

struct service_data {
	char *uuid;
	struct ad_data data;
};

struct manufacturer_data {
	uint16_t id;
	struct ad_data data;
};

static struct ad {
	bool registered;
	char *type;
	char *local_name;
	uint16_t local_appearance;
	char **uuids;
	size_t uuids_len;
	struct service_data service;
	struct manufacturer_data manufacturer;
	bool tx_power;
	bool name;
	bool appearance;
} ad = {
	.local_appearance = UINT16_MAX,
};

static void ad_release(DBusConnection *conn)
{
	ad.registered = false;

	g_dbus_unregister_interface(conn, AD_PATH, AD_IFACE);
}

static DBusMessage *release_advertising(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	BT_LOGI("Advertising released\n");

	ad_release(conn);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable ad_methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, release_advertising) },
	{ }
};

static void register_setup(DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict;
	const char *path = AD_PATH;

    BT_LOGI("entry");
	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
				DBUS_TYPE_STRING_AS_STRING
				DBUS_TYPE_VARIANT_AS_STRING
				DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &dict);
	dbus_message_iter_close_container(iter, &dict);
}

static void register_reply(DBusMessage *message, void *user_data)
{
    struct  ad_register_data *data = user_data;
    DBusConnection *conn = data->conn;
    DBusError error;

    dbus_error_init(&error);

    BT_LOGI("entry");
    if (dbus_set_error_from_message(&error, message) == FALSE) {
        ad.registered = true;
        BT_LOGI("Advertising object registered\n");
        if (data->complete)
            data->complete(data->userdata, 0);
    } else {
        BT_LOGI("Failed to register advertisement: %s\n", error.name);
        dbus_error_free(&error);

        if (g_dbus_unregister_interface(conn, AD_PATH,
						AD_IFACE) == FALSE)
            BT_LOGI("Failed to unregister advertising object\n");
        if (data->complete)
            data->complete(data->userdata, -1);
    }
}

static gboolean get_type(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	const char *type = "peripheral";

	if (ad.type && strlen(ad.type) > 0)
		type = ad.type;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &type);

	return TRUE;
}

static gboolean uuids_exists(const GDBusPropertyTable *property, void *data)
{
	return ad.uuids_len != 0;
}

static gboolean get_uuids(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter array;
	size_t i;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "as", &array);

	for (i = 0; i < ad.uuids_len; i++)
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING,
							&ad.uuids[i]);

	dbus_message_iter_close_container(iter, &array);

	return TRUE;
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

static gboolean service_data_exists(const GDBusPropertyTable *property,
								void *data)
{
	return ad.service.uuid != NULL;
}

static gboolean get_service_data(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict;
	struct ad_data *data = &ad.service.data;
	uint8_t *val = data->data;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &dict);

	dict_append_array(&dict, ad.service.uuid, DBUS_TYPE_BYTE, &val,
								data->len);

	dbus_message_iter_close_container(iter, &dict);

	return TRUE;
}

static gboolean manufacturer_data_exists(const GDBusPropertyTable *property,
								void *data)
{
	return ad.manufacturer.id != 0;
}

static gboolean get_manufacturer_data(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict;
	struct ad_data *data = &ad.manufacturer.data;
	uint8_t *val = data->data;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{qv}", &dict);

	dict_append_basic_array(&dict, DBUS_TYPE_UINT16, &ad.manufacturer.id,
					DBUS_TYPE_BYTE, &val, data->len);

	dbus_message_iter_close_container(iter, &dict);

	return TRUE;
}

static gboolean includes_exists(const GDBusPropertyTable *property, void *data)
{
	return ad.tx_power || ad.name || ad.appearance;
}

static gboolean get_includes(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "as", &array);

	if (ad.tx_power) {
		const char *str = "tx-power";

		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &str);
	}

	if (ad.name) {
		const char *str = "local-name";

		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &str);
	}

	if (ad.appearance) {
		const char *str = "appearance";

		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &str);
	}

	dbus_message_iter_close_container(iter, &array);


	return TRUE;
}

static gboolean local_name_exits(const GDBusPropertyTable *property, void *data)
{
	return ad.local_name ? TRUE : FALSE;
}

static gboolean get_local_name(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &ad.local_name);

	return TRUE;
}

static gboolean appearance_exits(const GDBusPropertyTable *property, void *data)
{
	return ad.local_appearance != UINT16_MAX ? TRUE : FALSE;
}

static gboolean get_appearance(const GDBusPropertyTable *property,
				DBusMessageIter *iter, void *user_data)
{
	dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16,
							&ad.local_appearance);

	return TRUE;
}

static const GDBusPropertyTable ad_props[] = {
	{ "Type", "s", get_type },
	{ "ServiceUUIDs", "as", get_uuids, NULL, uuids_exists },
	{ "ServiceData", "a{sv}", get_service_data, NULL, service_data_exists },
	{ "ManufacturerData", "a{qv}", get_manufacturer_data, NULL,
						manufacturer_data_exists },
	{ "Includes", "as", get_includes, NULL, includes_exists },
	{ "LocalName", "s", get_local_name, NULL, local_name_exits },
	{ "Appearance", "q", get_appearance, NULL, appearance_exits },
	{ }
};

void ad_register(DBusConnection *conn, GDBusProxy *manager, const char *type, struct  ad_register_data *data)
{
    if (!data) return;
    if (ad.registered) {
        BT_LOGI("Advertisement is already registered\n");
        goto ERR_EXIT;
    }

    g_free(ad.type);
    ad.type = g_strdup(type);

    if (g_dbus_register_interface(conn, AD_PATH, AD_IFACE, ad_methods,
					NULL, ad_props, NULL, NULL) == FALSE) {
        BT_LOGI("Failed to register advertising object\n");
        goto ERR_EXIT;
    }

    if (g_dbus_proxy_method_call(manager, "RegisterAdvertisement",
					register_setup, register_reply,
					data, NULL) == FALSE) {
        BT_LOGI("Failed to register advertising\n");
        goto ERR_EXIT;
    }

    return;
ERR_EXIT:
    if (data->complete)
        data->complete(data->userdata, -1);
    return;
}

static void unregister_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = AD_PATH;

    BT_LOGI("entry");
	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void unregister_reply(DBusMessage *message, void *user_data)
{
    struct  ad_register_data *data = user_data;
    DBusConnection *conn = data->conn;
    DBusError error;

    dbus_error_init(&error);

    BT_LOGI("entry");
    if (dbus_set_error_from_message(&error, message) == FALSE) {
        ad.registered = false;
        BT_LOGI("Advertising object unregistered\n");
        if (g_dbus_unregister_interface(conn, AD_PATH,
							AD_IFACE) == FALSE)
            BT_LOGI("Failed to unregister advertising object\n");
        if (data->complete)
            data->complete(data->userdata, 0);
    } else {
        BT_LOGI("Failed to unregister advertisement: %s\n",
								error.name);
        if (data->complete)
            data->complete(data->userdata, -1);
        dbus_error_free(&error);
    }
}

void ad_unregister(DBusConnection *conn, GDBusProxy *manager, struct  ad_register_data *data)
{

    if (!data && manager) return;
    if (!manager) {
        ad_release(conn);
    }

    if (ad.type) {
        g_free(ad.type);
        ad.type = NULL;
    }

    if (!ad.registered)
        goto ERR_EXIT;

    if (g_dbus_proxy_method_call(manager, "UnregisterAdvertisement",
					unregister_setup, unregister_reply,
					data, NULL) == FALSE) {
        BT_LOGI("Failed to unregister advertisement method\n");
        goto ERR_EXIT;
    }

    return;
ERR_EXIT:
    if (data && data->complete)
        data->complete(data->userdata, -1);
}

void ad_advertise_uuids(DBusConnection *conn, const char *arg)
{
	g_strfreev(ad.uuids);
	ad.uuids = NULL;
	ad.uuids_len = 0;

	if (!arg || !strlen(arg))
		return;

	ad.uuids = g_strsplit(arg, " ", -1);
	if (!ad.uuids) {
		BT_LOGI("Failed to parse input\n");
		return;
	}

	ad.uuids_len = g_strv_length(ad.uuids);

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "ServiceUUIDs");
}

static void ad_clear_service(void)
{
	g_free(ad.service.uuid);
	memset(&ad.service, 0, sizeof(ad.service));
}

void ad_advertise_service(DBusConnection *conn, const char *arg)
{
	wordexp_t w;
	unsigned int i;
	struct ad_data *data;

	if (wordexp(arg, &w, WRDE_NOCMD)) {
		BT_LOGI("Invalid argument\n");
		return;
	}

	ad_clear_service();

	if (w.we_wordc == 0)
		goto done;

	ad.service.uuid = g_strdup(w.we_wordv[0]);
	data = &ad.service.data;

	for (i = 1; i < w.we_wordc; i++) {
		long int val;
		char *endptr = NULL;

		if (i >= G_N_ELEMENTS(data->data)) {
			BT_LOGI("Too much data\n");
			goto done;
		}

		val = strtol(w.we_wordv[i], &endptr, 0);
		if (!endptr || *endptr != '\0' || val > UINT8_MAX) {
			BT_LOGI("Invalid value at index %d\n", i);
			ad_clear_service();
			goto done;
		}

		data->data[data->len] = val;
		data->len++;
	}

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "ServiceData");

done:
	wordfree(&w);
}

static void ad_clear_manufacturer(void)
{
	memset(&ad.manufacturer, 0, sizeof(ad.manufacturer));
}

void ad_advertise_manufacturer(DBusConnection *conn, const char *arg)
{
	wordexp_t w;
	unsigned int i;
	char *endptr = NULL;
	long int val;
	struct ad_data *data;

	if (wordexp(arg, &w, WRDE_NOCMD)) {
		BT_LOGI("Invalid argument\n");
		return;
	}

	ad_clear_manufacturer();

	if (w.we_wordc == 0)
		goto done;

	val = strtol(w.we_wordv[0], &endptr, 0);
	if (!endptr || *endptr != '\0' || val > UINT16_MAX) {
		BT_LOGI("Invalid manufacture id\n");
		goto done;
	}

	ad.manufacturer.id = val;
	data = &ad.manufacturer.data;

	for (i = 1; i < w.we_wordc; i++) {
		if (i >= G_N_ELEMENTS(data->data)) {
			BT_LOGI("Too much data\n");
			goto done;
		}

		val = strtol(w.we_wordv[i], &endptr, 0);
		if (!endptr || *endptr != '\0' || val > UINT8_MAX) {
			BT_LOGI("Invalid value at index %d\n", i);
			ad_clear_service();
			goto done;
		}

		data->data[data->len] = val;
		data->len++;
	}

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE,
							"ManufacturerData");

done:
	wordfree(&w);
}

void ad_advertise_tx_power(DBusConnection *conn, bool value)
{
	if (ad.tx_power == value)
		return;

	ad.tx_power = value;

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "Includes");
}

void ad_advertise_name(DBusConnection *conn, bool value)
{
	if (ad.name == value)
		return;

	ad.name = value;

	if (!value) {
		g_free(ad.local_name);
		ad.local_name = NULL;
	}

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "Includes");
}

void ad_advertise_local_name(DBusConnection *conn, const char *name)
{
	if (ad.local_name && !strcmp(name, ad.local_name))
		return;

	g_free(ad.local_name);
	ad.local_name = strdup(name);

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "LocalName");
}

void ad_advertise_appearance(DBusConnection *conn, bool value)
{
	if (ad.appearance == value)
		return;

	ad.appearance = value;

	if (!value)
		ad.local_appearance = UINT16_MAX;

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "Includes");
}

void ad_advertise_local_appearance(DBusConnection *conn, uint16_t value)
{
	if (ad.local_appearance == value)
		return;

	ad.local_appearance = value;

	g_dbus_emit_property_changed(conn, AD_PATH, AD_IFACE, "Appearance");
}

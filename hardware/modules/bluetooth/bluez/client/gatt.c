/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2014  Intel Corporation. All rights reserved.
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
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/uio.h>
#include <wordexp.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <glib.h>

#include "shared/util.h"
#include "shared/queue.h"
#include "shared/io.h"
#include "gdbus/gdbus.h"
//#include "display.h"
#include "gatt.h"

#include <hardware/bt_common.h>

#define APP_PATH "/org/bluez/app"

/* String display constants */
#define COLORED_NEW	 "NEW"
#define COLORED_CHG	 "CHG"
#define COLORED_DEL	 "DEL"

static ble_callbacks_t event_listener;
static void *          event_holder;

void gatt_set_event_listener(ble_callbacks_t listener, void *data)
{
    event_listener = listener;
    event_holder = data;
}

struct desc {
	struct chrc *chrc;
	char *path;
	char *uuid;
	char **flags;
	int value_len;
	uint8_t *value;
};

struct chrc {
	struct service *service;
	char *path;
	char *uuid;
	char **flags;
	bool notifying;
	GList *descs;
	int send_value_len;
	uint8_t *send_value;
};

struct service {
	DBusConnection *conn;
	char *path;
	char *uuid;
	bool primary;
	GList *chrcs;
};

static GList *local_services;
static GList *services;
static GList *characteristics;
static GList *descriptors;
static GList *managers;
static GList *uuids;

static GDBusProxy *write_proxy;
static struct io *write_io;
static uint16_t write_mtu;

static GDBusProxy *notify_proxy;
static struct io *notify_io;
static uint16_t notify_mtu;


static struct chrc *chrc_find(const char *pattern);

static void print_service(struct service *service, const char *description)
{
	const char *text;

	text = bt_uuidstr_to_str(service->uuid);
	if (!text)
		BT_LOGI("%s%s%s%s Service\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					service->primary ? "Primary" :
					"Secondary",
					service->path, service->uuid);
	else
		BT_LOGI("%s%s%s%s Service\n\t%s\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					service->primary ? "Primary" :
					"Secondary",
					service->path, service->uuid, text);
}

static void print_service_proxy(GDBusProxy *proxy, const char *description)
{
	struct service service;
	DBusMessageIter iter;
	const char *uuid;
	dbus_bool_t primary;

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &uuid);

	if (g_dbus_proxy_get_property(proxy, "Primary", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &primary);

	service.path = (char *) g_dbus_proxy_get_path(proxy);
	service.uuid = (char *) uuid;
	service.primary = primary;

	print_service(&service, description);
}

void gatt_add_service(GDBusProxy *proxy)
{
	services = g_list_append(services, proxy);

	print_service_proxy(proxy, COLORED_NEW);
}

void gatt_remove_service(GDBusProxy *proxy)
{
	GList *l;

	l = g_list_find(services, proxy);
	if (!l)
		return;

	services = g_list_delete_link(services, l);

	print_service_proxy(proxy, COLORED_DEL);
}

static void print_chrc(struct chrc *chrc, const char *description)
{
	const char *text;

	text = bt_uuidstr_to_str(chrc->uuid);
	if (!text)
		BT_LOGI("%s%s%sCharacteristic\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					chrc->path, chrc->uuid);
	else
		BT_LOGI("%s%s%sCharacteristic\n\t%s\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					chrc->path, chrc->uuid, text);
}

static void print_characteristic(GDBusProxy *proxy, const char *description)
{
	struct chrc chrc;
	DBusMessageIter iter;
	const char *uuid;

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &uuid);

	chrc.path = (char *) g_dbus_proxy_get_path(proxy);
	chrc.uuid = (char *) uuid;

	print_chrc(&chrc, description);
}

static gboolean chrc_is_child(GDBusProxy *characteristic)
{
	GList *l;
	DBusMessageIter iter;
	const char *service, *path;

	if (!g_dbus_proxy_get_property(characteristic, "Service", &iter))
		return FALSE;

	dbus_message_iter_get_basic(&iter, &service);

	for (l = services; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;

		path = g_dbus_proxy_get_path(proxy);

		if (!strcmp(path, service))
			return TRUE;
	}

	return FALSE;
}

void gatt_add_characteristic(GDBusProxy *proxy)
{
	if (!chrc_is_child(proxy))
		return;

	characteristics = g_list_append(characteristics, proxy);

	print_characteristic(proxy, COLORED_NEW);
}

static void notify_io_destroy(void)
{
	io_destroy(notify_io);
	notify_io = NULL;
	notify_proxy = NULL;
	notify_mtu = 0;
}

static void write_io_destroy(void)
{
	io_destroy(write_io);
	write_io = NULL;
	write_proxy = NULL;
	write_mtu = 0;
}

void gatt_remove_characteristic(GDBusProxy *proxy)
{
	GList *l;

	l = g_list_find(characteristics, proxy);
	if (!l)
		return;

	characteristics = g_list_delete_link(characteristics, l);

	print_characteristic(proxy, COLORED_DEL);

	if (write_proxy == proxy)
		write_io_destroy();
	else if (notify_proxy == proxy)
		notify_io_destroy();
}

static void print_desc(struct desc *desc, const char *description)
{
	const char *text;

	text = bt_uuidstr_to_str(desc->uuid);
	if (!text)
		BT_LOGI("%s%s%sDescriptor\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					desc->path, desc->uuid);
	else
		BT_LOGI("%s%s%sDescriptor\n\t%s\n\t%s\n\t%s\n",
					description ? "[" : "",
					description ? : "",
					description ? "] " : "",
					desc->path, desc->uuid, text);
}

static void print_descriptor(GDBusProxy *proxy, const char *description)
{
	struct desc desc;
	DBusMessageIter iter;
	const char *uuid;

	if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
		return;

	dbus_message_iter_get_basic(&iter, &uuid);

	desc.path = (char *) g_dbus_proxy_get_path(proxy);
	desc.uuid = (char *) uuid;

	print_desc(&desc, description);
}

static gboolean descriptor_is_child(GDBusProxy *characteristic)
{
	GList *l;
	DBusMessageIter iter;
	const char *service, *path;

	if (!g_dbus_proxy_get_property(characteristic, "Characteristic", &iter))
		return FALSE;

	dbus_message_iter_get_basic(&iter, &service);

	for (l = characteristics; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;

		path = g_dbus_proxy_get_path(proxy);

		if (!strcmp(path, service))
			return TRUE;
	}

	return FALSE;
}

void gatt_add_descriptor(GDBusProxy *proxy)
{
	if (!descriptor_is_child(proxy))
		return;

	descriptors = g_list_append(descriptors, proxy);

	print_descriptor(proxy, COLORED_NEW);
}

void gatt_remove_descriptor(GDBusProxy *proxy)
{
	GList *l;

	l = g_list_find(descriptors, proxy);
	if (!l)
		return;

	descriptors = g_list_delete_link(descriptors, l);

	print_descriptor(proxy, COLORED_DEL);
}

static void list_attributes(const char *path, GList *source)
{
	GList *l;

	for (l = source; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;
		const char *proxy_path;

		proxy_path = g_dbus_proxy_get_path(proxy);

		if (!g_str_has_prefix(proxy_path, path))
			continue;

		if (source == services) {
			print_service_proxy(proxy, NULL);
			list_attributes(proxy_path, characteristics);
		} else if (source == characteristics) {
			print_characteristic(proxy, NULL);
			list_attributes(proxy_path, descriptors);
		} else if (source == descriptors)
			print_descriptor(proxy, NULL);
	}
}

void gatt_list_attributes(const char *path)
{
	list_attributes(path, services);
}

static GDBusProxy *select_proxy(const char *path, GList *source)
{
	GList *l;

	for (l = source; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;

		if (strcmp(path, g_dbus_proxy_get_path(proxy)) == 0)
			return proxy;
	}

	return NULL;
}

static GDBusProxy *select_attribute(const char *path)
{
	GDBusProxy *proxy;

	proxy = select_proxy(path, services);
	if (proxy)
		return proxy;

	proxy = select_proxy(path, characteristics);
	if (proxy)
		return proxy;

	return select_proxy(path, descriptors);
}

static GDBusProxy *select_proxy_by_uuid(GDBusProxy *parent, const char *uuid,
					GList *source)
{
	GList *l;
	const char *value;
	DBusMessageIter iter;

	for (l = source; l; l = g_list_next(l)) {
		GDBusProxy *proxy = l->data;

		if (parent && !g_str_has_prefix(g_dbus_proxy_get_path(proxy),
						g_dbus_proxy_get_path(parent)))
			continue;

		if (g_dbus_proxy_get_property(proxy, "UUID", &iter) == FALSE)
			continue;

		dbus_message_iter_get_basic(&iter, &value);

		if (strcasecmp(uuid, value) == 0)
			return proxy;
	}

	return NULL;
}

static GDBusProxy *select_attribute_by_uuid(GDBusProxy *parent,
							const char *uuid)
{
	GDBusProxy *proxy;

	proxy = select_proxy_by_uuid(parent, uuid, services);
	if (proxy)
		return proxy;

	proxy = select_proxy_by_uuid(parent, uuid, characteristics);
	if (proxy)
		return proxy;

	return select_proxy_by_uuid(parent, uuid, descriptors);
}

GDBusProxy *gatt_select_attribute(GDBusProxy *parent, const char *arg)
{
	if (arg[0] == '/')
		return select_attribute(arg);

	if (parent) {
		GDBusProxy *proxy = select_attribute_by_uuid(parent, arg);
		if (proxy)
			return proxy;
	}

	return select_attribute_by_uuid(parent, arg);
}

static char *attribute_generator(const char *text, int state, GList *source)
{
	static int index, len;
	GList *list;

	if (!state) {
		index = 0;
		len = strlen(text);
	}

	for (list = g_list_nth(source, index); list;
						list = g_list_next(list)) {
		GDBusProxy *proxy = list->data;
		const char *path;

		index++;

		path = g_dbus_proxy_get_path(proxy);

		if (!strncmp(path, text, len))
			return strdup(path);
        }

	return NULL;
}

char *gatt_attribute_generator(const char *text, int state)
{
	static GList *list = NULL;

	if (!state) {
		GList *list1;

		if (list) {
			g_list_free(list);
			list = NULL;
		}

		list1 = g_list_copy(characteristics);
		list1 = g_list_concat(list1, g_list_copy(descriptors));

		list = g_list_copy(services);
		list = g_list_concat(list, list1);
	}

	return attribute_generator(text, state, list);
}

static void read_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	DBusMessageIter iter, array;
	uint8_t *value;
	int len;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to read: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	dbus_message_iter_init(message, &iter);

	if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY) {
		BT_LOGI("Invalid response to read\n");
		return;
	}

	dbus_message_iter_recurse(&iter, &array);
	dbus_message_iter_get_fixed_array(&array, &value, &len);

	if (len < 0) {
		BT_LOGI("Unable to parse value\n");
		return;
	}

	//rl_hexdump(value, len);
}

static void read_setup(DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter dict;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&dict);
	/* TODO: Add offset support */
	dbus_message_iter_close_container(iter, &dict);
}

static void read_attribute(GDBusProxy *proxy)
{
	if (g_dbus_proxy_method_call(proxy, "ReadValue", read_setup, read_reply,
							NULL, NULL) == FALSE) {
		BT_LOGI("Failed to read\n");
		return;
	}

	BT_LOGI("Attempting to read %s\n", g_dbus_proxy_get_path(proxy));
}

void gatt_read_attribute(GDBusProxy *proxy)
{
	const char *iface;

	iface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(iface, CHRC_INTERFACE) ||
				!strcmp(iface, DESC_INTERFACE)) {
		read_attribute(proxy);
		return;
	}

	BT_LOGI("Unable to read attribute %s\n",
						g_dbus_proxy_get_path(proxy));
}

static void write_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to write: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}
}

static void write_setup(DBusMessageIter *iter, void *user_data)
{
	struct iovec *iov = user_data;
	DBusMessageIter array, dict;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array);
	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&iov->iov_base, iov->iov_len);
	dbus_message_iter_close_container(iter, &array);

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&dict);
	/* TODO: Add offset support */
	dbus_message_iter_close_container(iter, &dict);
}

static void write_attribute(GDBusProxy *proxy, char *arg)
{
	struct iovec iov;
	uint8_t value[512];
	char *entry;
	unsigned int i;

	for (i = 0; (entry = strsep(&arg, " \t")) != NULL; i++) {
		long int val;
		char *endptr = NULL;

		if (*entry == '\0')
			continue;

		if (i >= G_N_ELEMENTS(value)) {
			BT_LOGI("Too much data\n");
			return;
		}

		val = strtol(entry, &endptr, 0);
		if (!endptr || *endptr != '\0' || val > UINT8_MAX) {
			BT_LOGI("Invalid value at index %d\n", i);
			return;
		}

		value[i] = val;
	}

	iov.iov_base = arg;
	iov.iov_len = strlen(arg);

	/* Write using the fd if it has been acquired and fit the MTU */
	if (proxy == write_proxy && (write_io && write_mtu >= i)) {
		BT_LOGI("Attempting to write fd %d\n", io_get_fd(write_io));
		if (io_send(write_io, &iov, 1) < 0) {
			BT_LOGI("Failed to write: %s", strerror(errno));
			return;
		}
		return;
	}

	if (g_dbus_proxy_method_call(proxy, "WriteValue", write_setup,
					write_reply, &iov, NULL) == FALSE) {
		BT_LOGI("Failed to write\n");
		return;
	}

	BT_LOGI("Attempting to write %s\n", g_dbus_proxy_get_path(proxy));
}

void gatt_write_attribute(GDBusProxy *proxy, const char *arg)
{
	const char *iface;

	iface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(iface, CHRC_INTERFACE) ||
				!strcmp(iface, DESC_INTERFACE)) {
		write_attribute(proxy, (char *) arg);
		return;
	}

	BT_LOGI("Unable to write attribute %s\n",
						g_dbus_proxy_get_path(proxy));
}

static bool pipe_read(struct io *io, void *user_data)
{
	uint8_t buf[512];
	int fd = io_get_fd(io);
	ssize_t bytes_read;

	if (io != notify_io)
		return true;

	bytes_read = read(fd, buf, sizeof(buf));
	if (bytes_read < 0)
		return false;

	BT_LOGI("[" COLORED_CHG "] %s Notification:\n",
			g_dbus_proxy_get_path(notify_proxy));
	//rl_hexdump(buf, bytes_read);

	return true;
}

static bool pipe_hup(struct io *io, void *user_data)
{
	BT_LOGI("%s closed\n", io == notify_io ? "Notify" : "Write");

	if (io == notify_io)
		notify_io_destroy();
	else
		write_io_destroy();

	return false;
}

static struct io *pipe_io_new(int fd)
{
	struct io *io;

	io = io_new(fd);

	io_set_close_on_destroy(io, true);

	io_set_read_handler(io, pipe_read, NULL, NULL);

	io_set_disconnect_handler(io, pipe_hup, NULL, NULL);

	return io;
}

static void acquire_write_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	int fd;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to acquire write: %s\n", error.name);
		dbus_error_free(&error);
		write_proxy = NULL;
		return;
	}

	if (write_io)
		write_io_destroy();

	if ((dbus_message_get_args(message, NULL, DBUS_TYPE_UNIX_FD, &fd,
					DBUS_TYPE_UINT16, &write_mtu,
					DBUS_TYPE_INVALID) == false)) {
		BT_LOGI("Invalid AcquireWrite response\n");
		return;
	}

	BT_LOGI("AcquireWrite success: fd %d MTU %u\n", fd, write_mtu);

	write_io = pipe_io_new(fd);
}

void gatt_acquire_write(GDBusProxy *proxy, const char *arg)
{
	const char *iface;

	iface = g_dbus_proxy_get_interface(proxy);
	if (strcmp(iface, CHRC_INTERFACE)) {
		BT_LOGI("Unable to acquire write: %s not a characteristic\n",
						g_dbus_proxy_get_path(proxy));
		return;
	}

	if (g_dbus_proxy_method_call(proxy, "AcquireWrite", NULL,
				acquire_write_reply, NULL, NULL) == FALSE) {
		BT_LOGI("Failed to AcquireWrite\n");
		return;
	}

	write_proxy = proxy;
}

void gatt_release_write(GDBusProxy *proxy, const char *arg)
{
	if (proxy != write_proxy || !write_io) {
		BT_LOGI("Write not acquired\n");
		return;
	}

	write_io_destroy();
}

static void acquire_notify_reply(DBusMessage *message, void *user_data)
{
	DBusError error;
	int fd;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to acquire notify: %s\n", error.name);
		dbus_error_free(&error);
		write_proxy = NULL;
		return;
	}

	if (notify_io) {
		io_destroy(notify_io);
		notify_io = NULL;
	}

	notify_mtu = 0;

	if ((dbus_message_get_args(message, NULL, DBUS_TYPE_UNIX_FD, &fd,
					DBUS_TYPE_UINT16, &notify_mtu,
					DBUS_TYPE_INVALID) == false)) {
		BT_LOGI("Invalid AcquireNotify response\n");
		return;
	}

	BT_LOGI("AcquireNotify success: fd %d MTU %u\n", fd, notify_mtu);

	notify_io = pipe_io_new(fd);
}

void gatt_acquire_notify(GDBusProxy *proxy, const char *arg)
{
	const char *iface;

	iface = g_dbus_proxy_get_interface(proxy);
	if (strcmp(iface, CHRC_INTERFACE)) {
		BT_LOGI("Unable to acquire notify: %s not a characteristic\n",
						g_dbus_proxy_get_path(proxy));
		return;
	}

	if (g_dbus_proxy_method_call(proxy, "AcquireNotify", NULL,
				acquire_notify_reply, NULL, NULL) == FALSE) {
		BT_LOGI("Failed to AcquireNotify\n");
		return;
	}

	notify_proxy = proxy;
}

void gatt_release_notify(GDBusProxy *proxy, const char *arg)
{
	if (proxy != notify_proxy || !notify_io) {
		BT_LOGI("Write not acquired\n");
		return;
	}

	notify_io_destroy();
}

static void notify_reply(DBusMessage *message, void *user_data)
{
	bool enable = GPOINTER_TO_UINT(user_data);
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to %s notify: %s\n",
				enable ? "start" : "stop", error.name);
		dbus_error_free(&error);
		return;
	}

	BT_LOGI("Notify %s\n", enable == TRUE ? "started" : "stopped");
}

static void notify_attribute(GDBusProxy *proxy, bool enable)
{
	const char *method;

	if (enable == TRUE)
		method = "StartNotify";
	else
		method = "StopNotify";

	if (g_dbus_proxy_method_call(proxy, method, NULL, notify_reply,
				GUINT_TO_POINTER(enable), NULL) == FALSE) {
		BT_LOGI("Failed to %s notify\n", enable ? "start" : "stop");
		return;
	}
}

void gatt_notify_attribute(GDBusProxy *proxy, bool enable)
{
	const char *iface;

	iface = g_dbus_proxy_get_interface(proxy);
	if (!strcmp(iface, CHRC_INTERFACE)) {
		notify_attribute(proxy, enable);
		return;
	}

	BT_LOGI("Unable to notify attribute %s\n",
						g_dbus_proxy_get_path(proxy));
}

static void register_app_setup(DBusMessageIter *iter, void *user_data)
{
	DBusMessageIter opt;
	const char *path = "/";

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
					DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING
					DBUS_TYPE_STRING_AS_STRING
					DBUS_TYPE_VARIANT_AS_STRING
					DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
					&opt);
	dbus_message_iter_close_container(iter, &opt);

}

static void register_app_reply(DBusMessage *message, void *user_data)
{
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to register application: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	BT_LOGI("Application registered\n");
}

void gatt_add_manager(GDBusProxy *proxy)
{
    const char *path = g_dbus_proxy_get_path(proxy);
    const char *iface = g_dbus_proxy_get_interface(proxy);
    BT_LOGV("path %s interface %s\n", path, iface);
    managers = g_list_append(managers, proxy);
}

void gatt_remove_manager(GDBusProxy *proxy)
{
	managers = g_list_remove(managers, proxy);
}

bool find_manager_interface(const char *interface)
{
	GList *l;

	for (l = managers; l; l = g_list_next(l)) {
		struct GDBusProxy *proxy = l->data;

        const char *iface = g_dbus_proxy_get_interface(proxy);
		/* match object path */
		if (!strcmp(iface, interface))
			return true;
	}

	return false;
}

static int match_proxy(const void *a, const void *b)
{
	GDBusProxy *proxy1 = (void *) a;
	GDBusProxy *proxy2 = (void *) b;

	return strcmp(g_dbus_proxy_get_path(proxy1),
						g_dbus_proxy_get_path(proxy2));
}

static DBusMessage *release_profile(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	g_dbus_unregister_interface(conn, APP_PATH, PROFILE_INTERFACE);

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, release_profile) },
	{ }
};

static gboolean get_uuids(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	DBusMessageIter entry;
	GList *uuid;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
				DBUS_TYPE_STRING_AS_STRING, &entry);

	for (uuid = uuids; uuid; uuid = g_list_next(uuid->next))
		dbus_message_iter_append_basic(&entry, DBUS_TYPE_STRING,
							&uuid->data);

	dbus_message_iter_close_container(iter, &entry);

	return TRUE;
}

static const GDBusPropertyTable properties[] = {
	{ "UUIDs", "as", get_uuids },
	{ }
};

/**
 * register gatt profiles
 *
 * param : uuids or null for local service
 */
void gatt_register_manager(DBusConnection *conn, GDBusProxy *proxy)
{
	GList *l;

    BT_LOGI("### %s proxy\n", g_dbus_proxy_get_path(proxy));
	l = g_list_find_custom(managers, proxy, match_proxy);
	if (!l) {
		BT_LOGI("Unable to find GattManager proxy\n");
		return;
	}
    if (g_dbus_proxy_method_call(l->data, "RegisterApplication",
                register_app_setup,
                register_app_reply, NULL,
                NULL) == FALSE) {
        BT_LOGI("Failed register application\n");
        g_dbus_unregister_interface(conn, APP_PATH, PROFILE_INTERFACE);
        return;
    }
}
void gatt_register_app(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w)
{
	GList *l;
	unsigned int i;

    BT_LOGI("### %s proxy\n", g_dbus_proxy_get_path(proxy));
	l = g_list_find_custom(managers, proxy, match_proxy);
	if (!l) {
		BT_LOGI("Unable to find GattManager proxy\n");
		return;
	}

	for (i = 0; i < w->we_wordc; i++)
		uuids = g_list_append(uuids, g_strdup(w->we_wordv[i]));

	if (uuids) {
		if (g_dbus_register_interface(conn, APP_PATH,
						PROFILE_INTERFACE, methods,
						NULL, properties, NULL,
						NULL) == FALSE) {
			BT_LOGI("Failed to register application object\n");
			return;
		}
	}

	if (g_dbus_proxy_method_call(l->data, "RegisterApplication",
						register_app_setup,
						register_app_reply, w,
						NULL) == FALSE) {
		BT_LOGI("Failed register application\n");
		g_dbus_unregister_interface(conn, APP_PATH, PROFILE_INTERFACE);
		return;
	}
}


static void unregister_app_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		BT_LOGI("Failed to unregister application: %s\n", error.name);
		dbus_error_free(&error);
		return;
	}

	BT_LOGI("Application unregistered\n");

	if (!uuids)
		return;

	g_list_free_full(uuids, g_free);
	uuids = NULL;

	g_dbus_unregister_interface(conn, APP_PATH, PROFILE_INTERFACE);
}

static void unregister_app_setup(DBusMessageIter *iter, void *user_data)
{
	const char *path = "/";

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

void gatt_unregister_app(DBusConnection *conn, GDBusProxy *proxy)
{
	GList *l;

    BT_LOGI("entry");
	l = g_list_find_custom(managers, proxy, match_proxy);
	if (!l) {
		BT_LOGI("Unable to find GattManager proxy\n");
		return;
	}

	if (g_dbus_proxy_method_call(l->data, "UnregisterApplication",
						unregister_app_setup,
						unregister_app_reply, conn,
						NULL) == FALSE) {
		BT_LOGI("Failed unregister profile\n");
		return;
	}
}

static void desc_free(void *data)
{
	struct desc *desc = data;

	g_free(desc->path);
	g_free(desc->uuid);
	g_strfreev(desc->flags);
	g_free(desc->value);
	g_free(desc);
}

static void desc_unregister(void *data)
{
	struct desc *desc = data;

	print_desc(desc, COLORED_DEL);

	g_dbus_unregister_interface(desc->chrc->service->conn, desc->path,
						DESC_INTERFACE);
}

static void chrc_free(void *data)
{
	struct chrc *chrc = data;

	g_list_free_full(chrc->descs, desc_unregister);
	g_free(chrc->path);
	g_free(chrc->uuid);
	g_strfreev(chrc->flags);
	g_free(chrc->send_value);
	g_free(chrc);
}

static void chrc_unregister(void *data)
{
	struct chrc *chrc = data;

	print_chrc(chrc, COLORED_DEL);

	g_dbus_unregister_interface(chrc->service->conn, chrc->path,
						CHRC_INTERFACE);
}

static void service_free(void *data)
{
	struct service *service = data;

	g_list_free_full(service->chrcs, chrc_unregister);
	g_free(service->path);
	g_free(service->uuid);
	g_free(service);
}

static gboolean service_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct service *service = data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &service->uuid);

	return TRUE;
}

static gboolean service_get_primary(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct service *service = data;
	dbus_bool_t primary;

	primary = service->primary ? TRUE : FALSE;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &primary);

	return TRUE;
}

static const GDBusPropertyTable service_properties[] = {
	{ "UUID", "s", service_get_uuid },
	{ "Primary", "b", service_get_primary },
	{ }
};

static int parse_value_arg(DBusMessageIter *iter, uint8_t **value, int *len)
{
	DBusMessageIter array;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return -EINVAL;

	dbus_message_iter_recurse(iter, &array);
	dbus_message_iter_get_fixed_array(&array, value, len);

	return 0;
}

static void service_set_primary(const char *input, void *user_data)
{
	struct service *service = user_data;

	if (!strcmp(input, "yes"))
		service->primary = true;
	else if (!strcmp(input, "no")) {
		service->primary = false;
	} else {
		BT_LOGI("Invalid option: %s\n", input);
		local_services = g_list_remove(local_services, service);
		print_service(service, COLORED_DEL);
		g_dbus_unregister_interface(service->conn, service->path,
						SERVICE_INTERFACE);
	}
}

void gatt_register_service(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid)
{
	struct service *service;
	bool primary = true;

	service = g_new0(struct service, 1);
	service->conn = conn;
	service->uuid = g_strdup(uuid);
	service->path = g_strdup_printf("%s/service%p", APP_PATH, service);
	service->primary = primary;

	if (g_dbus_register_interface(conn, service->path,
					SERVICE_INTERFACE, NULL, NULL,
					service_properties, service,
					service_free) == FALSE) {
		BT_LOGI("Failed to register service object\n");
		service_free(service);
		return;
	}

	print_service(service, COLORED_NEW);

	local_services = g_list_append(local_services, service);
}

static struct service *service_find(const char *pattern)
{
	GList *l;

	for (l = local_services; l; l = g_list_next(l)) {
		struct service *service = l->data;

		/* match object path */
		if (!strcmp(service->path, pattern))
			return service;

		/* match UUID */
		if (!strcmp(service->uuid, pattern))
			return service;
	}

	return NULL;
}

void gatt_unregister_service(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid)
{
	struct service *service;

	service = service_find(uuid);
	if (!service) {
		BT_LOGI("Failed to unregister service object\n");
		return;
	}

	local_services = g_list_remove(local_services, service);

	print_service(service, COLORED_DEL);

	g_dbus_unregister_interface(service->conn, service->path,
						SERVICE_INTERFACE);
}

static gboolean chrc_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct chrc *chrc = data;
        //BT_LOGI("entry");
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &chrc->uuid);

	return TRUE;
}

static gboolean chrc_get_service(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct chrc *chrc = data;
        //BT_LOGI("entry");
	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
						&chrc->service->path);

	return TRUE;
}

static gboolean chrc_get_value(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct chrc *chrc = data;
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array);

	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&chrc->send_value, chrc->send_value_len);

	dbus_message_iter_close_container(iter, &array);

    BT_LOGI("get value : %s", chrc->send_value);
#if 0
    if (event_listener) {
        BT_BLE_MSG msg = {0};
        char src_uuid[5] = {0} ;

        if (strlen(chrc->uuid) >8) {
            int i;
            for (i = 0; i < 4; i++) {
                src_uuid[i] = chrc->uuid[4+ i];
            }
        }
        msg.ser_conf.uuid= (uint16_t)strtol(src_uuid, NULL, 16);
        msg.ser_conf.status= 0;
        BT_LOGI("BT_BLE_SE_CONF_EVT uuid 0x%x", data.ser_conf.conn_id);
        event_listener(event_holder, BT_BLE_SE_CONF_EVT, &msg);
    }
#endif
	return TRUE;
}

static gboolean chrc_get_notifying(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct chrc *chrc = data;
	dbus_bool_t value;

	value = chrc->notifying ? TRUE : FALSE;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &value);
    BT_LOGI("entry value %d", value);
	return TRUE;
}

static gboolean chrc_get_flags(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct chrc *chrc = data;
	int i;
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "s", &array);

	for (i = 0; chrc->flags[i]; i++)
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING,
							&chrc->flags[i]);

	dbus_message_iter_close_container(iter, &array);
    //BT_LOGI("entry");
	return TRUE;
}

int chrc_send(const char* uuid, const uint8_t *value, int len)
{
    struct chrc *chrc = NULL;

    chrc = chrc_find(uuid);
    if (!chrc) {
        BT_LOGI("Failed to get characteristic object\n");
        return -1;
    }
    if (!chrc->service) return -1;
    g_free(chrc->send_value);
    chrc->send_value = (unsigned char*)g_memdup(value, len);
    chrc->send_value_len = len;

    g_dbus_emit_property_changed_full(chrc->service->conn, chrc->path, CHRC_INTERFACE, "Value", G_DBUS_PROPERTY_CHANGED_FLAG_FLUSH);
    return 0;
}


static void chrc_set_value(const GDBusPropertyTable *property,
                DBusMessageIter *iter,
                GDBusPendingPropertySet id, void *data)
{
    struct chrc *chrc = data;
    uint8_t *value;
    int len;

    printf("Characteristic(%s): Set('Value', ...)\n", chrc->uuid);

    if (parse_value_arg(iter, &value, &len)) {
        printf("Invalid value for Set('Value'...)\n");
        g_dbus_pending_property_error(id,
                    "org.bluez.Error" ".InvalidArguments",
                    "Invalid arguments in method call");
        return;
    }
    int i;
    for (i = 0; i < len; i++)
    {
        printf(" %02x", value[i]);
    }
    printf("\n");

   g_dbus_emit_property_changed(chrc->service->conn, chrc->path, CHRC_INTERFACE, "Value");

    g_dbus_pending_property_success(id);
}

static const GDBusPropertyTable chrc_properties[] = {
	{ "UUID", "s", chrc_get_uuid, NULL, NULL },
	{ "Service", "o", chrc_get_service, NULL, NULL },
	{ "Value", "ay", chrc_get_value, chrc_set_value, NULL },
	{ "Notifying", "b", chrc_get_notifying, NULL, NULL },
	{ "Flags", "as", chrc_get_flags, NULL, NULL },
	{ }
};

static DBusMessage *read_value(DBusMessage *msg, uint8_t *value,
						uint16_t value_len)
{
	DBusMessage *reply;
	DBusMessageIter iter, array;

	reply = g_dbus_create_reply(msg, DBUS_TYPE_INVALID);

	dbus_message_iter_init_append(reply, &iter);
	dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array);
	dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
						&value, value_len);
	dbus_message_iter_close_container(&iter, &array);

    BT_LOGI("%s",value);
	return reply;
}

static DBusMessage *chrc_read_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct chrc *chrc = user_data;
        BT_LOGI("[" COLORED_CHG "] Attribute %s read\n" , chrc->path);

	return read_value(msg, chrc->send_value, chrc->send_value_len);
}

static void chrc_listern_get_write(char *uuid, uint8_t *value, int len)
{

    if (event_listener) {
        BT_BLE_MSG msg = {0};
        char src_uuid[5] = {0} ;

        if (strlen(uuid) >8) {
            int i;
            for (i = 0; i < 4; i++) {
                src_uuid[i] = uuid[4+ i];
            }
        }
        //BT_LOGI("src_uuid 0x%s, strlen = %d", src_uuid, strlen(src_uuid));
        len = len > BT_BLE_MAX_ATTR_LEN? BT_BLE_MAX_ATTR_LEN : len;
        msg.ser_write.uuid = (uint16_t)strtol(src_uuid, NULL, 16);
        msg.ser_write.len = len;
        memcpy(msg.ser_write.value, value, len);
        msg.ser_write.offset = 0;
        msg.ser_write.status = 0;
        BT_LOGI("uuid 0x%x, len = %d", msg.ser_write.uuid, len);
        event_listener(event_holder, BT_BLE_SE_WRITE_EVT, &msg);
    }

}


static DBusMessage *chrc_write_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct chrc *chrc = user_data;
	DBusMessageIter iter;
    uint8_t *get_value;
    int get_value_len = 0;
	dbus_message_iter_init(msg, &iter);

	if (parse_value_arg(&iter, &get_value, &get_value_len))
		return g_dbus_create_error(msg,
					"org.bluez.Error.InvalidArguments",
					NULL);

	BT_LOGI("[" COLORED_CHG "] Attribute %s written\n" , chrc->path);

	//g_dbus_emit_property_changed(conn, chrc->path, CHRC_INTERFACE, "Value");

        BT_LOGI("%s", get_value);
        chrc_listern_get_write(chrc->uuid, get_value, get_value_len);
	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *chrc_start_notify(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct chrc *chrc = user_data;

	//if (!chrc->notifying)
	//	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);

	chrc->notifying = true;
	BT_LOGI("[" COLORED_CHG "] Attribute %s notifications enabled",
							chrc->path);
	g_dbus_emit_property_changed(conn, chrc->path, CHRC_INTERFACE,
							"Notifying");

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *chrc_stop_notify(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct chrc *chrc = user_data;

	//if (chrc->notifying)
	//	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);

	chrc->notifying = false;
	BT_LOGI("[" COLORED_CHG "] Attribute %s notifications disabled",
							chrc->path);
	g_dbus_emit_property_changed(conn, chrc->path, CHRC_INTERFACE,
							"Notifying");

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *chrc_confirm(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct chrc *chrc = user_data;

	BT_LOGI("Attribute %s indication confirm received", chrc->path);

    if (event_listener) {
        BT_BLE_MSG msg = {0};
        char src_uuid[5] = {0} ;

        if (strlen(chrc->uuid) >8) {
            int i;
            for (i = 0; i < 4; i++) {
                src_uuid[i] = chrc->uuid[4+ i];
            }
        }
        msg.ser_conf.uuid= (uint16_t)strtol(src_uuid, NULL, 16);
        msg.ser_conf.status= 0;
        BT_LOGI("uuid 0x%x confirm", msg.ser_conf.conn_id);
        event_listener(event_holder, BT_BLE_SE_CONF_EVT, &msg);
    }
	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable chrc_methods[] = {
	{ GDBUS_ASYNC_METHOD("ReadValue", GDBUS_ARGS({ "options", "a{sv}" }),
					GDBUS_ARGS({ "value", "ay" }),
					chrc_read_value) },
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, chrc_write_value) },
	{ GDBUS_ASYNC_METHOD("StartNotify", NULL, NULL, chrc_start_notify) },
	{ GDBUS_METHOD("StopNotify", NULL, NULL, chrc_stop_notify) },
	{ GDBUS_METHOD("Confirm", NULL, NULL, chrc_confirm) },
	{ }
};

static uint8_t *str2bytearray(char *arg, int *val_len)
{
	uint8_t value[512];
	char *entry;
	unsigned int i;

	for (i = 0; (entry = strsep(&arg, " \t")) != NULL; i++) {
		long int val;
		char *endptr = NULL;

		if (*entry == '\0')
			continue;

		if (i >= G_N_ELEMENTS(value)) {
			BT_LOGI("Too much data\n");
			return NULL;
		}

		val = strtol(entry, &endptr, 0);
		if (!endptr || *endptr != '\0' || val > UINT8_MAX) {
			BT_LOGI("Invalid value at index %d\n", i);
			return NULL;
		}

		value[i] = val;
	}

	*val_len = i;

	return g_memdup(value, i);
}
/*
static void chrc_set_value(const char *input, void *user_data)
{
	struct chrc *chrc = user_data;

	g_free(chrc->value);

	chrc->value = str2bytearray((char *) input, &chrc->value_len);
}
*/
void gatt_register_chrc_full(DBusConnection *conn, GDBusProxy *proxy,
        const char *uuid,
        const char *flags, // Read,Write...
        char   *value,
        int   len
        )
{
	struct service *service;
	struct chrc *chrc;

	if (!local_services) {
		BT_LOGI("No service registered\n");
		return;
	}

	service = g_list_last(local_services)->data;

	chrc = g_new0(struct chrc, 1);
	chrc->service = service;
	chrc->uuid = g_strdup(uuid);
	chrc->path = g_strdup_printf("%s/chrc%p", service->path, chrc);
	chrc->flags = g_strsplit(flags, ",", -1);

    if (len > 0 && value) {
        chrc->send_value = g_memdup(value, len);
        chrc->send_value_len = len;
    }
    else {
        chrc->send_value = NULL;
        chrc->send_value_len = 0;
    }

    BT_LOGV("register chrc %s services %s %s\n", chrc->uuid, service->uuid, service->path);
	if (g_dbus_register_interface(conn, chrc->path, CHRC_INTERFACE,
					chrc_methods, NULL, chrc_properties,
					chrc, chrc_free) == FALSE) {
		BT_LOGI("Failed to register characteristic object\n");
		chrc_free(chrc);
		return;
	}

	service->chrcs = g_list_append(service->chrcs, chrc);

	print_chrc(chrc, COLORED_NEW);
}

void gatt_register_chrc(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w)
{
	struct service *service;
	struct chrc *chrc;

	if (!local_services) {
		BT_LOGI("No service registered\n");
		return;
	}

	service = g_list_last(local_services)->data;

	chrc = g_new0(struct chrc, 1);
	chrc->service = service;
	chrc->uuid = g_strdup(w->we_wordv[0]);
	chrc->path = g_strdup_printf("%s/chrc%p", service->path, chrc);
	chrc->flags = g_strsplit(w->we_wordv[1], ",", -1);

	if (g_dbus_register_interface(conn, chrc->path, CHRC_INTERFACE,
					chrc_methods, NULL, chrc_properties,
					chrc, chrc_free) == FALSE) {
		BT_LOGI("Failed to register characteristic object\n");
		chrc_free(chrc);
		return;
	}

	service->chrcs = g_list_append(service->chrcs, chrc);

	print_chrc(chrc, COLORED_NEW);

	//rl_prompt_input(chrc->path, "Enter value:", chrc_set_value, chrc);
}

static struct chrc *chrc_find(const char *pattern)
{
	GList *l, *lc;
	struct service *service;
	struct chrc *chrc;

	for (l = local_services; l; l = g_list_next(l)) {
		service = l->data;
		for (lc = service->chrcs; lc; lc =  g_list_next(lc)) {
			chrc = lc->data;
			/* match object path */
			if (!strcmp(chrc->path, pattern))
				return chrc;

			/* match UUID */
			if (!strcmp(chrc->uuid, pattern))
				return chrc;
		}
	}

	return NULL;
}

void gatt_unregister_chrc(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid)
{
	struct chrc *chrc;

	chrc = chrc_find(uuid);
	if (!chrc) {
		BT_LOGI("Failed to unregister characteristic object\n");
		return;
	}

	chrc->service->chrcs = g_list_remove(chrc->service->chrcs, chrc);

	chrc_unregister(chrc);
}

static DBusMessage *desc_read_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct desc *desc = user_data;

	return read_value(msg, desc->value, desc->value_len);
}

static DBusMessage *desc_write_value(DBusConnection *conn, DBusMessage *msg,
							void *user_data)
{
	struct desc *desc = user_data;
	DBusMessageIter iter;

	dbus_message_iter_init(msg, &iter);

	if (parse_value_arg(&iter, &desc->value, &desc->value_len))
		return g_dbus_create_error(msg,
					"org.bluez.Error.InvalidArguments",
					NULL);

	BT_LOGI("[" COLORED_CHG "] Attribute %s written" , desc->path);

	g_dbus_emit_property_changed(conn, desc->path, CHRC_INTERFACE, "Value");

	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static const GDBusMethodTable desc_methods[] = {
	{ GDBUS_ASYNC_METHOD("ReadValue", GDBUS_ARGS({ "options", "a{sv}" }),
					GDBUS_ARGS({ "value", "ay" }),
					desc_read_value) },
	{ GDBUS_ASYNC_METHOD("WriteValue", GDBUS_ARGS({ "value", "ay" },
						{ "options", "a{sv}" }),
					NULL, desc_write_value) },
	{ }
};

static gboolean desc_get_uuid(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct desc *desc = data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &desc->uuid);

	return TRUE;
}

static gboolean desc_get_chrc(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct desc *desc = data;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
						&desc->chrc->path);

	return TRUE;
}

static gboolean desc_get_value(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct desc *desc = data;
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array);

	if (desc->value)
		dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE,
							&desc->value,
							desc->value_len);

	dbus_message_iter_close_container(iter, &array);

	return TRUE;
}

static gboolean desc_get_flags(const GDBusPropertyTable *property,
					DBusMessageIter *iter, void *data)
{
	struct desc *desc = data;
	int i;
	DBusMessageIter array;

	dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "s", &array);

	for (i = 0; desc->flags[i]; i++)
		dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING,
							&desc->flags[i]);

	dbus_message_iter_close_container(iter, &array);

	return TRUE;
}

static const GDBusPropertyTable desc_properties[] = {
	{ "UUID", "s", desc_get_uuid, NULL, NULL },
	{ "Characteristic", "o", desc_get_chrc, NULL, NULL },
	{ "Value", "ay", desc_get_value, NULL, NULL },
	{ "Flags", "as", desc_get_flags, NULL, NULL },
	{ }
};

static void desc_set_value(const char *input, void *user_data)
{
	struct desc *desc = user_data;

	g_free(desc->value);

	desc->value = str2bytearray((char *) input, &desc->value_len);
}

void gatt_register_desc(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w)
{
	struct service *service;
	struct desc *desc;

	if (!local_services) {
		BT_LOGI("No service registered\n");
		return;
	}

	service = g_list_last(local_services)->data;

	if (!service->chrcs) {
		BT_LOGI("No characteristic registered\n");
		return;
	}

	desc = g_new0(struct desc, 1);
	desc->chrc = g_list_last(service->chrcs)->data;
	desc->uuid = g_strdup(w->we_wordv[0]);
	desc->path = g_strdup_printf("%s/desc%p", desc->chrc->path, desc);
	desc->flags = g_strsplit(w->we_wordv[1], ",", -1);

	if (g_dbus_register_interface(conn, desc->path, DESC_INTERFACE,
					desc_methods, NULL, desc_properties,
					desc, desc_free) == FALSE) {
		BT_LOGI("Failed to register descriptor object\n");
		desc_free(desc);
		return;
	}

	desc->chrc->descs = g_list_append(desc->chrc->descs, desc);

	print_desc(desc, COLORED_NEW);

	//rl_prompt_input(desc->path, "Enter value:", desc_set_value, desc);
}

static struct desc *desc_find(const char *pattern)
{
	GList *l, *lc, *ld;
	struct service *service;
	struct chrc *chrc;
	struct desc *desc;

	for (l = local_services; l; l = g_list_next(l)) {
		service = l->data;

		for (lc = service->chrcs; lc; lc = g_list_next(lc)) {
			chrc = lc->data;

			for (ld = chrc->descs; ld; ld = g_list_next(ld)) {
				desc = ld->data;

				/* match object path */
				if (!strcmp(desc->path, pattern))
					return desc;

				/* match UUID */
				if (!strcmp(desc->uuid, pattern))
					return desc;
			}
		}
	}

	return NULL;
}

void gatt_unregister_desc(DBusConnection *conn, GDBusProxy *proxy,
								wordexp_t *w)
{
	struct desc *desc;

	desc = desc_find(w->we_wordv[0]);
	if (!desc) {
		BT_LOGI("Failed to unregister descriptor object\n");
		return;
	}

	desc->chrc->descs = g_list_remove(desc->chrc->descs, desc);

	desc_unregister(desc);
}

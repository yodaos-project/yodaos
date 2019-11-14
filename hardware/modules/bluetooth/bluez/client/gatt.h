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
#ifndef __GATT_H__
#define __GATT_H__

#include <wordexp.h>
#include <hardware/bt_common.h>

#define PROFILE_INTERFACE "org.bluez.GattProfile1"
#define SERVICE_INTERFACE "org.bluez.GattService1"
#define CHRC_INTERFACE "org.bluez.GattCharacteristic1"
#define DESC_INTERFACE "org.bluez.GattDescriptor1"

void gatt_set_event_listener(ble_callbacks_t listener, void *data);
bool find_manager_interface(const char *interface);
void gatt_add_service(GDBusProxy *proxy);
void gatt_remove_service(GDBusProxy *proxy);

void gatt_add_characteristic(GDBusProxy *proxy);
void gatt_remove_characteristic(GDBusProxy *proxy);

void gatt_add_descriptor(GDBusProxy *proxy);
void gatt_remove_descriptor(GDBusProxy *proxy);

void gatt_list_attributes(const char *device);
GDBusProxy *gatt_select_attribute(GDBusProxy *parent, const char *path);
char *gatt_attribute_generator(const char *text, int state);

void gatt_read_attribute(GDBusProxy *proxy);
void gatt_write_attribute(GDBusProxy *proxy, const char *arg);
void gatt_notify_attribute(GDBusProxy *proxy, bool enable);

void gatt_acquire_write(GDBusProxy *proxy, const char *arg);
void gatt_release_write(GDBusProxy *proxy, const char *arg);

void gatt_acquire_notify(GDBusProxy *proxy, const char *arg);
void gatt_release_notify(GDBusProxy *proxy, const char *arg);

void gatt_add_manager(GDBusProxy *proxy);
void gatt_remove_manager(GDBusProxy *proxy);

void gatt_register_app(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w);
void gatt_register_manager(DBusConnection *conn, GDBusProxy *proxy);
void gatt_unregister_app(DBusConnection *conn, GDBusProxy *proxy);

void gatt_register_service(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid);
void gatt_unregister_service(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid);
int chrc_send(const char* uuid, const uint8_t *value, int len);
void gatt_register_chrc(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w);
void gatt_register_chrc_full(DBusConnection *conn, GDBusProxy *proxy,
        const char *uuid,
        const char *flags, // Read,Write...
        char   *value,
        int   len
        );
void gatt_unregister_chrc(DBusConnection *conn, GDBusProxy *proxy,
								const char *uuid_or_path);

void gatt_register_desc(DBusConnection *conn, GDBusProxy *proxy, wordexp_t *w);
void gatt_unregister_desc(DBusConnection *conn, GDBusProxy *proxy,
								wordexp_t *w);
#endif

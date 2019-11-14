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

#ifndef __ADVERTISING_H__
#define __ADVERTISING_H__

#include <hardware/bt_common.h>

struct  ad_register_data
{
    int (*complete)(void *userdata, int status);
    DBusConnection *conn;
    void *userdata;
};
void ad_register(DBusConnection *conn, GDBusProxy *manager, const char *type, struct  ad_register_data *data);
void ad_unregister(DBusConnection *conn, GDBusProxy *manager, struct  ad_register_data *data);

void ad_advertise_uuids(DBusConnection *conn, const char *arg);
void ad_advertise_service(DBusConnection *conn, const char *arg);
void ad_advertise_manufacturer(DBusConnection *conn, const char *arg);
void ad_advertise_tx_power(DBusConnection *conn, bool value);
void ad_advertise_name(DBusConnection *conn, bool value);
void ad_advertise_appearance(DBusConnection *conn, bool value);
void ad_advertise_local_name(DBusConnection *conn, const char *name);
void ad_advertise_local_appearance(DBusConnection *conn, uint16_t value);
#endif

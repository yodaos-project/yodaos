/*****************************************************************************
**
**  Name:           app_common.h
**
**  Description:    Bluetooth common application
**
**  Copyright (c) 2010-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef __APP_COMMON_H__
#define __APP_COMMON_H__

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>
#include "gdbus/gdbus.h"

#include <hardware/bt_common.h>


#define COLORED_NEW	"NEW"
#define COLORED_CHG	        "CHG"
#define COLORED_DEL	        "DEL"

#define PROMPT_ON	 "[bluetooth] #"
#define PROMPT_OFF	"Waiting to connect to bluetoothd..."

#define DEVICE_INTERFACE	"org.bluez.Device1"

struct adapter {
	GDBusProxy *proxy;
	GDBusProxy *ad_proxy;
	GList *devices;
};

int bdcmp(BTAddr bd_addr1, BTAddr bd_addr2);

int bd_strtoba(BTAddr addr, const char *address);

char *proxy_description(GDBusProxy *proxy, const char *title,
						const char *description);
void print_fixed_iter(const char *label, const char *name, DBusMessageIter *iter);

void print_iter(const char *label, const char *name, DBusMessageIter *iter);

void print_proxy(GDBusProxy *proxy, const char *title, const char *description, const char *type);

void print_property(GDBusProxy *proxy, const char *name);

void print_property_changed(GDBusProxy *proxy, const char *title, const char *name,
						DBusMessageIter *iter);

void print_device(GDBusProxy *proxy, const char *description);

void print_uuids(GDBusProxy *proxy);

void print_adapter(GDBusProxy *proxy, const char *description, const char *type);

gboolean parse_argument_on_off(const char *arg, dbus_bool_t *value);

GDBusProxy *find_proxy_by_address(GList *source, const char *address);

struct adapter *find_parent(GList *source, GDBusProxy *device);

struct adapter *find_ctrl(GList *source, const char *path);

void show_adapter_list(GList *source);

gboolean device_is_child(GDBusProxy *device, GDBusProxy *master);

bool find_uuid(GDBusProxy *proxy, char *uuid);

#endif /* __APP_COMMON_H__ */

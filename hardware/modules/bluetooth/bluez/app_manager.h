/*****************************************************************************
**
**  Name:           app_manager.h
**
**  Description:    Bluetooth Manager application
**
**  Copyright (c) 2010-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef __APP_MANAGER_H__
#define __APP_MANAGER_H__

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <glib.h>
#include <wordexp.h>

#include <sys/time.h>
#include <hardware/bt_common.h>
#include <bluetooth/bluetooth.h>
#include "lib/mgmt.h"
#include "shared/mgmt.h"

#include "shared/util.h"
#include "gdbus/gdbus.h"
//#include "client/display.h"
#include "lib/uuid.h"
#include "client/agent.h"
#include "client/gatt.h"
#include "client/advertising.h"
#include "app_common.h"
#include "ble_server.h"
#include "a2dp_sink.h"
#include "a2dp.h"

#define VERSION "bluez-5.48"

DBusConnection *dbus_conn;

typedef int (*connect_fail_cb) (void *userdata, int status);

struct manager_proxy
{
    GDBusProxy *agent_manager;
    GDBusProxy *GattManager;
    struct adapter *default_ctrl;
    GDBusProxy *default_dev;
    GDBusProxy *default_attr;
};

struct manager_cxt_t
{
    bool init;
    bool powered;
    struct mgmt *mgmt;
    GList *ctrl_list;
    struct manager_proxy proxy;
    int disconnect_completed;

    void *caller;
    BTDevice default_device;
    BT_disc_device   discovery_devs[BT_DISC_NB_DEVICES];
    char bt_name[249];

    bool disc_start;
    void *disc_listener_handle;
    discovery_cb_t disc_listener;

    void *manage_listener_handle;
    manage_callbacks_t manage_listener;

    A2dpCtx         *a2dp_ctx;
    A2dpSink        *a2dp_sink;
    BleServer       *ble_ctx;
    //HFP_HS           *hs_ctx;

    pthread_mutex_t mutex;
};
typedef struct manager_cxt_t manager_cxt;

gboolean check_default_ctrl(manager_cxt *manager);


unsigned int send_cmd(struct mgmt *mgmt, uint16_t op, uint16_t id,
				uint16_t len, const void *param);
void set_default_attribute(manager_cxt *manager, GDBusProxy *proxy);
void show_devices(manager_cxt *manager);
int get_paired_devices(manager_cxt *manager, BTDevice *dev_array, int arr_len);

int set_remove_device(manager_cxt *manager, const char *addr);
int set_power(manager_cxt *manager, bool on);
int set_pairable(manager_cxt *manager, bool on);
int set_discoverable(manager_cxt *manager, bool on);
int set_scan(manager_cxt *manager, bool start);
int set_pair_device(manager_cxt *manager, const char *addr);
int set_trust_device(manager_cxt *manager, const char *addr, bool trusted);
int connect_addr(manager_cxt *manager, const char *addr);
int connect_addr_array(manager_cxt *manager, BTAddr addr);
int connect_profile_addr_array(manager_cxt *manager, BTAddr addr, const char *uuid, connect_fail_cb cb, void *userdata);

void disconnect_device(manager_cxt *manager, GDBusProxy *proxy);
int disconn_addr(manager_cxt *manager, const char *address);
int disconn_addr_array(manager_cxt *manager, BTAddr addr);

gboolean service_is_child(manager_cxt *manager, GDBusProxy *service);

int set_system_alias(manager_cxt *manager, const char *arg);


int app_get_module_addr(manager_cxt *manager, BTAddr addr);
manager_cxt *manager_create(void);
int manager_init(manager_cxt *manager);
int manager_deinit(manager_cxt *manager);

int manager_destroy(manager_cxt *manager);

int bt_set_visibility(manager_cxt *manager, bool discoverable, bool connectable);

int bt_set_name(manager_cxt *manager, const char *name);

#endif /* __APP_MANAGER_H__ */

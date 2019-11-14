#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "a2dp_sink.h"
#include "app_manager.h"

#define USE_PULSE_DB /**/

#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_MEDIA_CONTROL_INTERFACE "org.bluez.MediaControl1"
#define BLUEZ_MEDIA_FOLDER_INTERFACE "org.bluez.MediaFolder1"
#define BLUEZ_MEDIA_ITEM_INTERFACE "org.bluez.MediaItem1"
#define BLUEZ_MEDIA_TRANSPORT_INTERFACE "org.bluez.MediaTransport1"

#define BLUEZ_SINK_INPUT_STREAM_NAME "sink-input-by-media-name:playback"
//#define BLUEZ_SINK_INPUT_STREAM_NAME "sink-input-by-media-name:bluetooth"

static GDBusProxy *default_transport;
static GDBusProxy *default_player;
static GDBusProxy *default_device;
static GDBusProxy *default_control;
static GSList *players = NULL;
static GSList *folders = NULL;
static GSList *items = NULL;


static void state_cb(pa_context *context, void *ud)
{
    A2dpk_PulseAudio *pa = ud;

    switch(pa_context_get_state(context))
    {
        case PA_CONTEXT_READY:
            pa->state = PA_STATE_CONNECTED;
            break;
        case PA_CONTEXT_FAILED:
            pa->state = PA_STATE_ERROR;
            break;
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_TERMINATED:
            break;
    }
}

static void wait_for_pa_operation(A2dpk_PulseAudio *pa, pa_operation *op)
{
    int ret;

    if (!op) return;
    while (pa_operation_get_state(op) == PA_OPERATION_RUNNING) {
        if (pa_mainloop_iterate(pa->mainloop, 1, &ret) < 0) {
            BT_LOGE("### error: pa_mainloop_iterate failed %d\n", ret);
            break;
        }
    }
    pa_operation_unref(op);
}

#ifndef USE_PULSE_DB
static void sink_input_list_cb(pa_context * c, const pa_sink_input_info *i, int eol, void *raw)
{
    if (eol != 0) return;

    A2dpk_PulseAudio *pa = raw;
    BT_LOGD("sink input [%d] name: %s, driver: %s\n", i->index, i->name, i->driver);
    if (strncmp(i->name, "playback", 8) == 0 && strncmp(i->driver, "module-loopback.c", strlen("module-loopback.c")) == 0) {
        if(pa_proplist_contains( i->proplist, PA_PROP_MEDIA_ICON_NAME)) {
            const char *str1 = NULL;
            char *str2 = NULL;
            str1 = pa_proplist_gets(i->proplist, PA_PROP_MEDIA_ICON_NAME);
            BT_LOGI("found bt sink MEDIA ICON NAME: %s", str1);
            str2 = strstr(str1, "bluetooth");
            if (!str2) return;
            BT_LOGI("found bt sink input %d", i->index);
            pa->sink_input_index = i->index;
        }
    }
}

#else
static void sink_input_stream_read_list_cb(pa_context * c, const pa_ext_stream_restore_info *info, int eol, void *userdata)
{
    if (eol != 0) return;

    A2dpk_PulseAudio *pa = userdata;
    BT_LOGD("sink_input_stream mute %d,  name: %s", info->mute, info->name);
    if (strncmp(info->name, BLUEZ_SINK_INPUT_STREAM_NAME, strlen(BLUEZ_SINK_INPUT_STREAM_NAME)) == 0) {
        BT_LOGI("find bluez sink input stream info mute %d", info->mute);
        memcpy(&pa->info, info, sizeof(pa->info));
        pa->restore_info_flag = 1;
    }
}
#endif

static void pulse_find_bluez_sinkinput(A2dpk_PulseAudio *pa)
{
    pa_operation *op;
#ifndef USE_PULSE_DB
    pa->sink_input_index = -1;
    op = pa_context_get_sink_input_info_list(pa->context, sink_input_list_cb, pa);
    wait_for_pa_operation(pa, op);
#else
    pa->restore_info_flag = 0;
    op =  pa_ext_stream_restore_read(pa->context, sink_input_stream_read_list_cb, pa);
    wait_for_pa_operation(pa, op);
#endif
}


static void pa_ext_stream_success_cb(pa_context *c, int success, void *userdata)
{
    A2dpk_PulseAudio *pa = userdata;
    if (success) {
        BT_LOGI("success to set mute %d", pa->info.mute);
        pa->mute = pa->info.mute;
    }
}

static void set_bluez_sinkinput_mute(A2dpk_PulseAudio *pa, bool mute)
{
    if (!pa) return;

    pulse_find_bluez_sinkinput(pa);
#ifndef USE_PULSE_DB
    if (pa->sink_input_index >= 0) {
        pa_operation *op = pa_context_set_sink_input_mute(pa->context, pa->sink_input_index, mute, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pa->mute = mute;
    }
#else
    if (pa->restore_info_flag  == 1  && pa->info.mute != mute) {
        pa->info.mute = mute;
        pa_operation *op = pa_ext_stream_restore_write(pa->context, PA_UPDATE_REPLACE, &pa->info, 1, 1, pa_ext_stream_success_cb, pa);
        wait_for_pa_operation(pa, op);
    }
#endif
}

static A2dpk_PulseAudio *a2dpk_pulse_create(void)
{
    A2dpk_PulseAudio *pa = calloc(1, sizeof(*pa));

    pa->mainloop = pa_mainloop_new();
    pa->mainloop_api = pa_mainloop_get_api(pa->mainloop);

    pa->context = pa_context_new(pa->mainloop_api, "rokid-bt");
    pa_context_set_state_callback(pa->context, &state_cb, pa);

    pa->state = PA_STATE_CONNECTING;
    pa_context_connect(pa->context, NULL, PA_CONTEXT_NOFLAGS, NULL);
    while (pa->state == PA_STATE_CONNECTING) {
        pa_mainloop_iterate(pa->mainloop, 1, NULL);
    }

    assert(pa->state != PA_STATE_ERROR);
    if (pa->state == PA_STATE_ERROR) {
        abort();
    }

    return pa;
}

static void a2dpk_pulse_destroy(A2dpk_PulseAudio *pa)
{
    if (pa) {
        if (pa->state == PA_STATE_CONNECTED) {
            pa_context_disconnect(pa->context);
        }
        pa_context_unref(pa->context);
        pa_mainloop_free(pa->mainloop);

        free(pa);
    }
}


static GDBusProxy *find_item(const char *path)
{
    GSList *l;

    for (l = items; l; l = g_slist_next(l)) {
        GDBusProxy *proxy = l->data;

        if (strcmp(path, g_dbus_proxy_get_path(proxy)) == 0)
            return proxy;
    }

    return NULL;
}

static GDBusProxy *find_player(const char *path)
{
    GSList *l;

    for (l = players; l; l = g_slist_next(l)) {
        GDBusProxy *proxy = l->data;

        if (strcmp(path, g_dbus_proxy_get_path(proxy)) == 0)
            return proxy;
    }

    return NULL;
}

static int set_default_player(const char *path)
{
    GDBusProxy *proxy;

    if (!path) return -1;
    proxy = find_player(path);
    if (proxy == NULL)
        return -1;

    if (default_player == proxy)
        return 0;

    default_player = proxy,
    print_proxy(proxy, "Player",  COLORED_CHG, "[default]");
    return 0;
}

static bool check_default_control(void)
{
    if (!default_control) {
        BT_LOGI("No default control available\n");
        return FALSE;
    }

    return TRUE;
}

static bool check_default_player(void)
{
    if (!default_player) {
        BT_LOGI("No default player available\n");
        return FALSE;
    }

    return TRUE;
}

static void save_track_attrs(A2dpSink *as, DBusMessageIter *iter, int key)
{

    DBusMessageIter subiter;
    const char *valstr;
    dbus_uint32_t valu32;

    if (iter == NULL || as == NULL) {
        return;
    }

    if (as->elem_attr.num_attr >= BT_AVK_ELEMENT_ATTR_MAX) {
        BT_LOGE("too many data!!!");
        as->elem_attr.num_attr = BT_AVK_ELEMENT_ATTR_MAX;
        return;
    }

    switch (dbus_message_iter_get_arg_type(iter)) {
    case DBUS_TYPE_STRING:
    case DBUS_TYPE_OBJECT_PATH:
        dbus_message_iter_get_basic(iter, &valstr);
        if (key) {
            if (strcasecmp(valstr, "Title") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_TITLE;
            } else if (strcasecmp(valstr, "Album") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_ALBUM;
            } else if (strcasecmp(valstr, "TrackNumber") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_TRACK_NUM;
            } else if (strcasecmp(valstr, "Artist") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_ARTIST;
            } else if (strcasecmp(valstr, "NumberOfTracks") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_NUM_TRACKS;
            } else if (strcasecmp(valstr, "Duration") == 0) {
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = BT_AVRC_MEDIA_ATTR_ID_PLAYING_TIME;
            } else {
                BT_LOGW("known key %s", valstr);
                as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id = 0;//ignorge known key
            }
        } else {
            snprintf(as->elem_attr.attr_entry[as->elem_attr.num_attr].data, sizeof(as->elem_attr.attr_entry[as->elem_attr.num_attr].data), "%s", valstr);
            as->elem_attr.num_attr++;
        }
        break;
    case DBUS_TYPE_UINT32:
        dbus_message_iter_get_basic(iter, &valu32);
        if (as->elem_attr.attr_entry[as->elem_attr.num_attr].attr_id == BT_AVRC_MEDIA_ATTR_ID_TRACK_NUM) {
            valu32 += 1; // track num begin with 0, so we must plus 1
        }
        snprintf(as->elem_attr.attr_entry[as->elem_attr.num_attr].data, sizeof(as->elem_attr.attr_entry[as->elem_attr.num_attr].data), "%d", valu32);
        as->elem_attr.num_attr++;
        //BT_LOGV("%d", valu32);
        break;
    case DBUS_TYPE_VARIANT:
        dbus_message_iter_recurse(iter, &subiter);
        save_track_attrs(as, &subiter, key);
        break;
    case DBUS_TYPE_ARRAY:
        dbus_message_iter_recurse(iter, &subiter);
        while (dbus_message_iter_get_arg_type(&subiter) !=
							DBUS_TYPE_INVALID) {
            save_track_attrs(as, &subiter, 0);
            dbus_message_iter_next(&subiter);
        }
        break;
    case DBUS_TYPE_DICT_ENTRY:
        dbus_message_iter_recurse(iter, &subiter);
        save_track_attrs(as, &subiter, 1);
        dbus_message_iter_next(&subiter);
        save_track_attrs(as, &subiter, 0);
        break;
    default:
        BT_LOGE("dbus type %d error", dbus_message_iter_get_arg_type(iter));
        break;
    }

}

static void player_show(GDBusProxy *proxy)
{
    BT_LOGI("Player %s\n", g_dbus_proxy_get_path(proxy));

    print_property(proxy, "Name");
    print_property(proxy, "Repeat");
    print_property(proxy, "Equalizer");
    print_property(proxy, "Shuffle");
    print_property(proxy, "Scan");
    print_property(proxy, "Status");
    print_property(proxy, "Position");
    print_property(proxy, "Track");
}

static void folder_show(GDBusProxy *proxy)
{
    DBusMessageIter iter;
    const char *path;
    BT_LOGI("Folder %s\n", g_dbus_proxy_get_path(proxy));

    print_property(proxy, "Name");
    print_property(proxy, "NumberOfItems");

    if (!g_dbus_proxy_get_property(proxy, "Playlist", &iter))
        return;

    dbus_message_iter_get_basic(&iter, &path);
    BT_LOGI("Folder Playlist path %s\n", path);
}

static void item_show(GDBusProxy *proxy)
{

    BT_LOGI("Item %s\n", g_dbus_proxy_get_path(proxy));

    print_property(proxy, "Player");
    print_property(proxy, "Name");
    print_property(proxy, "Type");
    print_property(proxy, "FolderType");
    print_property(proxy, "Playable");
    print_property(proxy, "Metadata");
}

static void a2dpk_device_added(A2dpSink *as, GDBusProxy *proxy, void *user_data)
{
    DBusMessageIter iter;
    struct adapter *adapter = NULL;
    manager_cxt *manager = user_data;
    BTConnection *conn = &as->connections[0];

    if (!manager) {
        return;
    }

    if (conn->connected) return;//TODO: only support connect devices num 1
    adapter = find_parent(manager->ctrl_list, proxy);

    if (!adapter) {
        /* TODO: Error */
        return;
    }

    if (g_dbus_proxy_get_property(proxy, "Connected", &iter)) {
        BT_AVK_MSG msg = {0};
        dbus_bool_t connected;
        const char *address = NULL;
        const char *alias = NULL;

        dbus_message_iter_get_basic(&iter, &connected);

        if (connected) {

            if (g_dbus_proxy_get_property(proxy, "Address",
                                    &iter) == TRUE) {
                dbus_message_iter_get_basic(&iter, &address);
                BT_LOGD("addr %s", address);
                bd_strtoba(conn->bd_addr, address);
            }
            if (g_dbus_proxy_get_property(proxy, "Alias", &iter) == TRUE)
                dbus_message_iter_get_basic(&iter, &alias);
            else
                alias = "";
            snprintf((char *)conn->name, sizeof(conn->name), "%s", alias);

            if (as->listener) {
                    msg.sig_chnl_open.status = 0;
                    snprintf((char *)msg.sig_chnl_open.name, sizeof(msg.sig_chnl_open.name), "%s", alias);
                    msg.sig_chnl_open.idx = as->conn_index;
                    memcpy(msg.sig_chnl_open.bd_addr, conn->bd_addr, sizeof(msg.sig_chnl_open.bd_addr));
                    as->listener(as->listener_handle, BT_AVK_OPEN_EVT, &msg);
            }
            conn->connected = connected;
            if (default_device == NULL)
                default_device = proxy;
        }
    }
}

static void transport_added(A2dpSink *as, GDBusProxy *proxy)
{
    BT_AVK_MSG msg = {0};
    BTConnection *conn = &as->connections[0];

    if (default_transport == NULL) {
        default_transport = proxy;

        if ((as->listener) && (conn->connected)) {
            msg.stream_chnl_open.status = 0;
            msg.stream_chnl_open.idx = 0;
            snprintf((char *)msg.stream_chnl_open.name, sizeof(msg.stream_chnl_open.name), "%s", conn->name);
            memcpy(msg.stream_chnl_open.bd_addr, conn->bd_addr, sizeof(msg.stream_chnl_open.bd_addr));
            as->listener(as->listener_handle, BT_AVK_STR_OPEN_EVT, &msg);
        }
    }
}

static void player_added(A2dpSink *as, GDBusProxy *proxy)
{
    players = g_slist_append(players, proxy);

    if (default_player == NULL) {
        default_player = proxy;
    }
    print_proxy(proxy, "Player",  COLORED_NEW, default_player == proxy ? "[default]" : "");
    player_show(proxy);
}

static void folder_added(A2dpSink *as, GDBusProxy *proxy)
{
    folders = g_slist_append(folders, proxy);

    print_proxy(proxy, "Folder", COLORED_NEW, "");
    folder_show(proxy);
}

static void item_added(A2dpSink *as, GDBusProxy *proxy)
{
    const char *name;
    DBusMessageIter iter;

    items = g_slist_append(items, proxy);

    if (g_dbus_proxy_get_property(proxy, "Name", &iter))
        dbus_message_iter_get_basic(&iter, &name);
    else
        name = "<unknown>";
    print_proxy(proxy, "Item", COLORED_NEW, name);
    item_show(proxy);
}


static void transport_removed(A2dpSink *as, GDBusProxy *proxy)
{
    BT_AVK_MSG msg = {0};
    BTConnection *conn = &as->connections[0];

    if (default_transport == proxy) {
        default_transport = NULL;
        as->playing = false;
        as->start = false;
        if ((as->listener) && (conn->connected)) {
            msg.stop_streaming.status = 0;
            msg.stop_streaming.idx = 0;
            msg.stop_streaming.suspended = 0;
            memcpy(msg.stop_streaming.bd_addr, conn->bd_addr, sizeof(BTAddr));
            as->listener(as->listener_handle, BT_AVK_STOP_EVT, &msg);

            memset(&msg, 0, sizeof(msg));
            msg.stream_chnl_close.status = 0;
            msg.stream_chnl_close.idx = 0;
            snprintf((char *)msg.stream_chnl_close.name, sizeof(msg.stream_chnl_close.name), "%s", conn->name);
            memcpy(msg.stream_chnl_close.bd_addr, conn->bd_addr, sizeof(msg.stream_chnl_close.bd_addr));
            as->listener(as->listener_handle, BT_AVK_STR_CLOSE_EVT, &msg);
        }
    }
}

static void player_removed(A2dpSink *as, GDBusProxy *proxy)
{

    print_proxy(proxy, "Player", COLORED_DEL, default_player == proxy ? "[default]" : "");
    if (default_player == proxy) {
        default_player = NULL;
    }
    players = g_slist_remove(players, proxy);
}

static void folder_removed(A2dpSink *as, GDBusProxy *proxy)
{
    folders = g_slist_remove(folders, proxy);

    print_proxy(proxy, "Folder", COLORED_DEL, "");
}

static void item_removed(A2dpSink *as, GDBusProxy *proxy)
{
    items = g_slist_remove(items, proxy);

    print_proxy(proxy, "Item", COLORED_DEL, "");
}


void a2dpk_proxy_added(A2dpSink *as, GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    if (!as) return;
    if (!as->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, DEVICE_INTERFACE))
        a2dpk_device_added(as, proxy, user_data);
    else if (!strcmp(interface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
        transport_added(as, proxy);
    }
    else if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE))
        player_added(as, proxy);
    else if (!strcmp(interface, BLUEZ_MEDIA_FOLDER_INTERFACE))
        folder_added(as, proxy);
    else if (!strcmp(interface, BLUEZ_MEDIA_ITEM_INTERFACE))
        item_added(as, proxy);
}

void a2dpk_proxy_removed(A2dpSink *as, GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    if (!as) return;
    if (!as->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
        transport_removed(as, proxy);
    }
    if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE))
        player_removed(as, proxy);
    if (!strcmp(interface, BLUEZ_MEDIA_FOLDER_INTERFACE))
        folder_removed(as, proxy);
    if (!strcmp(interface, BLUEZ_MEDIA_ITEM_INTERFACE))
        item_removed(as, proxy);
}

void a2dpk_property_changed(A2dpSink *as, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
    const char *interface;
    BT_AVK_MSG msg = {0};
    manager_cxt *manager = user_data;
    DBusMessageIter get_iter;

    if (!manager) return;
    if (!as) return;
    if (!as->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);
    BTConnection *conn = &as->connections[0];

    if (!strcmp(interface, DEVICE_INTERFACE)) {
        if (manager->proxy.default_ctrl && device_is_child(proxy,
                manager->proxy.default_ctrl->proxy) == TRUE) {
            if (strcmp(name, "Connected") == 0) {
                const char *address = NULL;
                const char *alias = NULL;

                if (g_dbus_proxy_get_property(proxy, "Address",
                                &get_iter) == TRUE) {
                    dbus_message_iter_get_basic(&get_iter, &address);
                    BT_LOGD("addr %s", address);
                    bd_strtoba(conn->bd_addr, address);
                }
                if (g_dbus_proxy_get_property(proxy, "Alias", &get_iter) == TRUE)
                    dbus_message_iter_get_basic(&get_iter, &alias);
                else
                    alias = "";
                snprintf((char *)conn->name, sizeof(conn->name), "%s", alias);

                dbus_bool_t connected;
                dbus_message_iter_get_basic(iter, &connected);

                /*TODO: only support connect devices num 1*/
                if (connected && default_device) {
                    BT_LOGW("already have connect device");
                    return;
                }
                if (!connected && proxy != default_device) {
                    BT_LOGW("not default device ignored this disconnected");
                    return;
                }
                if (connected) {
                    if (!find_uuid(proxy, A2DP_SOURCE_UUID)) {
                        //BT_LOGW("remote device is not a2dp source!!!ignored and disconnect ");
                        //disconnect_device(manager, proxy);
                        //return;
                    }
                }
                if (conn->connected != connected) {
                    if (as->listener) {
                        msg.sig_chnl_open.status = 0;
                        snprintf((char *)msg.sig_chnl_open.name, sizeof(msg.sig_chnl_open.name), "%s", alias);
                        msg.sig_chnl_open.idx = as->conn_index;
                        memcpy(msg.sig_chnl_open.bd_addr, conn->bd_addr, sizeof(msg.sig_chnl_open.bd_addr));
                        as->listener(as->listener_handle, connected ? BT_AVK_OPEN_EVT : BT_AVK_CLOSE_EVT, &msg);
                    }
                    conn->connected = connected;
                    if (connected && default_device == NULL)
                        default_device = proxy;
                    if (!connected && default_device == proxy)
                        default_device = NULL;
                }
            }
        }
    }
    else if (!strcmp(interface, BLUEZ_MEDIA_PLAYER_INTERFACE) && default_player == proxy) {
        if (strcmp(name, "Status") == 0) {
            const char *status = NULL;
            dbus_message_iter_get_basic(iter, &status);
            if (strcmp(status, "playing") == 0) {
                as->playing = true;
                msg.reg_notif.rsp.param.play_status = BT_AVRC_PLAYSTATE_PLAYING;
            } else if (strcmp(status, "stopped") == 0) {
                as->playing = false;
                msg.reg_notif.rsp.param.play_status = BT_AVRC_PLAYSTATE_STOPPED;
            } else {
                as->playing = false;
                msg.reg_notif.rsp.param.play_status = BT_AVRC_PLAYSTATE_PAUSED;
            }
            if ((as->listener) && (conn->connected)) {
                msg.reg_notif.rsp.event_id = BT_AVRC_EVT_PLAY_STATUS_CHANGE;
                as->listener(as->listener_handle, BT_AVK_REGISTER_NOTIFICATION_EVT, &msg);
            }
        }
        if (strcmp(name, "Track") == 0) {
            pthread_mutex_lock(&as->mutex);
            as->elem_attr.num_attr = 0;
            save_track_attrs(as, iter, 0);
            pthread_mutex_unlock(&as->mutex);
            BT_LOGI("att num %d", as->elem_attr.num_attr);
            if (as->listener  && (conn->connected)) {
                msg.elem_attr = as->elem_attr;
                as->listener(as->listener_handle, BT_AVK_GET_ELEM_ATTR_EVT, &msg);
            }
        } else
            print_property_changed(proxy, "Player", name, iter);
    }
    else if (!strcmp(interface, BLUEZ_MEDIA_CONTROL_INTERFACE)) {

        if (strcmp(name, "Player") == 0) {
            const char *player = NULL;
            dbus_message_iter_get_basic(iter, &player);
            set_default_player(player);/* we want use latest palyer*/
        }
            if (strcmp(name, "Connected") == 0) {
                dbus_bool_t connected;
                dbus_message_iter_get_basic(iter, &connected);
                if (connected && default_control) return;
                if (!connected && default_control != proxy) return;
                if (conn->rc_opened != connected) {
                    conn->rc_opened = connected;
                    if (connected)
                        default_control = proxy;
                    else
                        default_control = NULL;
                    if(as->listener) {
                        msg.rc_open.status = 0;
                        memcpy(msg.rc_open.bd_addr, conn->bd_addr, sizeof(BTAddr));
                        msg.rc_open.idx = 0;
                        as->listener(as->listener_handle, connected ? BT_AVK_RC_OPEN_EVT : BT_AVK_RC_CLOSE_EVT, &msg);
                    }
                }
            }

        print_property_changed(proxy, "Control", name, iter);
    }
    else if (!strcmp(interface, BLUEZ_MEDIA_FOLDER_INTERFACE))
        print_property_changed(proxy, "Folder", name, iter);
    else if (!strcmp(interface, BLUEZ_MEDIA_ITEM_INTERFACE))
        print_property_changed(proxy, "Item", name, iter);
    else if (!strcmp(interface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
        if (strcmp(name, "State") == 0) {
            const char *status = NULL;
            dbus_message_iter_get_basic(iter, &status);
            if (strcmp(status, "active") == 0) {
                as->playing = true;
                as->start = true;
                if ((as->listener) && (conn->connected)) {
                    msg.start_streaming.status = 0;
                    msg.start_streaming.idx = 0;
                    memcpy(msg.start_streaming.bd_addr, conn->bd_addr, sizeof(msg.start_streaming.bd_addr));
                    as->listener(as->listener_handle, BT_AVK_START_EVT, &msg);
                }
            } else if (strcmp(status, "idle") == 0) {
                as->playing = false;
                as->start = false;
                if ((as->listener) && (conn->connected)) {
                    msg.stop_streaming.status = 0;
                    msg.stop_streaming.idx = 0;
                    msg.stop_streaming.suspended = true;
                    memcpy(msg.stop_streaming.bd_addr, conn->bd_addr, sizeof(BTAddr));
                    as->listener(as->listener_handle, BT_AVK_STOP_EVT, &msg);
                }
            }
        } else if (strcmp(name, "Volume") == 0) {
            dbus_uint16_t volume;
            dbus_message_iter_get_basic(iter, &volume);
            if ((as->listener) && (conn->connected)) {
                //msg.abs_volume.opcode = opcode;
                msg.abs_volume.volume = volume;
                as->listener(as->listener_handle,  BT_AVK_SET_ABS_VOL_CMD_EVT, &msg);
            }
        }
        print_property_changed(proxy, "Transport", name, iter);
    }
}

static void add_to_nowplaying_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGE("Failed to queue: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }

    BT_LOGI("AddToNowPlaying successful\n");
}

int set_queue(A2dpSink *as, const char *path)
{
    GDBusProxy *proxy;

    proxy = find_item(path);
    if (proxy == NULL) {
        BT_LOGE("Item %s not available", path);
        return -1;
    }

    if (g_dbus_proxy_method_call(proxy, "AddtoNowPlaying", NULL,
					add_to_nowplaying_reply, NULL,
					NULL) == FALSE) {
        BT_LOGE("Failed to play");
        return -1;
    }

    BT_LOGI("Attempting to queue %s", path);
    return 0;
}

static void send_cmd_reply(DBusMessage *message, void *user_data)
{
    DBusError error;

    dbus_error_init(&error);

    if (dbus_set_error_from_message(&error, message) == TRUE) {
        BT_LOGE("Failed to send cmd: %s\n", error.name);
        dbus_error_free(&error);
        return;
    }
    BT_LOGI("send cmd successful\n");
}

static void abvol_callback(const DBusError *error, void *user_data)
{
    if (dbus_error_is_set(error))
        BT_LOGE("Failed to set  %s\n", error->name);
    else {
        BT_LOGI("succeeded\n");
    }
}

int a2dpk_set_abs_vol(A2dpSink *as, uint8_t vol)
{
    int ret = -BT_STATUS_FAIL;
    dbus_uint16_t volume = vol;
    if (!default_transport)
        return -1;

    if (vol > A2DP_MAX_ABS_VOLUME) return -1;
    if (g_dbus_proxy_set_property_basic(default_transport, "Volume",
                    DBUS_TYPE_UINT16, &volume, abvol_callback, NULL, NULL) == TRUE)
        return BT_STATUS_SUCCESS;

    return ret;
}

int a2dpk_rc_send_cmd(A2dpSink *as, uint8_t command)
{
    const char *cmd = NULL;
    if (!check_default_control())
        return -1;

    switch (command) {
        case BT_AVRC_PLAY:
            cmd = "Play";
            break;
        case BT_AVRC_PAUSE:
            cmd = "Pause";
            break;
        case BT_AVRC_STOP:
            cmd = "Stop";
            break;
        case BT_AVRC_FORWARD:
            cmd = "Next";
            break;
        case BT_AVRC_BACKWARD:
            cmd = "Previous";
            break;
        case BT_AVRC_FAST_FOR:
            cmd = "FastForward";
            break;
        case BT_AVRC_REWIND:
            cmd = "Rewind";
            break;
        case BT_AVRC_VOL_UP:
            cmd = "VolumeUp";
            break;
        case BT_AVRC_VOL_DOWN:
            cmd = "VolumeDown";
            break;
        default:
            BT_LOGE("not support %d ", command);
            return -1;
            break;
    }
    if (g_dbus_proxy_method_call(default_control, cmd, NULL, send_cmd_reply,
							NULL, NULL) == FALSE) {
        BT_LOGE("Failed to send cmd %s ", cmd);
        return -1;
    }

    BT_LOGI("Attempting to send cmd %s", cmd);
    return 0;
}

#if 0
static int a2dpk_rc_send_getstatus_notify(A2dpSink *as, bool notify)
{
    const char *status = NULL;
    DBusMessageIter iter;
    BT_AVK_MSG msg = {0};

    if (!check_default_player())
        return -1;
    if (g_dbus_proxy_get_property(default_player, "Status", &iter) == FALSE)
        return -1;
    dbus_message_iter_get_basic(&iter, &status);
    BT_LOGD("status %s", status);
    if (strcmp(status, "playing") == 0) {
        as->playing = true;
        msg.get_play_status.play_status = 1;
    } else if (strcmp(status, "paused") == 0) {
        as->playing = false;
        msg.get_play_status.play_status = 2;
    } else {
        as->playing = false;
        msg.get_play_status.play_status = 0;
    }
    if (notify && (as->listener) && as->connections[as->conn_index].connected) {
            as->listener(as->listener_handle, BT_AVK_GET_PLAY_STATUS_EVT, &msg);
    }
    return 0;
}
#else
static int a2dpk_rc_send_getstatus_notify(A2dpSink *as, bool notify)
{
    BT_AVK_MSG msg = {0};

    if (as->playing)
        msg.get_play_status.play_status = BT_AVRC_PLAYSTATE_PLAYING;
    else if (!as->start)
        msg.get_play_status.play_status = BT_AVRC_PLAYSTATE_STOPPED;
    else
        msg.get_play_status.play_status = BT_AVRC_PLAYSTATE_PAUSED;

    if (notify && (as->listener) && as->connections[as->conn_index].connected) {
        as->listener(as->listener_handle, BT_AVK_GET_PLAY_STATUS_EVT, &msg);
    }
    return 0;
}
#endif

int a2dpk_rc_send_getstatus(A2dpSink *as)
{
    return a2dpk_rc_send_getstatus_notify(as, true);
}


static bool get_avail_connection(A2dpSink *as)
{
    int index;
    for (index = 0; index < MAX_CONNECTIONS; index++) {
        if (as->connections[index].connected == false) {
            return true;
        }
    }

    return false;
}

int a2dpk_connect_failed_cb(void *userdata, int status)
{
    A2dpSink *as = userdata;
    BT_CHECK_HANDLE(as);
    BT_AVK_MSG msg = {0};

    if (as->listener) {
        msg.sig_chnl_open.status = status;
        msg.sig_chnl_open.idx = as->conn_index;
        as->listener(as->listener_handle, BT_AVK_OPEN_EVT , &msg);
    }
    return 0;
}

int a2dpk_open(A2dpSink *as, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(as);
    manager_cxt *manager = as->caller;

    if (!as->enabled) return -1;
    if (get_avail_connection(as) == false) {
        BT_LOGW("Connect full!! If you really want to connect PLS disconnect devices before");
        return -1;
    }
    //return connect_addr_array(manager, bd_addr);
    return connect_profile_addr_array(manager, bd_addr, A2DP_SOURCE_UUID, a2dpk_connect_failed_cb, as);
}

int a2dpk_close_device(A2dpSink *as, BTAddr bd_addr)
{

    BT_CHECK_HANDLE(as);
    manager_cxt *manager = as->caller;
    return disconn_addr_array(manager, bd_addr);
}

int a2dpk_close(A2dpSink *as)
{
    int i;
    int ret = 0;

    BT_CHECK_HANDLE(as);

    for (i = 0; i < MAX_CONNECTIONS; i++) {
        if (as->connections[i].connected) {
            ret = a2dpk_close_device(as, as->connections[i].bd_addr);
        }
    }
    return ret;
}

int a2dpk_enable(A2dpSink *as)
{
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(as);
    manager = as->caller;

    if (!manager->init || !check_default_ctrl(manager))
        return -1;

    set_pairable(manager, true);
    if (as->enabled) return 0;
    if (!as->pa)
        as->pa = a2dpk_pulse_create();
    agent_default(dbus_conn, manager->proxy.agent_manager);
    as->enabled = true;
    usleep(30 * 1000);
    return 0;
}

int a2dpk_reset_stauts(A2dpSink *as)
{
    as->playing = 0;
    memset(as->connections, 0, sizeof(as->connections));
    as->conn_index = 0;
    return 0;
}

int a2dpk_disable(A2dpSink *as)
{
    int times =30;
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(as);
    manager = as->caller;

    if (as->enabled) {
        BTConnection *conn = &as->connections[as->conn_index];
        set_pairable(manager, false);
        if (conn->connected) {
            a2dpk_close(as);

            while (conn->connected && times--)
                usleep(100 * 1000);
        }
        a2dpk_reset_stauts(as);
        as->enabled = false;
    }
    pthread_mutex_lock(&as->mutex);
    a2dpk_pulse_destroy(as->pa);
    pthread_mutex_unlock(&as->mutex);
    as->pa = NULL;
    return 0;
}

A2dpSink *a2dpk_create(void *caller)
{
    A2dpSink *as = calloc(1, sizeof(*as));

    as->caller = caller;
    pthread_mutex_init(&as->mutex, NULL);

    return as;
}

int a2dpk_destroy(A2dpSink *as)
{
    if (as) {
        //a2dpk_disable(as);
        pthread_mutex_destroy(&as->mutex);
        free(as);
    }

    return 0;
}

int a2dpk_set_listener(A2dpSink *as, a2dp_sink_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(as);
    as->listener = listener;
    as->listener_handle = listener_handle;
    return 0;
}

int a2dpk_get_connected_devices(A2dpSink *as, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;

    BT_CHECK_HANDLE(as);
    if (!a2dpk_get_opened(as)) return 0;

    for (i = 0; i< MAX_CONNECTIONS; i++) {
        BTConnection *conn = as->connections + i;

        BT_LOGD("connection %d  connected %d, rc_opened %d",
                i, conn->connected, conn->rc_opened);
        if (conn->connected) {
            if (count < arr_len && dev_array) {
                BTDevice *dev = dev_array + count;
                snprintf(dev->name, sizeof(dev->name), "%s", conn->name);
                memcpy(dev->bd_addr, conn->bd_addr, sizeof(BTAddr));
                count ++;
            }
        }
        if (count >= arr_len) break;
    }

    return count;
}

int a2dpk_get_enabled(A2dpSink *as)
{
    return as ? as->enabled : 0;
}

int a2dpk_get_opened(A2dpSink *as)
{
    return as ? as->connections[as->conn_index].connected: 0;
}

int a2dpk_get_playing(A2dpSink *as)
{
    //a2dpk_rc_send_getstatus_notify(as, false);
    return as? as->playing : 0;
}


int a2dpk_set_mute(A2dpSink *as, bool mute)
{
    BT_CHECK_HANDLE(as);
    pthread_mutex_lock(&as->mutex);
    set_bluez_sinkinput_mute(as->pa, mute);
    pthread_mutex_unlock(&as->mutex);
    return 0;
}

int a2dpk_get_element_attr_command(A2dpSink *as)
{
    BT_AVK_MSG msg = {0};
    BT_CHECK_HANDLE(as);
    BTConnection *conn = &as->connections[0];
    if (as->listener  && (conn->connected)) {
        pthread_mutex_lock(&as->mutex);
        msg.elem_attr = as->elem_attr;
        as->listener(as->listener_handle, BT_AVK_GET_ELEM_ATTR_EVT, &msg);
        pthread_mutex_unlock(&as->mutex);
    }
    return 0;
}


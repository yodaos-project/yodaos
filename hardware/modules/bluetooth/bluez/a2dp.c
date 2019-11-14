#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "a2dp.h"
#include "app_manager.h"

#define BLUEZ_MEDIA_PLAYER_INTERFACE "org.bluez.MediaPlayer1"
#define BLUEZ_MEDIA_CONTROL_INTERFACE "org.bluez.MediaControl1"
#define BLUEZ_MEDIA_FOLDER_INTERFACE "org.bluez.MediaFolder1"
#define BLUEZ_MEDIA_ITEM_INTERFACE "org.bluez.MediaItem1"
#define BLUEZ_MEDIA_TRANSPORT_INTERFACE "org.bluez.MediaTransport1"

static GDBusProxy *default_device;
static GDBusProxy *default_control;

static void state_cb(pa_context *context, void *ud)
{
    A2dp_PulseAudio *pa = ud;

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

static void wait_for_pa_operation(A2dp_PulseAudio *pa, pa_operation *op)
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

static void sink_list_cb(pa_context * c, const pa_sink_info *i, int eol, void *raw) {
    if (eol != 0) return;
    A2dp_PulseAudio *pa = raw;

    if (i->index == 0) {
        snprintf(pa->default_sink_name, sizeof(pa->default_sink_name), "%s", i->name);
        BT_LOGI("default sink name %s\n", i->name);
    }

    if (strncmp(i->name, "bluez_sink.", 11) == 0) {
        BT_LOGI("found remount bt sink %d, %s\n", i->index, i->name);
        snprintf(pa->pipe_sink_name, sizeof(pa->pipe_sink_name), "%s", i->name);
        pa->pipe_sink = i->index;
    }
}

static int pulse_find_fifo_sink(A2dp_PulseAudio *pa)
{
    pa->pipe_sink = -1;
    pa_operation* op = pa_context_get_sink_info_list(pa->context, &sink_list_cb, pa);
    wait_for_pa_operation(pa, op);

    return pa->pipe_sink;
}

static void sink_input_list_cb(pa_context * c, const pa_sink_input_info *i, int eol, void *raw) {
    if (eol != 0) return;

    A2dp_PulseAudio *pa = raw;

    if (pa->pipe_sink < 0)
    {
        BT_LOGW("### execption : no pipe sink exsit\n");
        return ;
    }

    if (pa->count_sink_inputs < 10) {
        pa->current_sink_inputs[pa->count_sink_inputs] = i->index;
        pa->count_sink_inputs += 1;
    } else {
        BT_LOGW("### out of range %s inputs %d\n", i->name, i->index);
    }
}

static void pulse_move_current_sinkinputs_to_sink(A2dp_PulseAudio *pa, int sink)
{
    int i;
    pa->used_sink = sink;
    pa->count_sink_inputs = 0;
    pa_operation *op = pa_context_get_sink_input_info_list(pa->context, sink_input_list_cb, pa);
    wait_for_pa_operation(pa, op);
    for (i = 0; i < pa->count_sink_inputs; ++i)
    {
        // move sink inputs to pipe sink
        BT_LOGI("move sink input [%d] to sink %d\n", pa->current_sink_inputs[i], pa->used_sink);
        pa_operation *op = pa_context_move_sink_input_by_index(pa->context, pa->current_sink_inputs[i], pa->used_sink, NULL, NULL);

        wait_for_pa_operation(pa, op);
    }
}

static int a2dp_pulse_start(A2dp_PulseAudio *pa)
{
    pa_operation *op;

    if (!pa) return -1;
    pulse_find_fifo_sink(pa);
    if (pa->pipe_sink >= 0)
    {
        op = pa_context_set_default_sink(pa->context, pa->pipe_sink_name, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pulse_move_current_sinkinputs_to_sink(pa, pa->pipe_sink);
        system("sync");
        usleep(10000);
    }

    return pa->pipe_sink >= 0 ? 0 : -1;
}

static int a2dp_pulse_stop(A2dp_PulseAudio *pa)
{
    pa_operation *op;

    if (!pa) return -1;
    if (pa->pipe_sink >= 0)
    {
        op = pa_context_set_default_sink(pa->context, pa->default_sink_name, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pulse_move_current_sinkinputs_to_sink(pa, 0);
        pa->pipe_sink = -1;
    }

    return 0;
}

static A2dp_PulseAudio *a2dp_pulse_create(void)
{
    A2dp_PulseAudio *pa = calloc(1, sizeof(*pa));
    pa->pipe_sink = -1;

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

static void a2dp_pulse_destroy(A2dp_PulseAudio *pa)
{
    if (pa) {
        a2dp_pulse_stop(pa);
        if (pa->state == PA_STATE_CONNECTED) {
            pa_context_disconnect(pa->context);
        }
        pa_context_unref(pa->context);
        pa_mainloop_free(pa->mainloop);

        free(pa);
    }
}

static void a2dp_device_added(A2dpCtx *ac, GDBusProxy *proxy, void *user_data)
{
    DBusMessageIter iter;
    struct adapter *adapter = NULL;
    manager_cxt *manager = user_data;
    A2dpConnection *conn = &ac->connections[0];

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
        BT_A2DP_MSG msg = {0};
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

            if (ac->listener) {
                    msg.open.status = 0;
                    snprintf((char *)msg.open.dev.name, sizeof(msg.open.dev.name), "%s", alias);
                    memcpy(msg.open.dev.bd_addr, conn->bd_addr, sizeof(msg.open.dev.bd_addr));
                    ac->listener(ac->listener_handle, BT_A2DP_OPEN_EVT, &msg);
            }
            conn->connected = connected;
            if (default_device == NULL)
                default_device = proxy;
        }
    }
}

void a2dp_proxy_added(A2dpCtx *ac, GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    if (!ac) return;
    if (!ac->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, DEVICE_INTERFACE)) {
        a2dp_device_added(ac, proxy, user_data);
    }
}

void a2dp_proxy_removed(A2dpCtx *ac, GDBusProxy *proxy, void *user_data)
{
    const char *interface;

    if (!ac) return;
    if (!ac->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
        ac->playing = BT_AVRC_PLAYSTATE_STOPPED;
        pthread_mutex_lock(&ac->mutex);
        a2dp_pulse_stop(ac->pa);
        pthread_mutex_unlock(&ac->mutex);
        if(ac->listener) {
            BT_A2DP_MSG msg = {0};
            msg.stop.status = 0;
            msg.stop.pause = 0;
            ac->listener(ac->listener_handle, BT_A2DP_STOP_EVT, &msg);
        }
    }
}

void a2dp_property_changed(A2dpCtx *ac, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data)
{
    const char *interface;
    BT_A2DP_MSG msg = {0};
    manager_cxt *manager = user_data;
    DBusMessageIter get_iter;

    if (!manager) return;
    if (!ac) return;
    if (!ac->enabled) return;
    interface = g_dbus_proxy_get_interface(proxy);
    A2dpConnection *conn = &ac->connections[0];

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
                    if (!find_uuid(proxy, A2DP_SINK_UUID)) {
                        //BT_LOGW("remote device is not a2dp sink!!!ignored and disconnect ");
                        //disconnect_device(manager, proxy);
                        //return;
                    }
                }
                if (conn->connected != connected) {
                    if (ac->listener) {
                        msg.open.status = 0;
                        snprintf((char *)msg.open.dev.name, sizeof(msg.open.dev.name), "%s", alias);
                        memcpy(msg.open.dev.bd_addr, conn->bd_addr, sizeof(msg.open.dev.bd_addr));
                        ac->listener(ac->listener_handle, connected ? BT_A2DP_OPEN_EVT : BT_A2DP_CLOSE_EVT, &msg);
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
    else if (!strcmp(interface, BLUEZ_MEDIA_CONTROL_INTERFACE)) {
        if (strcmp(name, "Connected") == 0) {
                dbus_bool_t connected;
                dbus_message_iter_get_basic(iter, &connected);
                if (connected && default_control) return;
                if (!connected && default_control != proxy) return;
                if (conn->rc_opened != connected) {
                    if(ac->listener) {
                        msg.rc_open.status = 0;
                        ac->listener(ac->listener_handle, connected ? BT_A2DP_RC_OPEN_EVT : BT_A2DP_RC_CLOSE_EVT, &msg);
                    }
                    conn->rc_opened = connected;
                    if (connected)
                        default_control = proxy;
                    else
                        default_control = NULL;
                }
        }
        print_property_changed(proxy, "Control", name, iter);
    }
    else if (!strcmp(interface, BLUEZ_MEDIA_TRANSPORT_INTERFACE)) {
        if (strcmp(name, "State") == 0) {
            const char *state = NULL;
            dbus_message_iter_get_basic(iter, &state);
            if (strcmp(state, "active") == 0) {
                ac->playing = BT_AVRC_PLAYSTATE_PLAYING;
                pthread_mutex_lock(&ac->mutex);
                a2dp_pulse_start(ac->pa);
                pthread_mutex_unlock(&ac->mutex);
                if(ac->listener) {
                    msg.start.status = 0;
                    ac->listener(ac->listener_handle, BT_A2DP_START_EVT, &msg);
                }
            }
            else if (strcmp(state, "idle") == 0) {
                ac->playing = BT_AVRC_PLAYSTATE_STOPPED;
                if(ac->listener) {
                    msg.stop.status = 0;
                    msg.stop.pause = 0;
                    ac->listener(ac->listener_handle, BT_A2DP_STOP_EVT, &msg);
                }
            } 
        }
        print_property_changed(proxy, "Transport", name, iter);
    }
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


int a2dp_send_rc_command(A2dpCtx *ac, uint8_t command)
{
    const char *cmd = NULL;

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
        case BT_AVRC_VOL_UP:
            cmd = "VolumeUp";
            break;
        case BT_AVRC_VOL_DOWN:
            cmd = "VolumeDown";
            break;
        case BT_AVRC_FAST_FOR:
            cmd = "FastForward";
            break;
        case BT_AVRC_REWIND:
            cmd = "Rewind";
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

static bool get_avail_connection(A2dpCtx *ac)
{
    int index;
    for (index = 0; index < A2DP_MAX_CONNECTIONS; index++) {
        if (ac->connections[index].connected == false) {
            return true;
        }
    }

    return false;
}

int a2dp_connect_failed_cb(void *userdata, int status)
{
    A2dpCtx *ac = userdata;
    BT_CHECK_HANDLE(ac);
    BT_A2DP_MSG msg = {0};

    if (ac->listener) {
        msg.open.status = status;
        ac->listener(ac->listener_handle, BT_A2DP_OPEN_EVT, &msg);
    }
    return 0;
}

int a2dp_open(A2dpCtx *ac, BTAddr bd_addr)
{
    BT_CHECK_HANDLE(ac);
    manager_cxt *manager = ac->caller;

    if (!ac->enabled) return -1;
    if (get_avail_connection(ac) == false) {
        BT_LOGW("Connect full!! If you really want to connect PLS disconnect devices before");
        return -1;
    }
    //return connect_addr_array(manager, bd_addr);
    return connect_profile_addr_array(manager, bd_addr, A2DP_SINK_UUID, a2dp_connect_failed_cb, ac);
}

int a2dp_close_device(A2dpCtx *ac, BTAddr bd_addr)
{

    BT_CHECK_HANDLE(ac);
    manager_cxt *manager = ac->caller;
    return disconn_addr_array(manager, bd_addr);
}

int a2dp_close(A2dpCtx *ac)
{
    int i;
    int ret = 0;

    BT_CHECK_HANDLE(ac);

    for (i = 0; i < A2DP_MAX_CONNECTIONS; i++) {
        if (ac->connections[i].connected) {
            ret = a2dp_close_device(ac, ac->connections[i].bd_addr);
        }
    }
    return ret;
}

int a2dp_enable(A2dpCtx *ac)
{
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(ac);
    manager = ac->caller;

    pthread_mutex_lock(&ac->mutex);
    if (!ac->pa) {
        ac->pa = a2dp_pulse_create();
    }
    pthread_mutex_unlock(&ac->mutex);
    if (ac->enabled) return 0;

    set_pairable(manager, true);
    agent_default(dbus_conn, manager->proxy.agent_manager);
    ac->enabled = true;
    usleep(30 * 1000);
    return 0;
}

int a2dp_reset_stauts(A2dpCtx *ac)
{
    ac->playing = 0;
    memset(ac->connections, 0, sizeof(ac->connections));
    ac->conn_index = 0;
    return 0;
}

int a2dp_disable(A2dpCtx *ac)
{
    int times =30;
    manager_cxt *manager = NULL;

    BT_CHECK_HANDLE(ac);
    manager = ac->caller;

    if (ac->enabled) {
        A2dpConnection *conn = &ac->connections[ac->conn_index];
        set_pairable(manager, false);
        if (conn->connected) {
            a2dp_close(ac);

            while (conn->connected && times--)
                usleep(100 * 1000);
        }
        a2dp_reset_stauts(ac);
        ac->enabled = false;
    }
    pthread_mutex_lock(&ac->mutex);
    a2dp_pulse_destroy(ac->pa);
    pthread_mutex_unlock(&ac->mutex);
    ac->pa = NULL;
    return 0;
}

A2dpCtx *a2dp_create(void *caller)
{
    A2dpCtx *ac = calloc(1, sizeof(*ac));

    ac->caller = caller;
    pthread_mutex_init(&ac->mutex, NULL);

    return ac;
}

int a2dp_destroy(A2dpCtx *ac)
{
    if (ac) {
        //a2dp_disable(ac);
        pthread_mutex_destroy(&ac->mutex);
        free(ac);
    }

    return 0;
}

int a2dp_set_listener(A2dpCtx *ac, a2dp_callbacks_t listener, void *listener_handle)
{
    BT_CHECK_HANDLE(ac);
    ac->listener = listener;
    ac->listener_handle = listener_handle;
    return 0;
}

int a2dp_get_connected_devices(A2dpCtx *ac, BTDevice *dev_array, int arr_len)
{
    int i, count = 0;

    BT_CHECK_HANDLE(ac);
    if (!a2dp_get_opened(ac)) return 0;

    for (i = 0; i< A2DP_MAX_CONNECTIONS; i++) {
        A2dpConnection *conn = ac->connections + i;

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

int a2dp_get_enabled(A2dpCtx *ac)
{
    return ac ? ac->enabled : 0;
}

int a2dp_get_opened(A2dpCtx *ac)
{
    return ac ? ac->connections[ac->conn_index].connected: 0;
}

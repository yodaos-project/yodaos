#ifndef __A2DP_H__
#define __A2DP_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <hardware/bt_common.h>
#include <pulse/pulseaudio.h>
#include "gdbus/gdbus.h"


#define A2DP_MAX_CONNECTIONS 1

enum {
    PA_STATE_CONNECTING,
    PA_STATE_CONNECTED,
    PA_STATE_ERROR,
};

typedef struct A2dp_PulseAudio
{
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    int state;

    int default_sink;       // default index
    char default_sink_name[128];
    char pipe_sink_name[128];
    int pipe_sink;       // sink index
    int used_sink;

    int current_sink_inputs[10];
    int count_sink_inputs;
} A2dp_PulseAudio;

typedef struct
{
    BTAddr bd_addr;
    char name[249];
    bool connected;
    bool rc_opened;
} A2dpConnection;

struct A2dpCtx_t
{
    void *caller;
    int conn_index;
    A2dpConnection connections[A2DP_MAX_CONNECTIONS];

    bool enabled;
    int playing;

    a2dp_callbacks_t listener;
    void *          listener_handle;
    A2dp_PulseAudio *pa;
    pthread_mutex_t mutex;
};

typedef struct A2dpCtx_t A2dpCtx;



void a2dp_proxy_added(A2dpCtx *ac, GDBusProxy *proxy, void *user_data);

void a2dp_proxy_removed(A2dpCtx *ac, GDBusProxy *proxy, void *user_data);

void a2dp_property_changed(A2dpCtx *ac, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data);


int a2dp_send_rc_command(A2dpCtx *ac, uint8_t command);

int a2dp_open(A2dpCtx *ac, BTAddr bd_addr);

int a2dp_close_device(A2dpCtx *ac, BTAddr bd_addr);

int a2dp_close(A2dpCtx *ac);


int a2dp_enable(A2dpCtx *ac);

int a2dp_disable(A2dpCtx *ac);

A2dpCtx *a2dp_create(void *caller);

int a2dp_destroy(A2dpCtx *ac);

int a2dp_set_listener(A2dpCtx *ac, a2dp_callbacks_t listener, void *listener_handle);
int a2dp_get_connected_devices(A2dpCtx *ac, BTDevice *dev_array, int arr_len);

int a2dp_get_enabled(A2dpCtx *ac);

int a2dp_get_opened(A2dpCtx *ac);


#ifdef __cplusplus
}
#endif
#endif

#ifndef _A2DP_SINK_H_
#define _A2DP_SINK_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <string.h>
#include <hardware/bt_common.h>
#include <pulse/pulseaudio.h>
#include <pulse/ext-stream-restore.h>
#include "gdbus/gdbus.h"


#define MAX_CONNECTIONS 1
#define A2DP_MIN_ABS_VOLUME        0x00   /* Min and max absolute vol */
#define A2DP_MAX_ABS_VOLUME        0x7F

typedef struct
{
    BTAddr bd_addr;
    char name[249];
    bool connected;
    bool rc_opened;
} BTConnection;

typedef struct A2dpk_PulseAudio
{
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    int sink_input_index;
    int state;
    pa_ext_stream_restore_info info;
    int restore_info_flag;
    bool mute;
} A2dpk_PulseAudio;

struct A2dpSink_t
{
    //FILE *fp;
    void *caller;
    //int volume;
    int conn_index;
    BTConnection connections[MAX_CONNECTIONS];

    bool enabled;
    bool start;
    bool playing;
    A2dpk_PulseAudio *pa;

    a2dp_sink_callbacks_t listener;
    void *          listener_handle;

    BT_AVK_GET_ELEMENT_ATTR_MSG       elem_attr;
    pthread_mutex_t mutex;
};

typedef struct A2dpSink_t A2dpSink;



void a2dpk_proxy_added(A2dpSink *as, GDBusProxy *proxy, void *user_data);

void a2dpk_proxy_removed(A2dpSink *as, GDBusProxy *proxy, void *user_data);

void a2dpk_property_changed(A2dpSink *as, GDBusProxy *proxy, const char *name,
					DBusMessageIter *iter, void *user_data);

int a2dpk_set_abs_vol(A2dpSink *as, uint8_t vol);

int a2dpk_rc_send_cmd(A2dpSink *as, uint8_t command);
int a2dpk_rc_send_getstatus(A2dpSink *as);

int a2dpk_open(A2dpSink *as, BTAddr bd_addr);

int a2dpk_close_device(A2dpSink *as, BTAddr bd_addr);

int a2dpk_close(A2dpSink *as);


int a2dpk_enable(A2dpSink *as);

int a2dpk_disable(A2dpSink *as);

A2dpSink *a2dpk_create(void *caller);

int a2dpk_destroy(A2dpSink *as);

int a2dpk_set_listener(A2dpSink *as, a2dp_sink_callbacks_t listener, void *listener_handle);
int a2dpk_get_connected_devices(A2dpSink *as, BTDevice *dev_array, int arr_len);


int a2dpk_set_mute(A2dpSink *as, bool mute);

int a2dpk_get_enabled(A2dpSink *as);

int a2dpk_get_opened(A2dpSink *as);

int a2dpk_get_playing(A2dpSink *as);

int a2dpk_get_element_attr_command(A2dpSink *as);

#ifdef __cplusplus
}
#endif
#endif

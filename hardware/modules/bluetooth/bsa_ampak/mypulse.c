#include "mypulse.h"

#include <pulse/pulseaudio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hardware/bt_log.h>

enum {
    PA_STATE_CONNECTING,
    PA_STATE_CONNECTED,
    PA_STATE_ERROR,
};

struct PulseAudio
{
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;

    int state;

    int default_sink;       // default index
    char default_sink_name[128];
    int pipe_sink;       // sink index
    int pipe_module_index; // module index

    int used_sink;

    int current_sink_inputs[SINK_INPUTS_MAX];
    int count_sink_inputs;
};

static void state_cb(pa_context *context, void *ud)
{
    PulseAudio *pa = ud;

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

PulseAudio *pulse_create()
{
    PulseAudio *pa = calloc(1, sizeof(*pa));
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

static void wait_for_pa_operation(PulseAudio *pa, pa_operation *op)
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
    PulseAudio *pa = raw;

    if (i->index == 0)
    {
        snprintf(pa->default_sink_name, sizeof(pa->default_sink_name), "%s", i->name);
        //BT_LOGI("default sink name %s\n", i->name);
    }

    if (strcmp(i->name, A2DP_FIFO_NAME) == 0) {
        BT_LOGI("found pipe sink %d\n", i->index);
        pa->pipe_sink = i->index;
    }
}

static void sink_input_list_cb(pa_context * c, const pa_sink_input_info *i, int eol, void *raw) {
    if (eol != 0) return;

    PulseAudio *pa = raw;

    if (pa->pipe_sink < 0)
    {
        BT_LOGW("### execption : no pipe sink exsit\n");
        return ;
    }

    if (pa->count_sink_inputs < SINK_INPUTS_MAX) {
        pa->current_sink_inputs[pa->count_sink_inputs] = i->index;
        pa->count_sink_inputs += 1;
    } else {
        BT_LOGW("### out of range %s inputs %d\n", i->name, i->index);
    }
}

static int pulse_find_fifo_sink(PulseAudio *pa)
{
    pa->pipe_sink = -1;
    pa_operation* op = pa_context_get_sink_info_list(pa->context, &sink_list_cb, pa);
    wait_for_pa_operation(pa, op);

    return pa->pipe_sink;
}

static void pulse_move_current_sinkinputs_to_sink(PulseAudio *pa, int sink)
{
    int i;

    if (pa->used_sink == sink) return;
    pa->count_sink_inputs = 0;
    pa_operation *op = pa_context_get_sink_input_info_list(pa->context, sink_input_list_cb, pa);
    wait_for_pa_operation(pa, op);
    for (i = 0; i < pa->count_sink_inputs; ++i)
    {
        // move sink inputs to pipe sink
        BT_LOGI("move sink input [%d] to sink %d\n", pa->current_sink_inputs[i], sink);
        pa_operation *op = pa_context_move_sink_input_by_index(pa->context, pa->current_sink_inputs[i], sink, NULL, NULL);
        wait_for_pa_operation(pa, op);
    }
    pa->used_sink = sink;
}

static void module_index_cb(pa_context *c, uint32_t idx, void *userdata)
{
    PulseAudio *pa = userdata;
    pa->pipe_module_index = idx;
    BT_LOGI("module index %d\n", idx);
}

static void find_module_info_callback(pa_context *c, const pa_module_info *i, int is_last, void *userdata) {
    PulseAudio *pa = userdata;

    if (is_last < 0) {
        BT_LOGE("Failed to get module information: %s"), pa_strerror(pa_context_errno(c));
        return;
    }
    if (is_last) return;
    if (!i) return;
    if (!i->argument) return;

    if (strcmp(i->name, "module-pipe-sink") == 0) {
        char cmd_arg[256];
        snprintf(cmd_arg, sizeof(cmd_arg), "sink_name=%s file=/tmp/audio rate=%d channels=2", A2DP_FIFO_NAME, A2DP_SIMPLE_RATE);
        if (strncmp(i->argument, cmd_arg, 39) == 0) {
            pa->pipe_module_index = i->index;
            BT_LOGI("module index %d\n", pa->pipe_module_index);
        }
    }
}

int pulse_pipe_start(PulseAudio *pa)
{
    pa_operation *op;

    if (!pa) return -1;
    if (pa->pipe_sink >= 0) {
        op = pa_context_set_default_sink(pa->context, A2DP_FIFO_NAME, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pulse_move_current_sinkinputs_to_sink(pa, pa->pipe_sink);
        system("sync");
        usleep(10000);
    }

    return pa->pipe_sink >= 0 ? 0 : -1;
}

int pulse_pipe_init(PulseAudio *pa)
{
    int ret;
    pa_operation *op;

    if (!pa) return -1;
    if (access("/tmp/audio", F_OK) == -1)
    {
        char cmd_arg[256];
        snprintf(cmd_arg, sizeof(cmd_arg), "sink_name=%s file=/tmp/audio rate=%d channels=2", A2DP_FIFO_NAME, A2DP_SIMPLE_RATE);
        op = pa_context_load_module(pa->context, "module-pipe-sink",
                cmd_arg, module_index_cb, pa);
        wait_for_pa_operation(pa, op);
        if (access("/tmp/audio", F_OK) == -1 || pa->pipe_module_index <= 0)
        {
            BT_LOGE("### load module failed %d\n", pa->pipe_module_index);
            exit(1);
            return -1;
        }
        ret = pulse_find_fifo_sink(pa);
        if (ret < 0) {
            BT_LOGE("find fifo sink failed\n");
            return ret;
        }
    } else if (pa->pipe_module_index <= 0) {
        op = pa_context_get_module_info_list(pa->context, find_module_info_callback, pa);
        wait_for_pa_operation(pa, op);
        if (pa->pipe_module_index <= 0) {
            BT_LOGE("find pipe module failed\n");
            return -1;
        }
        ret = pulse_find_fifo_sink(pa);
        if (ret < 0) {
            BT_LOGE("find fifo sink failed\n");
            return ret;
        }
    }

    ret = pulse_pipe_start(pa);

    return ret;
}

int pulse_pipe_stop(PulseAudio *pa)
{
    pa_operation *op;

    if (!pa) return -1;
    if (pa->pipe_sink >= 0) {
        op = pa_context_set_default_sink(pa->context, pa->default_sink_name, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pulse_move_current_sinkinputs_to_sink(pa, 0);
    }

    return 0;
}

int pulse_pipe_uninit(PulseAudio *pa)
{
    pa_operation *op;

    if (!pa) return -1;
    pulse_pipe_stop(pa);
    if (pa->pipe_sink >= 0) {
        op = pa_context_unload_module(pa->context, pa->pipe_module_index, NULL, NULL);
        wait_for_pa_operation(pa, op);
        pa->pipe_module_index = -1;
        pa->pipe_sink = -1;
    }

    return 0;
}

void pulse_destroy(PulseAudio *pa)
{
    if (pa) {
        pulse_pipe_uninit(pa);
        if (pa->state == PA_STATE_CONNECTED) {
            pa_context_disconnect(pa->context);
        }
        pa_context_unref(pa->context);
        pa_mainloop_free(pa->mainloop);
        free(pa);
    }
}

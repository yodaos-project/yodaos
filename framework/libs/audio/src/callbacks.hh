#pragma once

#include "pulseaudio.hh"
#include <pulse/pulseaudio.h>

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

void state_cb(pa_context *context, void *raw);
void sink_list_cb(pa_context *c, const pa_sink_info *i, int eol, void *raw);
void sink_input_list_cb(pa_context *c, const pa_sink_input_info *i, int eol, void *raw);
void source_list_cb(pa_context *c, const pa_source_info *i, int eol, void *raw);
void server_info_cb(pa_context *context, const pa_server_info *i, void *raw);
void success_cb(pa_context *context, int success, void *raw);

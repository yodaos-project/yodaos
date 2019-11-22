#ifndef IOTJS_MODULE_SIGNAL_H
#define IOTJS_MODULE_SIGNAL_H

#include "iotjs_binding.h"
#include "iotjs_handlewrap.h"

typedef struct {
  iotjs_handlewrap_t handlewrap;
  uv_signal_t handle;
} IOTJS_VALIDATED_STRUCT(iotjs_signalwrap_t);

iotjs_signalwrap_t* iotjs_signalwrap_create(jerry_value_t jsignal);
iotjs_signalwrap_t* iotjs_signalwrap_from_handle(uv_signal_t* handle);
iotjs_signalwrap_t* iotjs_signalwrap_from_jobject(jerry_value_t jsignal);

uv_signal_t* iotjs_signalwrap_signal_handle(iotjs_signalwrap_t* signalwrap);
jerry_value_t iotjs_signalwrap_jobject(iotjs_signalwrap_t* signalwrap);
void iotjs_signalwrap_onsignal(uv_signal_t* handle, int signum);


#endif /* IOTJS_MODULE_SIGNAL_H */

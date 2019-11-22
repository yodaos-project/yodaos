#ifndef IOTJS_MODULE_PIPE_H
#define IOTJS_MODULE_PIPE_H

#include "iotjs_def.h"
#include "iotjs_handlewrap.h"

typedef struct {
  iotjs_handlewrap_t handlewrap;
  uv_pipe_t handle;
} IOTJS_VALIDATED_STRUCT(iotjs_pipewrap_t);

iotjs_pipewrap_t* iotjs_pipewrap_from_handle(uv_pipe_t* pipe);
iotjs_pipewrap_t* iotjs_pipewrap_from_jobject(jerry_value_t jval);
uv_pipe_t* iotjs_pipewrap_get_handle(iotjs_pipewrap_t* wrap);
jerry_value_t iotjs_pipewrap_get_jobject(iotjs_pipewrap_t* wrap);

#endif /* IOTJS_MODULE_PIPE_H */

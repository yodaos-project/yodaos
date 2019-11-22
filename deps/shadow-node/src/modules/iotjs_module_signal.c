#include "iotjs_def.h"
#include "iotjs_module_signal.h"
#include "iotjs_handlewrap.h"
#include "iotjs_module_buffer.h"

IOTJS_DEFINE_NATIVE_HANDLE_INFO_THIS_MODULE(signalwrap);

iotjs_signalwrap_t* iotjs_signalwrap_create(jerry_value_t jsignal) {
  iotjs_signalwrap_t* signalwrap = IOTJS_ALLOC(iotjs_signalwrap_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_signalwrap_t, signalwrap);

  iotjs_handlewrap_initialize(&_this->handlewrap, jsignal,
                              (uv_handle_t*)(&_this->handle),
                              &this_module_native_info);

  const iotjs_environment_t* env = iotjs_environment_get();
  uv_signal_init(iotjs_environment_loop(env), &_this->handle);
  return signalwrap;
}

static void iotjs_signalwrap_destroy(iotjs_signalwrap_t* signalwrap) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_signalwrap_t, signalwrap);
  iotjs_handlewrap_destroy(&_this->handlewrap);
  IOTJS_RELEASE(signalwrap);
}

iotjs_signalwrap_t* iotjs_signalwrap_from_handle(uv_signal_t* signal_handle) {
  uv_handle_t* handle = (uv_handle_t*)(signal_handle);
  iotjs_handlewrap_t* handlewrap = iotjs_handlewrap_from_handle(handle);
  iotjs_signalwrap_t* signalwrap = (iotjs_signalwrap_t*)handlewrap;
  IOTJS_ASSERT(iotjs_signalwrap_signal_handle(signalwrap) == signal_handle);
  return signalwrap;
}

iotjs_signalwrap_t* iotjs_signalwrap_from_jobject(jerry_value_t jsignal) {
  iotjs_handlewrap_t* handlewrap = iotjs_handlewrap_from_jobject(jsignal);
  return (iotjs_signalwrap_t*)handlewrap;
}

uv_signal_t* iotjs_signalwrap_signal_handle(iotjs_signalwrap_t* signalwrap) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_signalwrap_t, signalwrap);
  uv_handle_t* handle = iotjs_handlewrap_get_uv_handle(&_this->handlewrap);
  return (uv_signal_t*)handle;
}

jerry_value_t iotjs_signalwrap_jobject(iotjs_signalwrap_t* signalwrap) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_signalwrap_t, signalwrap);
  return iotjs_handlewrap_jobject(&_this->handlewrap);
}

void iotjs_signalwrap_onsignal(uv_signal_t* handle, int signum) {
  iotjs_signalwrap_t* signalwrap = iotjs_signalwrap_from_handle(handle);
  jerry_value_t jthis = iotjs_signalwrap_jobject(signalwrap);
  IOTJS_ASSERT(signalwrap != NULL);

  iotjs_jargs_t jargs = iotjs_jargs_create(1);
  jerry_value_t signal = jerry_create_number(signum);
  iotjs_jargs_append_jval(&jargs, signal);

  jerry_value_t fn = iotjs_jval_get_property(jthis, "onsignal");
  if (jerry_value_is_function(fn)) {
    iotjs_make_callback(fn, jthis, &jargs);
  }

  iotjs_jargs_destroy(&jargs);
  jerry_release_value(signal);
  jerry_release_value(fn);
}

JS_FUNCTION(CreateSignal) {
  DJS_CHECK_THIS();

  jerry_value_t jsignal = JS_GET_THIS();
  iotjs_signalwrap_create(jsignal);
  return jerry_create_undefined();
}

JS_FUNCTION(Start) {
  JS_DECLARE_THIS_PTR(signalwrap, signalwrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_signalwrap_t, signalwrap);
  int signum = JS_GET_ARG(0, number);
  int err = uv_signal_start(&_this->handle, iotjs_signalwrap_onsignal, signum);
  return jerry_create_number(err);
}

JS_FUNCTION(Stop) {
  JS_DECLARE_THIS_PTR(signalwrap, signalwrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_signalwrap_t, signalwrap);
  int err = uv_signal_stop(&_this->handle);
  return jerry_create_number(err);
}

jerry_value_t InitSignal() {
  jerry_value_t signal = jerry_create_external_function(CreateSignal);
  jerry_value_t prototype = jerry_create_object();

  iotjs_jval_set_property_jval(signal, IOTJS_MAGIC_STRING_PROTOTYPE, prototype);
  iotjs_jval_set_method(prototype, IOTJS_MAGIC_STRING_START, Start);
  iotjs_jval_set_method(prototype, IOTJS_MAGIC_STRING_STOP, Stop);
  jerry_release_value(prototype);
  return signal;
}

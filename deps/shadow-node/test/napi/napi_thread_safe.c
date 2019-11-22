#include <uv.h>
#include <node_api.h>
#include <stdlib.h>
#include "common.h"

static uv_thread_t pt;
static napi_env global_env;

void* thread_work(void* data) {
  napi_value callback = (napi_value)data;
  napi_call_function(global_env, NULL, callback, 0, NULL, NULL);

  return NULL;
}

napi_value SayHelloFromOtherThread(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  // test if `napi_get_cb_info` tolerants NULL pointers.
  NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
  global_env = env;

  uv_thread_create(&pt, (uv_thread_cb)thread_work, argv);

  napi_value nval_ret;
  NAPI_CALL(env, napi_get_undefined(env, &nval_ret));

  return nval_ret;
}

napi_value Init(napi_env env, napi_value exports) {
  SET_NAMED_METHOD(env, exports, "sayHelloFromOtherThread",
                   SayHelloFromOtherThread);

  return exports;
}

NAPI_MODULE(napi_test, Init);

#include <node_api.h>
#include "common.h"

static napi_value Test(napi_env env, napi_callback_info info) {
  napi_fatal_error("test_fatal::Test", NAPI_AUTO_LENGTH, "fatal message",
                   NAPI_AUTO_LENGTH);
  return NULL;
}

static napi_value TestStringLength(napi_env env, napi_callback_info info) {
  napi_fatal_error("test_fatal::TestStringLength", 16, "fatal message", 13);
  return NULL;
}

static napi_value Init(napi_env env, napi_value exports) {
  SET_NAMED_METHOD(env, exports, "Test", Test);
  SET_NAMED_METHOD(env, exports, "TestStringLength", TestStringLength);

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

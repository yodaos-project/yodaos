#include <node_api.h>
#include "common.h"

static napi_value ThrowError(napi_env env, napi_callback_info info) {
  NAPI_CALL(env, napi_throw_error(env, NULL, "foobar"));

  return NULL;
}

static napi_value ThrowCreatedError(napi_env env, napi_callback_info info) {
  napi_value error, code, message;
  NAPI_CALL(env,
            napi_create_string_utf8(env, "foobar", NAPI_AUTO_LENGTH, &message));
  NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &code));
  NAPI_CALL(env, napi_create_error(env, code, message, &error));
  NAPI_CALL(env, napi_throw(env, error));

  return NULL;
}

static napi_value RethrowError(napi_env env, napi_callback_info info) {
  NAPI_CALL(env, napi_throw_error(env, NULL, "foobar"));

  napi_value err;
  NAPI_CALL(env, napi_get_and_clear_last_exception(env, &err));

  NAPI_CALL(env, napi_throw(env, err));

  return NULL;
}

static napi_value GetError(napi_env env, napi_callback_info info) {
  napi_value error, code, message;
  NAPI_CALL(env,
            napi_create_string_utf8(env, "foobar", NAPI_AUTO_LENGTH, &message));
  NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &code));
  NAPI_CALL(env, napi_create_error(env, code, message, &error));

  return error;
}

static napi_value GetNoCodeError(napi_env env, napi_callback_info info) {
  napi_value error, message;
  NAPI_CALL(env,
            napi_create_string_utf8(env, "foobar", NAPI_AUTO_LENGTH, &message));
  NAPI_CALL(env, napi_create_error(env, NULL, message, &error));

  return error;
}

static napi_value Init(napi_env env, napi_value exports) {
  napi_value message;
  NAPI_CALL(env,
            napi_create_string_utf8(env, "foobar", NAPI_AUTO_LENGTH, &message));
  napi_value error;
  NAPI_CALL(env, napi_create_error(env, message, message, &error));

  SET_NAMED_METHOD(env, error, "ThrowError", ThrowError);
  SET_NAMED_METHOD(env, error, "ThrowCreatedError", ThrowCreatedError);
  SET_NAMED_METHOD(env, error, "RethrowError", RethrowError);
  SET_NAMED_METHOD(env, error, "GetError", GetError);
  SET_NAMED_METHOD(env, error, "GetNoCodeError", GetNoCodeError);

  NAPI_CALL(env, napi_throw(env, error));

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

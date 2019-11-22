/* Copyright 2018-present Rokid Co., Ltd. and other contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jerryscript-ext/handle-scope.h"
#include "jerryscript.h"
#include "internal/node_api_internal.h"

static void tsfn_async_close_cb(uv_handle_t* handle) {
  iotjs_threadsafe_function_t* tsfn =
      (iotjs_threadsafe_function_t*)handle->data;

  napi_env env = tsfn->env;
  napi_value func = tsfn->func;
  void* thread_finalize_data = tsfn->thread_finalize_data;
  napi_finalize thread_finalize_cb = tsfn->thread_finalize_cb;
  void* context = tsfn->context;
  napi_async_context async_context = tsfn->async_context;

  uv_cond_t* async_cond = &tsfn->async_cond;
  uv_mutex_t* op_mutex = &tsfn->op_mutex;

  thread_finalize_cb(env, thread_finalize_data, context);
  NAPI_ASSERT(tsfn->invocation_head == NULL,
              "TSFN invocation shall be cleared before closing async handle.");

  jerry_release_value(AS_JERRY_VALUE(func));
  napi_async_destroy(env, async_context);
  uv_cond_destroy(async_cond);
  uv_mutex_destroy(op_mutex);
  IOTJS_RELEASE(tsfn);
}

static void tsfn_async_callback(uv_async_t* handle) {
  iotjs_threadsafe_function_t* tsfn =
      (iotjs_threadsafe_function_t*)handle->data;
  uv_mutex_t* op_mutex = &tsfn->op_mutex;
  uv_cond_t* async_cond = &tsfn->async_cond;

  uv_mutex_lock(op_mutex);
  iotjs_tsfn_invocation_t* invocation = tsfn->invocation_head;

  tsfn->invocation_head = NULL;
  tsfn->invocation_tail = NULL;
  tsfn->queue_size = 0;

  napi_env env = tsfn->env;
  napi_value func = tsfn->func;
  void* context = tsfn->context;
  napi_threadsafe_function_call_js call_js_cb = tsfn->call_js_cb;
  uv_mutex_unlock(op_mutex);
  uv_cond_signal(async_cond);

  while (invocation != NULL) {
    jerryx_handle_scope scope;
    jerryx_open_handle_scope(&scope);
    if (call_js_cb != NULL) {
      call_js_cb(env, func, context, invocation->data);
    } else {
      napi_value nval_undefined = AS_NAPI_VALUE(jerry_create_undefined());
      napi_call_function(env, nval_undefined, func, 0, NULL, NULL);
    }
    jerryx_close_handle_scope(scope);

    if (iotjs_napi_is_exception_pending(env)) {
      jerry_value_t jval_err;
      jval_err = iotjs_napi_env_get_and_clear_exception(env);
      if (jval_err == (uintptr_t)NULL) {
        jval_err = iotjs_napi_env_get_and_clear_fatal_exception(env);
      }

      /** Argument cannot have error flag */
      jerry_value_clear_error_flag(&jval_err);
      iotjs_uncaught_exception(jval_err);
      jerry_release_value(jval_err);
    }

    iotjs_tsfn_invocation_t* tobereleased = invocation;
    invocation = invocation->next;
    IOTJS_RELEASE(tobereleased);
  }

  uv_mutex_lock(op_mutex);
  if ((tsfn->thread_count == 0 && tsfn->queue_size == 0) || tsfn->aborted) {
    uv_close((uv_handle_t*)&tsfn->async_handle, tsfn_async_close_cb);
  }
  uv_mutex_unlock(op_mutex);
}

napi_status napi_create_threadsafe_function(
    napi_env env, napi_value func, napi_value async_resource,
    napi_value async_resource_name, size_t max_queue_size,
    size_t initial_thread_count, void* thread_finalize_data,
    napi_finalize thread_finalize_cb, void* context,
    napi_threadsafe_function_call_js call_js_cb,
    napi_threadsafe_function* result) {
  NAPI_TRY_ENV(env);
  NAPI_ASSERT(result != NULL,
              "Expect an non-null out result pointer on "
              "napi_create_threadsafe_function.");

  iotjs_threadsafe_function_t* tsfn = IOTJS_ALLOC(iotjs_threadsafe_function_t);

  bool napi_async_inited = false, uv_async_inited = false,
       uv_cond_inited = false, uv_mutex_inited = false, func_acquired = false;

  napi_async_context async_context;
  NAPI_INTERNAL_CALL(napi_async_init(env, async_resource, async_resource_name,
                                     &async_context));
  napi_async_inited = true;

  uv_async_t* async_handle = &tsfn->async_handle;
  iotjs_environment_t* iotjs_env = iotjs_environment_get();
  uv_loop_t* iotjs_loop = iotjs_environment_loop(iotjs_env);
  int uv_status = uv_async_init(iotjs_loop, async_handle, tsfn_async_callback);
  if (uv_status != 0) {
    goto clean;
  }
  uv_async_inited = true;

  uv_cond_t* async_cond = &tsfn->async_cond;
  uv_status = uv_cond_init(async_cond);
  if (uv_status != 0) {
    goto clean;
  }
  uv_cond_inited = true;


  uv_mutex_t* op_mutex = &tsfn->op_mutex;
  uv_status = uv_mutex_init(op_mutex);
  if (uv_status != 0) {
    goto clean;
  }
  uv_mutex_inited = true;

  func = AS_NAPI_VALUE(jerry_acquire_value(AS_JERRY_VALUE(func)));
  func_acquired = true;

  tsfn->env = env;
  tsfn->func = func;
  tsfn->max_queue_size = max_queue_size;
  tsfn->thread_count = initial_thread_count;
  tsfn->thread_finalize_data = thread_finalize_data;
  tsfn->thread_finalize_cb = thread_finalize_cb;
  tsfn->context = context;
  tsfn->call_js_cb = call_js_cb;

  tsfn->async_context = async_context;

  tsfn->invocation_head = NULL;
  tsfn->invocation_tail = NULL;
  tsfn->queue_size = 0;

  tsfn->aborted = false;

  async_handle->data = (void*)tsfn;

  NAPI_ASSIGN(result, (napi_threadsafe_function)tsfn);
  NAPI_RETURN(napi_ok);

clean:
  if (napi_async_inited) {
    napi_async_destroy(env, async_context);
  }
  if (func_acquired) {
    jerry_release_value(AS_JERRY_VALUE(func));
  }
  if (uv_async_inited) {
    uv_close((uv_handle_t*)async_handle, tsfn_async_close_cb);
  }
  if (uv_cond_inited) {
    uv_cond_destroy(async_cond);
  }
  if (uv_mutex_inited) {
    uv_mutex_destroy(op_mutex);
  }
  IOTJS_RELEASE(tsfn);
  NAPI_RETURN_WITH_MSG(napi_generic_failure,
                       "Unexpected error on napi_create_threadsafe_function.");
}

napi_status napi_get_threadsafe_function_context(napi_threadsafe_function func,
                                                 void** result) {
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;
  NAPI_ASSIGN(result, tsfn->context);

  /** do not use NAPI_RETURN macro as it would access napi_env */
  return napi_ok;
}

napi_status napi_call_threadsafe_function(
    napi_threadsafe_function func, void* data,
    napi_threadsafe_function_call_mode is_blocking) {
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;

  uv_mutex_lock(&tsfn->op_mutex);
  napi_status ret_status = napi_ok;


  if (tsfn->max_queue_size != 0 && tsfn->queue_size == tsfn->max_queue_size) {
    switch (is_blocking) {
      case napi_tsfn_nonblocking:
        ret_status = napi_queue_full;
        goto clean;
      case napi_tsfn_blocking:
        uv_cond_wait(&tsfn->async_cond, &tsfn->op_mutex);
        break;
      default:
        NAPI_ASSERT(false,
                    "Unrecognized mode on napi_call_threadsafe_function.");
    }
  }

  if (tsfn->aborted) {
    ret_status = napi_closing;
    goto clean;
  }

  iotjs_tsfn_invocation_t* tsfn_invocation =
      IOTJS_ALLOC(iotjs_tsfn_invocation_t);
  tsfn_invocation->data = data;
  tsfn_invocation->next = NULL;

  if (tsfn->invocation_head == NULL) {
    tsfn->invocation_head = tsfn_invocation;
  }
  if (tsfn->invocation_tail == NULL) {
    tsfn->invocation_tail = tsfn_invocation;
  } else {
    tsfn->invocation_tail->next = tsfn_invocation;
    tsfn->invocation_tail = tsfn_invocation;
  }
  ++tsfn->queue_size;

  uv_async_send(&tsfn->async_handle);

clean:
  uv_mutex_unlock(&tsfn->op_mutex);
  return ret_status;
}

napi_status napi_acquire_threadsafe_function(napi_threadsafe_function func) {
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;
  uv_mutex_lock(&tsfn->op_mutex);

  ++tsfn->thread_count;

  uv_mutex_unlock(&tsfn->op_mutex);
  return napi_ok;
}

napi_status napi_release_threadsafe_function(
    napi_threadsafe_function func, napi_threadsafe_function_release_mode mode) {
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;
  uv_mutex_lock(&tsfn->op_mutex);

  if (tsfn->thread_count > 0) {
    --tsfn->thread_count;
  } else {
    fprintf(stderr,
            "[N-API] WARNING: trying to release an already zero-referenced "
            "threadsafe function.\n");
  }
  switch (mode) {
    case napi_tsfn_release:
      break;
    case napi_tsfn_abort:
      tsfn->aborted = true;
      break;
    default:
      NAPI_ASSERT(false,
                  "Unrecognized mode on napi_release_threadsafe_function.");
  }

  if ((tsfn->thread_count == 0 && tsfn->queue_size == 0) || tsfn->aborted) {
    /**
     * No uv methods are thread safe except `uv_async_send`.
     * Just notifying main thread that the tsfn is closing.
     */
    uv_async_send(&tsfn->async_handle);
  }

  uv_mutex_unlock(&tsfn->op_mutex);
  return napi_ok;
}

napi_status napi_unref_threadsafe_function(napi_env env,
                                           napi_threadsafe_function func) {
  NAPI_TRY_ENV(env);
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;

  uv_unref((uv_handle_t*)&tsfn->async_handle);

  NAPI_RETURN(napi_ok);
}

napi_status napi_ref_threadsafe_function(napi_env env,
                                         napi_threadsafe_function func) {
  NAPI_TRY_ENV(env);
  iotjs_threadsafe_function_t* tsfn = (iotjs_threadsafe_function_t*)func;

  uv_ref((uv_handle_t*)&tsfn->async_handle);

  NAPI_RETURN(napi_ok);
}

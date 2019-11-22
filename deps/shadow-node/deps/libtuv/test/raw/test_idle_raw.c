/* Copyright 2015 Samsung Electronics Co., Ltd.
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <uv.h>
#include "runner.h"


static uv_idle_t idle_handle;
static int idle_cb_called = 0;

void idle_callback(uv_idle_t* handle) {
  idle_cb_called++;
}


typedef struct {
  uv_loop_t* loop;
} idle_param_t;


static int idle_base_loop(void* vparam) {
  idle_param_t* param = (idle_param_t*)vparam;
  uv_run(param->loop, UV_RUN_ONCE);
  return 0; // stop the loop
}

static int idle_base_final(void* vparam) {
  idle_param_t* param = (idle_param_t*)vparam;

  uv_close((uv_handle_t*) &idle_handle, NULL);
  uv_run(param->loop, UV_RUN_ONCE);

  TUV_ASSERT(0 == uv_loop_close(param->loop));
  TUV_WARN(idle_cb_called == 1);

  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}

TEST_IMPL(idle_basic) {
  idle_param_t* param = (idle_param_t*)malloc(sizeof(idle_param_t));
  param->loop = uv_default_loop();

  idle_cb_called = 0;

  uv_idle_init(param->loop, &idle_handle);
  uv_idle_start(&idle_handle, idle_callback);

  tuv_run(param->loop, idle_base_loop, idle_base_final, param);

  return 0;
}

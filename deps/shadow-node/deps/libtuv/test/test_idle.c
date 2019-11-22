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
  TDDDLOG("idle_callback handle flag %p, %x, %d",
          handle, handle->flags, idle_cb_called);
  idle_cb_called++;
}


TEST_IMPL(idle_basic) {
  uv_loop_t* loop;

  idle_cb_called = 0;

  loop = uv_default_loop();
  uv_idle_init(loop, &idle_handle);
  uv_idle_start(&idle_handle, idle_callback);
  uv_run(loop, UV_RUN_ONCE);
  uv_close((uv_handle_t*) &idle_handle, NULL);
  uv_run(loop, UV_RUN_ONCE);

  TUV_ASSERT(0 == uv_loop_close(loop));

  TUV_WARN(idle_cb_called==1);

  return 0;
}

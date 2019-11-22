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

/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <uv.h>

#include "runner.h"


//-----------------------------------------------------------------------------

static unsigned int timer_run_once_timer_cb_called = 0;

static void timer_run_once_timer_cb(uv_timer_t* handle) {
  timer_run_once_timer_cb_called++;
}


typedef struct {
  uv_loop_t* loop;
  uv_timer_t timer_handle;
} timer_param_t;


TEST_IMPL(timer_run_once) {
  timer_param_t* param;
  uv_loop_t* loop;

  param = (timer_param_t*)malloc(sizeof(timer_param_t));
  loop = uv_default_loop();
  param->loop = loop;

  timer_run_once_timer_cb_called = 0;

  TUV_ASSERT(0 == uv_timer_init(param->loop, &param->timer_handle));
  TUV_ASSERT(0 == uv_timer_start(&param->timer_handle,
                                 timer_run_once_timer_cb, 0, 0));
  TUV_ASSERT(0 == uv_run(param->loop, UV_RUN_ONCE));
  TUV_ASSERT(1 == timer_run_once_timer_cb_called);

  TUV_ASSERT(0 == uv_timer_start(&param->timer_handle,
                                timer_run_once_timer_cb, 1, 0));
  // slow systems may have nano second resolution
  // give some time to sleep so that time tick is changed
  uv_sleep(1);
  TUV_ASSERT(0 == uv_run(param->loop, UV_RUN_ONCE));
  TUV_ASSERT(2 == timer_run_once_timer_cb_called);

  uv_close((uv_handle_t*)&param->timer_handle, NULL);
  TUV_ASSERT(0 == uv_run(param->loop, UV_RUN_ONCE));

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(param->loop));

  // cleanup tuv param
  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}

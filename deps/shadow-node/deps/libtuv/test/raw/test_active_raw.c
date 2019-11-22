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

#include <stdio.h>
#include <stdlib.h>

#include <uv.h>

#include "runner.h"


static int close_cb_called = 0;


static void close_cb(uv_handle_t* handle) {
  TUV_ASSERT(handle != NULL);
  close_cb_called++;
}


static void timer_cb(uv_timer_t* handle) {
  TUV_ASSERT(0 && "timer_cb should not have been called");
}


typedef struct {

  uv_loop_t* loop;
  uv_timer_t timer;

} active_param_t;


static int active_loop(void* vparam) {
  active_param_t* param = (active_param_t*)vparam;
  uv_loop_t* loop = param->loop;
  int r;

  r = uv_run(loop, UV_RUN_ONCE);

  return r; // continue while r == 1
}

static int active_final(void* vparam) {
  active_param_t* param = (active_param_t*)vparam;
  uv_loop_t* loop = param->loop;

  TUV_ASSERT(close_cb_called == 1);
  TUV_ASSERT(0 == uv_loop_close(loop));

  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}


TEST_IMPL(active) {
  uv_loop_t* loop;
  active_param_t* param;

  param = (active_param_t*)malloc(sizeof(active_param_t));
  loop = uv_default_loop();
  param->loop = loop;

  int r;

  close_cb_called = 0;

  r = uv_timer_init(loop, &param->timer);
  TUV_ASSERT(r == 0);

  /* uv_is_active() and uv_is_closing() should always return either 0 or 1. */
  TUV_ASSERT(0 == uv_is_active((uv_handle_t*) &param->timer));
  TUV_ASSERT(0 == uv_is_closing((uv_handle_t*) &param->timer));

  r = uv_timer_start(&param->timer, timer_cb, 1000, 0);
  TUV_ASSERT(r == 0);

  TUV_ASSERT(1 == uv_is_active((uv_handle_t*) &param->timer));
  TUV_ASSERT(0 == uv_is_closing((uv_handle_t*) &param->timer));

  r = uv_timer_stop(&param->timer);
  TUV_ASSERT(r == 0);

  TUV_ASSERT(0 == uv_is_active((uv_handle_t*) &param->timer));
  TUV_ASSERT(0 == uv_is_closing((uv_handle_t*) &param->timer));

  r = uv_timer_start(&param->timer, timer_cb, 1000, 0);
  TUV_ASSERT(r == 0);

  TUV_ASSERT(1 == uv_is_active((uv_handle_t*) &param->timer));
  TUV_ASSERT(0 == uv_is_closing((uv_handle_t*) &param->timer));

  uv_close((uv_handle_t*) &param->timer, close_cb);

  TUV_ASSERT(0 == uv_is_active((uv_handle_t*) &param->timer));
  TUV_ASSERT(1 == uv_is_closing((uv_handle_t*) &param->timer));

  tuv_run(loop, active_loop, active_final, param);

  return 0;
}

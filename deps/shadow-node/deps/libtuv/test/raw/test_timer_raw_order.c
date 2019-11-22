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

#include <string.h>
#include <unistd.h> // usleep
#include <uv.h>


#include "runner.h"



//-----------------------------------------------------------------------------

static int order_cb_called = 0;


static void order_cb_a(uv_timer_t *handle) {
  TUV_ASSERT(order_cb_called++ == *(int*)handle->data);
}


static void order_cb_b(uv_timer_t *handle) {
  TUV_ASSERT(order_cb_called++ == *(int*)handle->data);
}

//-----------------------------------------------------------------------------

typedef struct {
  uv_loop_t* loop;
  uv_timer_t handle_a;
  uv_timer_t handle_b;
  int first;
  int second;
} timer_param_t;


static int timer_loop(void* vparam) {
  timer_param_t* param = (timer_param_t*)vparam;
  return uv_run(param->loop, UV_RUN_ONCE);
}

static int timer_final1(void* vparam);
static int timer_final2(void* vparam);

static int timer_final1(void* vparam) {
  timer_param_t* param = (timer_param_t*)vparam;

  TUV_ASSERT(order_cb_called == 2);

  TUV_ASSERT(0 == uv_timer_stop(&param->handle_a));
  TUV_ASSERT(0 == uv_timer_stop(&param->handle_b));

  /* Test for starting handle_b then handle_a */
  order_cb_called = 0;
  param->handle_b.data = &param->first;
  TUV_ASSERT(0 == uv_timer_start(&param->handle_b, order_cb_b, 0, 0));

  param->handle_a.data = &param->second;
  TUV_ASSERT(0 == uv_timer_start(&param->handle_a, order_cb_a, 0, 0));

  tuv_run(param->loop, timer_loop, timer_final2, param);

  return 1; // don't clear loop param info
}

static int timer_final2(void* vparam) {
  timer_param_t* param = (timer_param_t*)vparam;

  TUV_ASSERT(order_cb_called == 2);

  // for platforms that needs cleaning
  uv_close((uv_handle_t*) &param->handle_a, NULL);
  uv_close((uv_handle_t*) &param->handle_b, NULL);
  uv_run(param->loop, UV_RUN_ONCE);
  TUV_ASSERT(0 == uv_loop_close(param->loop));

  // cleanup tuv param
  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}

TEST_IMPL(timer_order) {
  timer_param_t* param = (timer_param_t*)malloc(sizeof(timer_param_t));
  param->loop = uv_default_loop();

  order_cb_called = 0;

  param->first = 0;
  param->second = 1;
  TUV_ASSERT(0 == uv_timer_init(param->loop, &param->handle_a));
  TUV_ASSERT(0 == uv_timer_init(param->loop, &param->handle_b));

  /* Test for starting handle_a then handle_b */
  param->handle_a.data = &param->first;
  TUV_ASSERT(0 == uv_timer_start(&param->handle_a, order_cb_a, 0, 0));
  param->handle_b.data = &param->second;
  TUV_ASSERT(0 == uv_timer_start(&param->handle_b, order_cb_b, 0, 0));

  tuv_run(param->loop, timer_loop, timer_final1, param);

  return 0;
}

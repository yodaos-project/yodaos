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

static uv_timer_t tiny_timer;
static uv_timer_t huge_timer1;
static uv_timer_t huge_timer2;


static int huge_repeat_cb_ncalls;

static void huge_repeat_cb(uv_timer_t* handle) {
  if (huge_repeat_cb_ncalls == 0)
    TUV_ASSERT(handle == &huge_timer1);
  else
    TUV_ASSERT(handle == &tiny_timer);

  if (++huge_repeat_cb_ncalls == 10) {
    uv_close((uv_handle_t*) &tiny_timer, NULL);
    uv_close((uv_handle_t*) &huge_timer1, NULL);
  }
}

//-----------------------------------------------------------------------------

typedef struct {
  uv_loop_t* loop;
} timer_param_t;


static int timer_loop(void* vparam) {
  timer_param_t* param = (timer_param_t*)vparam;
  return uv_run(param->loop, UV_RUN_ONCE);
}

static int timer_final(void* vparam) {
  timer_param_t* param = (timer_param_t*)vparam;

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(param->loop));

  // cleanup tuv param
  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}


TEST_IMPL(timer_huge_repeat) {
  timer_param_t* param = (timer_param_t*)malloc(sizeof(timer_param_t));
  param->loop = uv_default_loop();

  huge_repeat_cb_ncalls = 0;

  TUV_ASSERT(0 == uv_timer_init(param->loop, &tiny_timer));
  TUV_ASSERT(0 == uv_timer_init(param->loop, &huge_timer1));
  TUV_ASSERT(0 == uv_timer_start(&tiny_timer, huge_repeat_cb, 2, 2));
  TUV_ASSERT(0 == uv_timer_start(&huge_timer1, huge_repeat_cb, 1, (uint64_t) -1));

  tuv_run(param->loop, timer_loop, timer_final, param);

  return 0;
}

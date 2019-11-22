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

static uv_thread_t _thread;
static uv_mutex_t _mutex;

static uv_timer_t _timer;
static uv_async_t _async;

static int _async_cb_called;
static int _timer_cb_called;
static int _close_cb_called;


static void task_entry(void* arg) {
}

static int task_loop(void* arg) {
  int n;
  int r;

  uv_mutex_lock(&_mutex);
  n = _async_cb_called;
  uv_mutex_unlock(&_mutex);

  if (n == 3) {
    // stop task
    return 0;
  }

  r = uv_async_send(&_async);
  TUV_ASSERT(r == 0);

  // continue task
  return 1;
}


static void close_cb(uv_handle_t* handle) {
  TUV_ASSERT(handle != NULL);
  _close_cb_called++;
}


static void async_cb(uv_async_t* handle) {
  int n;

  TUV_ASSERT(handle == &_async);

  uv_mutex_lock(&_mutex);
  n = ++_async_cb_called;
  uv_mutex_unlock(&_mutex);

  if (n == 3) {
    uv_close((uv_handle_t*)&_async, close_cb);
  }
}


static void timer_cb(uv_timer_t* timer) {
  int r;

  TUV_ASSERT(timer == &_timer);
  _timer_cb_called++;

  r = tuv_task_create(&_thread, task_entry, task_loop, NULL);
  TUV_ASSERT(r == 0);
  uv_mutex_unlock(&_mutex);

  uv_close((uv_handle_t*)timer, NULL);
}


//-----------------------------------------------------------------------------

typedef struct {
  uv_loop_t* loop;
} async_param_t;


static int async_loop(void* vparam) {
  async_param_t* param = (async_param_t*)vparam;
  int r;
  r = uv_run(param->loop, UV_RUN_ONCE);
  if (r == 0) {
    if (tuv_task_running(&_thread))
      return 1;
  }
  return r;
}


static int async_final(void* vparam) {
  async_param_t* param = (async_param_t*)vparam;

  TUV_ASSERT(_async_cb_called == 3);
  TUV_ASSERT(_close_cb_called == 1);
  TUV_ASSERT(_timer_cb_called == 1);

  //TUV_ASSERT(0 == uv_thread_join(&_thread));

  uv_mutex_destroy(&_mutex);
  tuv_task_close(&_thread);
  TUV_ASSERT(0 == uv_loop_close(param->loop));

  // cleanup tuv param
  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}


TEST_IMPL(async) {
  async_param_t* param = (async_param_t*)malloc(sizeof(async_param_t));
  param->loop = uv_default_loop();

  int r;

  _async_cb_called = 0;
  _timer_cb_called = 0;
  _close_cb_called = 0;

  r = uv_mutex_init(&_mutex);
  TUV_ASSERT(r == 0);
  uv_mutex_lock(&_mutex);

  r = uv_async_init(param->loop, &_async, async_cb);
  TUV_ASSERT(r == 0);

  r = uv_timer_init(param->loop, &_timer);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&_timer, timer_cb, 250, 0);
  TUV_ASSERT(r == 0);

  tuv_run(param->loop, async_loop, async_final, param);

  return 0;
}

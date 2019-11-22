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
#include <uv.h>


#include "uv-common.h"
#include "runner.h"


//-----------------------------------------------------------------------------

TEST_IMPL(timer_init) {
  uv_timer_t handle;

  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &handle));
  TUV_ASSERT(0 == uv_timer_get_repeat(&handle));
  TUV_ASSERT(0 == uv_is_active((uv_handle_t*) &handle));

  // for platforms that needs cleaning
  uv_close((uv_handle_t*) &handle, NULL);
  uv_run(uv_default_loop(), UV_RUN_ONCE);
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


//-----------------------------------------------------------------------------

static int once_cb_called = 0;
static int once_close_cb_called = 0;
static int repeat_cb_called = 0;
static int repeat_close_cb_called = 0;

static uint64_t start_time = 0ll;


static void once_close_cb(uv_handle_t* handle) {
  TUV_ASSERT(handle != NULL);
  TUV_ASSERT(0 == uv_is_active(handle));

  once_close_cb_called++;
}


static void once_cb(uv_timer_t* handle) {
  TUV_ASSERT(handle != NULL);
  TUV_ASSERT(0 == uv_is_active((uv_handle_t*) handle));

  once_cb_called++;

  uv_close((uv_handle_t*)handle, once_close_cb);

  /* Just call this randomly for the code coverage. */
  uv_update_time(uv_default_loop());
}


static void repeat_close_cb(uv_handle_t* handle) {
  TUV_ASSERT(handle != NULL);

  repeat_close_cb_called++;
}


static void repeat_cb(uv_timer_t* handle) {
  TUV_ASSERT(handle != NULL);
  TUV_ASSERT(1 == uv_is_active((uv_handle_t*) handle));

  repeat_cb_called++;

  if (repeat_cb_called == 5) {
    uv_close((uv_handle_t*)handle, repeat_close_cb);
  }
}


static void never_cb(uv_timer_t* handle) {
  TUV_FATAL("never_cb should never be called");
}


TEST_IMPL(timer) {
  uv_timer_t once_timers[10];
  uv_timer_t *once;
  uv_timer_t repeat, never;
  unsigned int i;
  int r;

  once_cb_called = 0;
  once_close_cb_called = 0;
  repeat_cb_called = 0;
  repeat_close_cb_called = 0;

  start_time = uv_now(uv_default_loop());
  TUV_ASSERT(0 < start_time);

  /* Let 10 timers time out in 500 ms total. */
  for (i = 0; i < ARRAY_SIZE(once_timers); i++) {
    once = once_timers + i;
    r = uv_timer_init(uv_default_loop(), once);
    TUV_ASSERT(r == 0);
    r = uv_timer_start(once, once_cb, i * 50, 0);
    TUV_ASSERT(r == 0);
  }

  /* The 11th timer is a repeating timer that runs 4 times */
  r = uv_timer_init(uv_default_loop(), &repeat);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&repeat, repeat_cb, 100, 100);
  TUV_ASSERT(r == 0);

  /* The 12th timer should not do anything. */
  r = uv_timer_init(uv_default_loop(), &never);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&never, never_cb, 100, 100);
  TUV_ASSERT(r == 0);
  r = uv_timer_stop(&never);
  TUV_ASSERT(r == 0);
  uv_unref((uv_handle_t*)&never);
  // for platforms that needs cleaning
  uv_close((uv_handle_t*) &never, NULL);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  TUV_ASSERT(once_cb_called == 10);
  TUV_ASSERT(once_close_cb_called == 10);
  TUV_ASSERT(repeat_cb_called == 5);
  TUV_ASSERT(repeat_close_cb_called == 1);

  TUV_ASSERT(500 <= uv_now(uv_default_loop()) - start_time);

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}



//-----------------------------------------------------------------------------

TEST_IMPL(timer_start_twice) {
  uv_timer_t once;
  int r;

  once_cb_called = 0;

  r = uv_timer_init(uv_default_loop(), &once);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&once, never_cb, 86400 * 1000, 0);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&once, once_cb, 10, 0);
  TUV_ASSERT(r == 0);
  r = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  TUV_ASSERT(r == 0);
  TUV_ASSERT(once_cb_called == 1);

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


//-----------------------------------------------------------------------------

static int order_cb_called = 0;

static void order_cb_a(uv_timer_t *handle) {
  TUV_ASSERT(order_cb_called++ == *(int*)handle->data);
}


static void order_cb_b(uv_timer_t *handle) {
  TUV_ASSERT(order_cb_called++ == *(int*)handle->data);
}


TEST_IMPL(timer_order) {
  int first;
  int second;
  uv_timer_t handle_a;
  uv_timer_t handle_b;

  order_cb_called = 0;

  first = 0;
  second = 1;
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &handle_a));
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &handle_b));

  /* Test for starting handle_a then handle_b */
  handle_a.data = &first;
  TUV_ASSERT(0 == uv_timer_start(&handle_a, order_cb_a, 0, 0));
  handle_b.data = &second;
  TUV_ASSERT(0 == uv_timer_start(&handle_b, order_cb_b, 0, 0));
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

  TUV_ASSERT(order_cb_called == 2);

  TUV_ASSERT(0 == uv_timer_stop(&handle_a));
  TUV_ASSERT(0 == uv_timer_stop(&handle_b));

  /* Test for starting handle_b then handle_a */
  order_cb_called = 0;
  handle_b.data = &first;
  TUV_ASSERT(0 == uv_timer_start(&handle_b, order_cb_b, 0, 0));

  handle_a.data = &second;
  TUV_ASSERT(0 == uv_timer_start(&handle_a, order_cb_a, 0, 0));
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

  TUV_ASSERT(order_cb_called == 2);

  // for platforms that needs cleaning
  uv_close((uv_handle_t*) &handle_a, NULL);
  uv_close((uv_handle_t*) &handle_b, NULL);
  uv_run(uv_default_loop(), UV_RUN_ONCE);
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


//-----------------------------------------------------------------------------

static uv_timer_t tiny_timer;
static uv_timer_t huge_timer1;
static uv_timer_t huge_timer2;


static void tiny_timer_cb(uv_timer_t* handle) {
  TUV_ASSERT(handle == &tiny_timer);
  uv_close((uv_handle_t*) &tiny_timer, NULL);
  uv_close((uv_handle_t*) &huge_timer1, NULL);
  uv_close((uv_handle_t*) &huge_timer2, NULL);
}


TEST_IMPL(timer_huge_timeout) {
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &tiny_timer));
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer1));
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer2));
  TUV_ASSERT(0 == uv_timer_start(&tiny_timer, tiny_timer_cb, 1, 0));
  TUV_ASSERT(0 == uv_timer_start(&huge_timer1, tiny_timer_cb, 0xffffffffffffLL, 0));
  TUV_ASSERT(0 == uv_timer_start(&huge_timer2, tiny_timer_cb, (uint64_t) -1, 0));
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


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


TEST_IMPL(timer_huge_repeat) {
  huge_repeat_cb_ncalls = 0;

  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &tiny_timer));
  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &huge_timer1));
  TUV_ASSERT(0 == uv_timer_start(&tiny_timer, huge_repeat_cb, 2, 2));
  TUV_ASSERT(0 == uv_timer_start(&huge_timer1, huge_repeat_cb, 1, (uint64_t) -1));
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_DEFAULT));

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


//-----------------------------------------------------------------------------

static unsigned int timer_run_once_timer_cb_called = 0;

static void timer_run_once_timer_cb(uv_timer_t* handle) {
  timer_run_once_timer_cb_called++;
}


TEST_IMPL(timer_run_once) {
  uv_timer_t timer_handle;

  timer_run_once_timer_cb_called = 0;

  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &timer_handle));
  TUV_ASSERT(0 == uv_timer_start(&timer_handle, timer_run_once_timer_cb, 0, 0));
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));
  TUV_ASSERT(1 == timer_run_once_timer_cb_called);

  TUV_ASSERT(0 == uv_timer_start(&timer_handle, timer_run_once_timer_cb, 1, 0));
  // slow systems may have nano second resolution
  // give some time to sleep so that time tick is changed
  uv_sleep(1);
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));
  TUV_ASSERT(2 == timer_run_once_timer_cb_called);

  uv_close((uv_handle_t*)&timer_handle, NULL);
  TUV_ASSERT(0 == uv_run(uv_default_loop(), UV_RUN_ONCE));

  // for platforms that needs cleaning
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


TEST_IMPL(timer_null_callback) {
  uv_timer_t handle;

  TUV_ASSERT(0 == uv_timer_init(uv_default_loop(), &handle));
  TUV_ASSERT(UV_EINVAL == uv_timer_start(&handle, NULL, 100, 100));

  // for platform that needs closing
  tuv_timer_deinit(uv_default_loop(), &handle);
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}

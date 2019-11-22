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


static int close_cb_called = 0;
static int repeat_1_cb_called = 0;
static int repeat_2_cb_called = 0;
static int repeat_2_cb_allowed = 0;

static uv_timer_t dummy, repeat_1, repeat_2;

static uint64_t start_time;


static void close_cb(uv_handle_t* handle) {
  TUV_ASSERT(handle != NULL);

  close_cb_called++;
}


static void repeat_1_cb(uv_timer_t* handle) {
  int r;

  TUV_ASSERT(handle == &repeat_1);
  TUV_ASSERT(uv_timer_get_repeat((uv_timer_t*)handle) == 50);

  repeat_1_cb_called++;

  r = uv_timer_again(&repeat_2);
  TUV_ASSERT(r == 0);

  if (repeat_1_cb_called == 10) {
    uv_close((uv_handle_t*)handle, close_cb);
    /* We're not calling uv_timer_again on repeat_2 any more, so after this */
    /* timer_2_cb is expected. */
    repeat_2_cb_allowed = 1;
    return;
  }
}


static void repeat_2_cb(uv_timer_t* handle) {
  TUV_ASSERT(handle == &repeat_2);
  TUV_ASSERT(repeat_2_cb_allowed);

  repeat_2_cb_called++;

  if (uv_timer_get_repeat(&repeat_2) == 0) {
    TUV_ASSERT(0 == uv_is_active((uv_handle_t*) handle));
    uv_close((uv_handle_t*)handle, close_cb);
    return;
  }

  TUV_ASSERT(uv_timer_get_repeat(&repeat_2) == 100);

  /* This shouldn't take effect immediately. */
  uv_timer_set_repeat(&repeat_2, 0);
}


TEST_IMPL(timer_again) {
  int r;

  close_cb_called = 0;
  repeat_1_cb_called = 0;
  repeat_2_cb_called = 0;
  repeat_2_cb_allowed = 0;

  start_time = uv_now(uv_default_loop());
  TUV_ASSERT(0 < start_time);

  /* Verify that it is not possible to uv_timer_again a never-started timer. */
  r = uv_timer_init(uv_default_loop(), &dummy);
  TUV_ASSERT(r == 0);
  r = uv_timer_again(&dummy);
  TUV_ASSERT(r == UV_EINVAL);
  uv_unref((uv_handle_t*)&dummy);

  /* Start timer repeat_1. */
  r = uv_timer_init(uv_default_loop(), &repeat_1);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&repeat_1, repeat_1_cb, 50, 0);
  TUV_ASSERT(r == 0);
  TUV_ASSERT(uv_timer_get_repeat(&repeat_1) == 0);

  /* Actually make repeat_1 repeating. */
  uv_timer_set_repeat(&repeat_1, 50);
  TUV_ASSERT(uv_timer_get_repeat(&repeat_1) == 50);

  /*
   * Start another repeating timer. It'll be again()ed by the repeat_1 so
   * it should not time out until repeat_1 stops.
   */
  r = uv_timer_init(uv_default_loop(), &repeat_2);
  TUV_ASSERT(r == 0);
  r = uv_timer_start(&repeat_2, repeat_2_cb, 100, 100);
  TUV_ASSERT(r == 0);
  TUV_ASSERT(uv_timer_get_repeat(&repeat_2) == 100);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  TUV_ASSERT(repeat_1_cb_called == 10);
  TUV_ASSERT(repeat_2_cb_called == 2);
  TUV_ASSERT(close_cb_called == 2);

  // for platform that needs closing
  tuv_timer_deinit(uv_default_loop(), &dummy);
  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}

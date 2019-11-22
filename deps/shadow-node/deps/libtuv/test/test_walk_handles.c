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


static char magic_cookie[] = "magic cookie";
static int seen_timer_handle;
static uv_timer_t timer;


static void walk_cb(uv_handle_t* handle, void* arg) {
  TUV_ASSERT(arg == (void*)magic_cookie);

  if (handle == (uv_handle_t*)&timer) {
    seen_timer_handle++;
  } else {
    TUV_ASSERT(0 && "unexpected handle");
  }
}


static void timer_cb(uv_timer_t* handle) {
  TUV_ASSERT(handle == &timer);

  uv_walk(handle->loop, walk_cb, magic_cookie);
  uv_close((uv_handle_t*)handle, NULL);
}


TEST_IMPL(walk_handles) {
  uv_loop_t* loop;
  int r;

  seen_timer_handle = 0;

  loop = uv_default_loop();

  r = uv_timer_init(loop, &timer);
  TUV_ASSERT(r == 0);

  r = uv_timer_start(&timer, timer_cb, 1, 0);
  TUV_ASSERT(r == 0);

  /* Start event loop, expect to see the timer handle in walk_cb. */
  TUV_ASSERT(seen_timer_handle == 0);
  r = uv_run(loop, UV_RUN_DEFAULT);
  TUV_ASSERT(r == 0);
  TUV_ASSERT(seen_timer_handle == 1);

  /* Loop is finished, walk_cb should not see our timer handle. */
  seen_timer_handle = 0;
  uv_walk(loop, walk_cb, magic_cookie);
  TUV_ASSERT(seen_timer_handle == 0);

  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}

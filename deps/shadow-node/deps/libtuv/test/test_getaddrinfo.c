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

#include <assert.h>
#include <stdio.h>

#include <uv.h>
#include "runner.h"

#define CONCURRENT_COUNT    10

static const char* name = "localhost";

static int getaddrinfo_cbs = 0;

static uv_getaddrinfo_t* getaddrinfo_handle;
static uv_getaddrinfo_t getaddrinfo_handles[CONCURRENT_COUNT];
static int callback_counts[CONCURRENT_COUNT];


static void getaddrinfo_basic_cb(uv_getaddrinfo_t* handle,
                                 int status,
                                 struct addrinfo* res) {
  TUV_ASSERT(handle == getaddrinfo_handle);
  getaddrinfo_cbs++;
  free(handle);
  uv_freeaddrinfo(res);
}


TEST_IMPL(getaddrinfo_basic) {
  int r;

  getaddrinfo_cbs = 0;

  getaddrinfo_handle = (uv_getaddrinfo_t*)malloc(sizeof(uv_getaddrinfo_t));

  r = uv_getaddrinfo(uv_default_loop(),
                     getaddrinfo_handle,
                     &getaddrinfo_basic_cb,
                     name,
                     NULL,
                     NULL);
  TUV_ASSERT(r == 0);

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  TUV_ASSERT(getaddrinfo_cbs == 1);

  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}


TEST_IMPL(getaddrinfo_basic_sync) {
  uv_getaddrinfo_t req;

  TUV_ASSERT(0 == uv_getaddrinfo(uv_default_loop(),
                                 &req,
                                 NULL,
                                 name,
                                 NULL,
                                 NULL));
  uv_freeaddrinfo(req.addrinfo);

  return 0;
}



static void getaddrinfo_cuncurrent_cb(uv_getaddrinfo_t* handle,
                                      int status,
                                      struct addrinfo* res) {
  int i;
  int* data = (int*)handle->data;

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    if (&getaddrinfo_handles[i] == handle) {
      TUV_ASSERT(i == *data);

      callback_counts[i]++;
      break;
    }
  }
  TUV_ASSERT(i < CONCURRENT_COUNT);

  free(data);
  uv_freeaddrinfo(res);

  getaddrinfo_cbs++;
}


TEST_IMPL(getaddrinfo_concurrent) {
  int i, r;
  int* data;

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    callback_counts[i] = 0;

    data = (int*)malloc(sizeof(int));
    TUV_ASSERT(data != NULL);
    *data = i;
    getaddrinfo_handles[i].data = data;

    r = uv_getaddrinfo(uv_default_loop(),
                       &getaddrinfo_handles[i],
                       &getaddrinfo_cuncurrent_cb,
                       name,
                       NULL,
                       NULL);
    TUV_ASSERT(r == 0);
  }

  uv_run(uv_default_loop(), UV_RUN_DEFAULT);

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    TUV_ASSERT(callback_counts[i] == 1);
  }

  TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));

  return 0;
}

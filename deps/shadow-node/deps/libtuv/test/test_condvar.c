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
#include <errno.h>

#include <uv.h>

#include "runner.h"

typedef struct {
  uv_mutex_t mutex;
  uv_cond_t cond;
  int delay;
  int use_broadcast;
  volatile int posted;
} worker_config;


static void worker(void* arg) {
  worker_config* c = (worker_config*)arg;

  if (c->delay)
    run_sleep(c->delay);

  uv_mutex_lock(&c->mutex);
  TUV_ASSERT(c->posted == 0);
  c->posted = 1;
  if (c->use_broadcast)
    uv_cond_broadcast(&c->cond);
  else
    uv_cond_signal(&c->cond);
  uv_mutex_unlock(&c->mutex);
}


TEST_IMPL(condvar_1) {
  uv_thread_t thread;
  worker_config wc;

  memset(&wc, 0, sizeof(wc));

  TUV_ASSERT(0 == uv_cond_init(&wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&wc.mutex));
  TUV_ASSERT(0 == uv_thread_create(&thread, worker, &wc));

  uv_mutex_lock(&wc.mutex);
  run_sleep(100);
  uv_cond_wait(&wc.cond, &wc.mutex);
  TUV_ASSERT(wc.posted == 1);
  uv_mutex_unlock(&wc.mutex);

  TUV_ASSERT(0 == uv_thread_join(&thread));
  uv_mutex_destroy(&wc.mutex);
  uv_cond_destroy(&wc.cond);

  return 0;
}


TEST_IMPL(condvar_2) {
  uv_thread_t thread;
  worker_config wc;

  memset(&wc, 0, sizeof(wc));
  wc.delay = 100;

  TUV_ASSERT(0 == uv_cond_init(&wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&wc.mutex));
  TUV_ASSERT(0 == uv_thread_create(&thread, worker, &wc));

  uv_mutex_lock(&wc.mutex);
  uv_cond_wait(&wc.cond, &wc.mutex);
  uv_mutex_unlock(&wc.mutex);

  TUV_ASSERT(0 == uv_thread_join(&thread));
  uv_mutex_destroy(&wc.mutex);
  uv_cond_destroy(&wc.cond);

  return 0;
}


TEST_IMPL(condvar_3) {
  uv_thread_t thread;
  worker_config wc;
  int r;

  memset(&wc, 0, sizeof(wc));
  wc.delay = 100;

  TUV_ASSERT(0 == uv_cond_init(&wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&wc.mutex));
  TUV_ASSERT(0 == uv_thread_create(&thread, worker, &wc));

  uv_mutex_lock(&wc.mutex);
  r = uv_cond_timedwait(&wc.cond, &wc.mutex, (uint64_t)(50 * 1e6));
  TUV_ASSERT(r == UV_ETIMEDOUT);
  uv_mutex_unlock(&wc.mutex);

  TUV_ASSERT(0 == uv_thread_join(&thread));
  uv_mutex_destroy(&wc.mutex);
  uv_cond_destroy(&wc.cond);

  return 0;
}


TEST_IMPL(condvar_4) {
  uv_thread_t thread;
  worker_config wc;
  int r;

  memset(&wc, 0, sizeof(wc));
  wc.delay = 100;

  TUV_ASSERT(0 == uv_cond_init(&wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&wc.mutex));
  TUV_ASSERT(0 == uv_thread_create(&thread, worker, &wc));

  uv_mutex_lock(&wc.mutex);
  r = uv_cond_timedwait(&wc.cond, &wc.mutex, (uint64_t)(250 * 1e6));
  TUV_ASSERT(r == 0);
  uv_mutex_unlock(&wc.mutex);

  TUV_ASSERT(0 == uv_thread_join(&thread));
  uv_mutex_destroy(&wc.mutex);
  uv_cond_destroy(&wc.cond);

  return 0;
}


TEST_IMPL(condvar_5) {
  uv_thread_t thread;
  worker_config wc;

  memset(&wc, 0, sizeof(wc));
  wc.use_broadcast = 1;
  wc.delay = 100;

  TUV_ASSERT(0 == uv_cond_init(&wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&wc.mutex));
  TUV_ASSERT(0 == uv_thread_create(&thread, worker, &wc));

  uv_mutex_lock(&wc.mutex);
  run_sleep(100);
  uv_cond_wait(&wc.cond, &wc.mutex);
  TUV_ASSERT(wc.posted == 1);
  uv_mutex_unlock(&wc.mutex);

  TUV_ASSERT(0 == uv_thread_join(&thread));
  uv_mutex_destroy(&wc.mutex);
  uv_cond_destroy(&wc.cond);

  return 0;
}

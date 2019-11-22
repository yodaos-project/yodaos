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
  void* data;
} worker_config;


static void worker_entry(void* arg) {
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

static int worker_loop(void* arg) {
  // stop the loop
  return 0;
}


typedef struct {
  uv_loop_t* loop;
  uv_thread_t thread;
  worker_config wc;
  int waitcond;
} condvar_1_param_t;


static int condvar_1_loop(void* vparam) {
  condvar_1_param_t* param = (condvar_1_param_t*)vparam;
  int r;

  if (param->waitcond) {
    uv_mutex_lock(&param->wc.mutex);
    run_sleep(100);
    r = tuv_cond_wait(&param->wc.cond, &param->wc.mutex);
    uv_mutex_unlock(&param->wc.mutex);
    if (r == 0) {
      // keep the loop going
      return 1;
    }
    param->waitcond = 0;
  }
  // check task has closed(something like thread joined)
  r = tuv_task_running(&param->thread);
  // r=1 for task needs to run, return 1 so that loop keeps going
  // r=0 for task has ended, return 0 so that loop ends
  return r > 0 ? 1 : 0;
}

static int condvar_1_final(void* vparam) {
  condvar_1_param_t* param = (condvar_1_param_t*)vparam;

  TUV_ASSERT(param->wc.posted == 1);

  tuv_task_close(&param->thread);
  uv_mutex_destroy(&param->wc.mutex);
  uv_cond_destroy(&param->wc.cond);

  // cleanup tuv param
  free(param);

  // jump to next test
  run_tests_continue();

  return 0;
}


TEST_IMPL(condvar_1) {
  condvar_1_param_t* param;
  param = (condvar_1_param_t*)malloc(sizeof(condvar_1_param_t));
  param->loop = uv_default_loop();

  memset(&param->wc, 0, sizeof(param->wc));
  param->wc.data = param;
  param->waitcond = 1;

  TUV_ASSERT(0 == uv_cond_init(&param->wc.cond));
  TUV_ASSERT(0 == uv_mutex_init(&param->wc.mutex));
  TUV_ASSERT(0 == tuv_task_create(&param->thread,
                                 worker_entry, worker_loop,
                                 &param->wc));

  tuv_run(param->loop, condvar_1_loop, condvar_1_final, param);

  return 0;
}

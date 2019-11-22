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
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <uv.h>

#include "runner.h"

/*
 * About this file.
 * "runner_linux_raw.c" is used as to create only for host executable,
 * for unit test running inside target boards which can have limited
 * capabilites.
 * Executable is (and should be) generated only for host system, and
 * the file is placed at same place as tuvtester program with
 * the name 'tuvhelper'.
 *
 * current usage: helper for echo server for mbed
 *    mbed cannot run echo server and at the same time a client connecting
 *    to the echo server.
 */


void run_sleep(int msec) {
  usleep(msec * 1000);
}


static pthread_t tid = 0;

int platform_init(int argc, char **argv) {
  return 0;
}

static void* helper_proc(void* data) {
  task_entry_t* task = (task_entry_t*)data;
  int ret;

  sem_post(&task->semsync);
  ret = task->main();
  TUV_ASSERT(ret == 0);

  sem_post(&task->semsync);
  pthread_exit(0);
  return NULL;
}

int run_helper(task_entry_t* task) {
  int r;
  sem_init(&task->semsync, 0, 0);
  r = pthread_create(&tid, NULL, helper_proc, task);
  sem_wait(&task->semsync);
  return r;
}

int wait_helper(task_entry_t* task) {
  sem_wait(&task->semsync);
  pthread_join(tid, NULL);
  sem_destroy(&task->semsync);
  return 0;
}

int run_test_one(task_entry_t* task) {
  return run_test_part(task->task_name, task->process_name);
}

int main(int argc, char *argv[]) {
  InitDebugSettings();
  platform_init(argc, argv);

  task_entry_t* task;
  task = get_helper("tcp4_echo_server");
  if (task) {
    TDDDLOG("Running network helper for target boards.");
    run_helper(task);
    TDDDLOG("Waiting for Test client and quit command...");
    wait_helper(task);
  }
  ReleaseDebugSettings();
  return 0;
}

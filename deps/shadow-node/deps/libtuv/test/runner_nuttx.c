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


/* Do platform-specific initialization. */
int platform_init(int argc, char **argv) {
  return 0;
}

void run_sleep(int msec) {
  usleep(msec * 1000);
}

//-----------------------------------------------------------------------------

#define HELPER_PRIORITY     100
#define HELPER_STACKSIZE    2048

static pid_t pid_helper;
static sem_t startsem;


static int helper_procedure(int argc, char *argv[]) {
  task_entry_t* helper;
  int ret;
  helper = get_helper(argv[1]);
  TUV_ASSERT(helper);

  sem_post(&helper->semsync);
  ret = helper->main();
  TUV_ASSERT(ret == 0);

  sem_post(&helper->semsync);

  return ret;
}

int run_helper(task_entry_t* task) {

  const char* argv[3] = {
    task->task_name,
    task->process_name,
    0
  };

  sem_init(&task->semsync, 0, 0);

  pid_helper = task_create(task->process_name,
                           HELPER_PRIORITY, HELPER_STACKSIZE,
                           helper_procedure,
                           (char * const *)argv);

  sem_wait(&task->semsync);

  usleep(1000);

  return 0;
}

int wait_helper(task_entry_t* task) {
  sem_wait(&task->semsync);
  task_delete(pid_helper);
  sem_destroy(&task->semsync);
  return 0;
}


int run_test_one(task_entry_t* task) {
  return run_test_part(task->task_name, task->process_name);
}


//
// this is called from nuttx apps system tuvtester
//
int tuvtester_entry(int argc, char *argv[]) {
  fprintf(stderr, ">> tuvtester_entry call ok\n");
  fflush(stderr);
  platform_init(argc, argv);
  if (argc>2) {
    return run_test_part(argv[1], argv[2]);
  }
  return run_tests();
}

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
#include <uv.h>

#include "runner.h"


//-----------------------------------------------------------------------------
// test function list

#define TEST_ENTRY(name, timeout) \
  { #name, #name, &run_test_##name, 0, 0, timeout },

task_entry_t TASKS[] = {

  TEST_LIST_ALL(TEST_ENTRY)
  TEST_LIST_EXT(TEST_ENTRY)

  { NULL, NULL, NULL, 0, 0, 0 }
};

#undef TEST_ENTRY

//-----------------------------------------------------------------------------
// test function list

#define HELPER_ENTRY(task_name, name)               \
    { #task_name, #name, &run_helper_##name, 1, 0, 0 },

task_entry_t HELPERS[] = {

  HELPER_LIST_ALL(HELPER_ENTRY)

  { NULL, NULL, NULL, 0, 0, 0 }
};

#undef HELPER_ENTRY

//-----------------------------------------------------------------------------

int run_test_part(const char* test, const char* part) {
  int r;
  task_entry_t* task;

  for (task = TASKS; task->main; task++) {
    if (strcmp(test, task->task_name) == 0 &&
        strcmp(part, task->process_name) == 0) {
      r = task->main();
      return r;
    }
  }

  fprintf(stderr, "No test part with that name: %s:%s\n", test, part);
  return 255;
}


task_entry_t* get_helper(const char* test) {
  int r;
  task_entry_t* task;

  for (task = HELPERS; task->main; task++) {
    if (strcmp(test, task->task_name) == 0) {
      return task;
    }
  }
  return NULL;
}


int run_tests() {
  int entry;
  int result;
  task_entry_t* task;

  fprintf(stderr, "Run Helpers...\r\n");
  entry = 0;
  while (HELPERS[entry].task_name) {
    task = &HELPERS[entry];

    fprintf(stderr, "[%-30s]...", task->task_name);
    result = run_helper(task);
    fprintf(stderr, "%s\r\n", result ? "failed" : "OK");
    entry++;
  }

  entry = 0;
  while (TASKS[entry].task_name) {
    task = &TASKS[entry];

    fprintf(stderr, "[%-30s]...", task->task_name);
    result = run_test_one(task);
    fprintf(stderr, "%s\r\n", result ? "failed" : "OK");
    entry++;
  }

  fprintf(stderr, "Waiting Helpers to end...\r\n");
  entry = 0;
  while (HELPERS[entry].task_name) {
    task = &HELPERS[entry];

    fprintf(stderr, "[%-30s]...", task->task_name);
    result = wait_helper(task);
    fprintf(stderr, "%s\r\n", result ? "failed" : "OK");
    entry++;
  }

  fprintf(stderr, "run_tests end\r\n");

  return 0;
}

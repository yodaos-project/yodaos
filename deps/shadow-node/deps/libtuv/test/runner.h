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

#ifndef __tuv_test_runner_header__
#define __tuv_test_runner_header__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


#if defined(__NUTTX__) || defined(__TUV_RAW__)
#define EMBED_LOW_MEMORY
#endif

// for tcp_test
#define TEST_PORT 9123
#define TEST_PORT_2 9124


typedef struct {
  const char *task_name;
  const char *process_name;
  int (*main)(void);
  int is_helper;
  int show_output;

  /*
   * The time in milliseconds after which a single test or benchmark times out.
   */
  int timeout;
  uv_sem_t semsync;
} task_entry_t;

/* This macro cleans up the main loop. This is used to avoid valgrind
 * warnings about memory being "leaked" by the main event loop.
 */
#define MAKE_VALGRIND_HAPPY()                           \
  do {                                                  \
    close_loop(uv_default_loop());                      \
    TUV_ASSERT(0 == uv_loop_close(uv_default_loop()));  \
  } while (0)

#define TEST_IMPL(name)                                                       \
  int run_test_##name(void);                                                  \
  int run_test_##name(void)

#define HELPER_IMPL(name)                                                     \
  int run_helper_##name(void);                                                \
  int run_helper_##name(void)


/* Have our own assert, so we are sure it does not get optimized away in
 * a release build.
 */
#define TUV_ASSERT(expr)                                  \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Assertion failed in %s on line %d: %s\r\n",  \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    ABORT();                                              \
  }                                                       \
 } while (0)

#define TUV_WARN(expr)                                    \
 do {                                                     \
  if (!(expr)) {                                          \
    fprintf(stderr,                                       \
            "Failed in %s on line %d: %s\r\n",            \
            __FILE__,                                     \
            __LINE__,                                     \
            #expr);                                       \
    return -1;                                            \
  }                                                       \
 } while (0)

#define TUV_FATAL(msg)                                    \
  do {                                                    \
    fprintf(stderr,                                       \
            "Fatal error in %s on line %d: %s\r\n",       \
            __FILE__,                                     \
            __LINE__,                                     \
            msg);                                         \
    fflush(stderr);                                       \
    ABORT();                                              \
  } while (0)

/* Fully close a loop */
static void close_walk_cb(uv_handle_t* handle, void* arg) {
  if (!uv_is_closing(handle))
    uv_close(handle, NULL);
}

static void close_loop(uv_loop_t* loop) {
  uv_walk(loop, close_walk_cb, NULL);
  uv_run(loop, UV_RUN_DEFAULT);
}

/* Reserved test exit codes. */
enum test_status {
  TEST_OK = 0,
  TEST_TODO,
  TEST_SKIP
};

typedef struct {
  FILE* stdout_file;
  pid_t pid;
  char* name;
  int status;
  int terminated;
} process_info_t;

typedef struct {
  int pipe[2];
  process_info_t* vec;
  int n;
} dowait_args;

/* tcp server */
typedef struct {
  uv_write_t req;
  uv_buf_t buf;
} write_req_t;

typedef enum {
  TEST_TCP = 0,
  TEST_UDP,
  TEST_PIPE
} stream_type;

#ifdef PATH_MAX
#define EXEC_PATH_LENGTH PATH_MAX
#else
#define EXEC_PATH_LENGTH 4096
#endif


int process_start(const char* name, const char* part, process_info_t* p,
                  int is_helper);
int process_wait(process_info_t* vec, int n, int timeout);
int process_reap(process_info_t *p);
int process_read_last_line(process_info_t *p, char* buffer, size_t buffer_len);
void process_cleanup(process_info_t *p);

int run_test_one(task_entry_t* task);
int run_test_part(const char* test, const char* part);
int run_tests(void);
void run_sleep(int msec);

task_entry_t* get_helper(const char* test);
int run_helper(task_entry_t* task);
int wait_helper(task_entry_t* task);


// for raw systems
void run_tests_init(void);
void run_tests_one(void);
void run_tests_continue(void);


#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
// test function declaration

#include "runner_list.h"


#define TEST_DECLARE(name,x)                                                  \
  int run_test_##name(void);

#define HELPER_DECLARE(task_name,name)                                        \
  int run_helper_##name(void);


TEST_LIST_ALL(TEST_DECLARE)
TEST_LIST_EXT(TEST_DECLARE)

HELPER_LIST_ALL(HELPER_DECLARE)

#undef TEST_DECLARE
#undef HELPER_DECLARE


#endif // __tuv_test_runner_header__

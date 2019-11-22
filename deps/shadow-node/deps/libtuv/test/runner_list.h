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


#if !defined(__TUV_RAW__)
#define TEST_LIST_ALL(TE)                                                     \
  TE(idle_basic, 5000)                                                        \
  TE(active, 5000)                                                            \
  TE(timer_init, 5000)                                                        \
  TE(timer, 5000)                                                             \
  TE(timer_start_twice, 5000)                                                 \
  TE(timer_order, 5000)                                                       \
  TE(timer_run_once, 5000)                                                    \
  TE(timer_null_callback, 5000)                                               \
  TE(timer_again, 5000)                                                       \
  TE(timer_huge_timeout, 5000)                                                \
  TE(timer_huge_repeat, 5000)                                                 \
  \
  TE(async, 5000)                                                             \
  TE(condvar_2, 5000)                                                         \
  TE(condvar_3, 5000)                                                         \
  TE(cwd, 5000)                                                               \
  \
  TE(fs_file_noent, 5000)                                                     \
  TE(fs_file_sync, 5000)                                                      \
  TE(fs_file_async, 5000)                                                     \
  TE(fs_file_write_null_buffer, 5000)                                         \
  TE(fs_stat_missing_path, 5000)                                              \
  TE(fs_scandir_empty_dir, 5000)                                              \
  TE(fs_scandir_file, 5000)                                                   \
  TE(fs_open_dir, 5000)                                                       \
  TE(fs_file_open_append, 5000)                                               \
  TE(fs_read_file_eof, 5000)                                                  \
  TE(fs_async_dir, 5000)                                                      \
  \
  TE(error_message, 5000)                                                     \
  \
  TE(threadpool_queue_work_simple, 5000)                                      \
  TE(walk_handles, 5000)                                                      \
  \
  TE(tcp_open,5000)                                                           \
  TE(shutdown_eof,5000)                                                       \

#else
#define TEST_LIST_ALL(TE)                                                     \
  TE(idle_basic, 5000)                                                        \
  TE(active, 5000)                                                            \
  TE(timer_init, 5000)                                                        \
  TE(timer, 5000)                                                             \
  TE(timer_start_twice, 5000)                                                 \
  TE(timer_order, 5000)                                                       \
  TE(timer_run_once, 5000)                                                    \
  TE(timer_null_callback, 5000)                                               \
  TE(timer_again, 5000)                                                       \
  TE(timer_huge_timeout, 5000)                                                \
  TE(timer_huge_repeat, 5000)                                                 \
  \
  TE(async, 5000)                                                             \
  TE(condvar_1, 5000)                                                         \
  \
  TE(threadpool_queue_work_simple, 5000)                                      \
  TE(walk_handles, 5000)                                                      \
  TE(tcp_open,5000)                                                           \
  TE(shutdown_eof,5000)                                                       \


#endif

// shutdown_eof should be last of tcp test, it'll stop "echo_sevrer"

#if defined(TUV_TEST_FORK)
#define TEST_LIST_EXT_FORK(TE) \
  TE(fork_timer, 5000) \
  TE(fork_socketpair, 5000) \
  TE(fork_socketpair_started, 5000) \
  TE(fork_signal_to_child, 5000) \
  TE(fork_signal_to_child_closed, 5000) \
  TE(fork_threadpool_queue_work_simple, 5000) \

#else
#define TEST_LIST_EXT_FORK(TE)
#endif

#if defined(__linux__)
#define TEST_LIST_EXT(TE)                                                     \
  TE(condvar_1, 5000)                                                         \
  TE(condvar_4, 5000)                                                         \
  \
  TE(fs_file_nametoolong, 5000)                                               \
  TE(fs_fstat, 5000)                                                          \
  TE(fs_utime, 5000)                                                          \
  TE(fs_futime, 5000)                                                         \
  \
  TE(getaddrinfo_basic, 5000)                                                 \
  TE(getaddrinfo_basic_sync, 5000)                                            \
  TE(getaddrinfo_concurrent, 5000)                                            \
  \
  TEST_LIST_EXT_FORK(TE)

#elif defined(__APPLE__)
#define TEST_LIST_EXT(TE)                                                     \
  TEST_LIST_EXT_FORK(TE)

#elif defined(__TUV_RAW__)
#define TEST_LIST_EXT(TE)                                                     \

#else
#define TEST_LIST_EXT(TE)                                                     \

#endif

#if !defined(__TUV_RAW__)
#define HELPER_LIST_ALL(TE)                                                   \
  TE(tcp4_echo_server, tcp4_echo_server)                                      \

#else
#define HELPER_LIST_ALL(TE)                                                   \

#endif

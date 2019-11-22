# Copyright 2015-2017 Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set(FLAGS_COMMON
      "-fno-builtin"
      "-D__TUV_RAW__"
      )

set(CMAKE_C_FLAGS_DEBUG     "-O0 -g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG")

# raw common source files
set(RAW_PATH "${SOURCE_ROOT}/raw")
set(RAW_TEST "${TEST_ROOT}/raw")

# test include
set(TUV_TEST_INCDIRS
      "${TEST_ROOT}"
      "${TEST_ROOT}/raw"
      )

set(PLATFORM_SRCFILES
      "${RAW_PATH}/uv_raw.c"
      "${RAW_PATH}/uv_raw_clock.c"
      "${RAW_PATH}/uv_raw_io.c"
      "${RAW_PATH}/uv_raw_loop.c"
      "${RAW_PATH}/uv_raw_threadpool.c"
      "${RAW_PATH}/uv_raw_thread.c"

      "${RAW_PATH}/uv_raw_async.c"
      "${RAW_PATH}/uv_raw_process.c"
      )

set(TEST_MAINFILE
      "${RAW_TEST}/raw_main.c"
      )

set(TEST_UNITFILES
      "${RAW_TEST}/runner_main_raw.c"
      "${RAW_TEST}/test_idle_raw.c"
      "${RAW_TEST}/test_active_raw.c"
      "${RAW_TEST}/test_timer_raw_init.c"
      "${RAW_TEST}/test_timer_raw_norm.c"
      "${RAW_TEST}/test_timer_raw_start_twice.c"
      "${RAW_TEST}/test_timer_raw_order.c"
      "${RAW_TEST}/test_timer_raw_run_once.c"
      "${RAW_TEST}/test_timer_raw_run_null_callback.c"
      "${RAW_TEST}/test_timer_raw_again.c"
      "${RAW_TEST}/test_timer_raw_huge_timeout.c"
      "${RAW_TEST}/test_timer_raw_huge_repeat.c"
      "${RAW_TEST}/test_condvar_raw.c"
      "${RAW_TEST}/test_async_raw.c"
      "${RAW_TEST}/test_threadpool_raw_queue_work_simple.c"
      "${RAW_TEST}/test_walk_handles_raw.c"

      "${RAW_TEST}/test_tcp_open_raw.c"
      "${RAW_TEST}/test_shutdown_eof_raw.c"
      )

set(TEST_APIEMULFILES
      "${RAW_TEST}/apiemul_main.c"
      "${RAW_TEST}/apiemul_socket.c"
      )

# configuration values
set(CONFIG_FILESYSTEM 0)

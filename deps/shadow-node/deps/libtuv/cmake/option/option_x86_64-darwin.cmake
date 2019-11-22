# Copyright 2017 Samsung Electronics Co., Ltd.
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

# unix common

include("cmake/option/option_unix_common.cmake")

# darwin specific

set(PLATFORM_SRCFILES
      ${PLATFORM_SRCFILES}
      ${INCLUDE_ROOT}/uv-darwin.h
      ${INCLUDE_ROOT}/pthread-barrier.h
#     ${UNIX_PATH}/bsd-ifaddrs.c
      ${UNIX_PATH}/loop.c
      ${UNIX_PATH}/darwin.c
      ${UNIX_PATH}/darwin-proctitle.c
      ${UNIX_PATH}/fsevents.c
      ${UNIX_PATH}/kqueue.c
      ${UNIX_PATH}/proctitle.c
      ${UNIX_PATH}/pipe.c
      ${UNIX_PATH}/pthread-barrier.c
      ${UNIX_PATH}/signal.c
      )

set(PLATFORM_TESTFILES "${TEST_ROOT}/runner_linux.c")

set(PLATFORM_HOSTHELPERFILES "${TEST_ROOT}/runner_linux_raw.c")

set(TUV_LINK_LIBS "pthread")

if(DEFINED BUILD_HOST_HELPER AND BUILD_HOST_HELPER STREQUAL "yes")
  add_definitions("-D__HOST_HELPER__")
endif()

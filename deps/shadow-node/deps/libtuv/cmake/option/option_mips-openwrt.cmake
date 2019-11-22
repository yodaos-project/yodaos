# Copyright 2016 Samsung Electronics Co., Ltd.
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

# linux common
include("cmake/option/option_unix_common.cmake")

set(TUV_PLATFORM_PATH "linux")
set(BUILDTESTER "no")
set(BUILDAPIEMULTESTER "no")

# linux specific source files
set(LINUX_PATH "${SOURCE_ROOT}/${TUV_PLATFORM_PATH}")

set(PLATFORM_SRCFILES ${PLATFORM_SRCFILES}
                      "${LINUX_PATH}/uv_linux.c"
                      "${LINUX_PATH}/uv_linux_loop.c"
                      "${LINUX_PATH}/uv_linux_clock.c"
                      "${LINUX_PATH}/uv_linux_io.c"
                      "${LINUX_PATH}/uv_linux_syscall.c"
                      "${LINUX_PATH}/uv_linux_thread.c"
                      )

set(PLATFORM_TESTFILES "${TEST_ROOT}/runner_linux.c"
                       )

set(TUV_LINK_LIBS "pthread")

# mips-linux specific
if(DEFINED TARGET_BOARD)
endif()

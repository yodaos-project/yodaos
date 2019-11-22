# Copyright 2017-present Samsung Electronics Co., Ltd.
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

include("cmake/option/option_unix_common.cmake")

set(FLAGS_COMMON ${FLAGS_COMMON} "-D__TIZENRT__")


set(PLATFORM_SRCFILES
      ${PLATFORM_SRCFILES}
      ${UNIX_PATH}/tizenrt.c
      )

set(PLATFORM_TESTFILES "${TEST_ROOT}/runner_tizenrt.c")

if(DEFINED TARGET_BOARD)
  if(${TARGET_BOARD} STREQUAL "artik05x")
    set(FLAGS_COMMON
          ${FLAGS_COMMON}
          "-mcpu=cortex-r4"
          "-mfpu=vfp3"
          )
  else()
    message(FATAL_ERROR "TARGET_BOARD=`${TARGET_BOARD}` is unknown to make")
  endif()
else()
  message(FATAL_ERROR "TARGET_BOARD is undefined")
endif()

if(NOT DEFINED TARGET_SYSTEMROOT OR "${TARGET_SYSTEMROOT}" STREQUAL "default")
  message(FATAL_ERROR "\nPlease set TARGET_SYSTEMROOT for TizenRT include path\n")
endif()

# system include folder
set(TARGET_INC
      ${TARGET_INC}
      "${TARGET_SYSTEMROOT}/include"
      "${TARGET_SYSTEMROOT}/include/cxx"
      )

# build tester as library
set(BUILD_TEST_LIB "yes")
unset(BUILD_TEST_LIB CACHE)

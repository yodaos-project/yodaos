# Copyright 2015 Samsung Electronics Co., Ltd.
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

include("cmake/option/option_raw_common.cmake")

# mbed common source files
set(MBED_PATH "${SOURCE_ROOT}/mbed")

set(PLATFORM_SRCFILES
      ${PLATFORM_SRCFILES}
      "${MBED_PATH}/uv_mbed.c"
      "${MBED_PATH}/uv_mbed_io.c"
      "${MBED_PATH}/uv_mbed_stream.c"
      "${MBED_PATH}/uv_mbed_tcp.c"
      )

#set(PLATFORM_TESTFILES "${TEST_ROOT}/runner_mbed.c")

set(FLAGS_COMMON
      ${FLAGS_COMMON}
      "-D__TUV_MBED__"
      )

# use "mbed" lower case for platform path
set(TUV_PLATFORM_PATH ${PLATFORM_NAME_L})

if(DEFINED TARGET_BOARD)
  if(${TARGET_BOARD} STREQUAL "mbedk64f")
    set(FLAGS_COMMON
          ${FLAGS_COMMON}
          "-mcpu=cortex-m4"
          "-mthumb"
          "-mlittle-endian"
          )
    if(NOT DEFINED TARGET_SYSTEMROOT)
      set(TARGET_SYSTEMROOT "${CMAKE_SOURCE_DIR}/targets/mbedk64f")
    endif()
  else()
    message(FATAL_ERROR "TARGET_BOARD=`${TARGET_BOARD}` is unknown to make")
  endif()
else()
  message(FATAL_ERROR "TARGET_BOARD is undefined")
endif()

# mbed port source/include
set(TARGET_INC ${TARGET_INC} "${TARGET_SYSTEMROOT}/source")


# build tester as library
set(BUILD_TEST_LIB "yes")
unset(BUILD_TEST_LIB CACHE)


# set copy libs to ${TARGET_SYSTEMROOT}/libtiv
file(MAKE_DIRECTORY "${TARGET_SYSTEMROOT}/libtuv")
set(COPY_TARGET_LIB "${TARGET_SYSTEMROOT}/libtuv/.")

# to help unittest to give host ip address
if(EXISTS ${RAW_TEST}/tuv_host_ipaddress.h)
  add_definitions("-D__TUV_HOST_IPEXIST__")
endif()

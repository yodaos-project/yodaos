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

cmake_minimum_required(VERSION 2.8.12)

# platform name in lower case
string(TOLOWER ${CMAKE_SYSTEM_NAME} PLATFORM_NAME_L)

set(PATH_ROOT    ${CMAKE_SOURCE_DIR})
set(SOURCE_ROOT  ${PATH_ROOT}/src)
set(INCLUDE_ROOT ${PATH_ROOT}/include)
set(TEST_ROOT    ${PATH_ROOT}/test)

if (DEFINED LIBTUV_CUSTOM_LIB_OUT)
  set(LIB_OUT ${LIBTUV_CUSTOM_LIB_OUT})
else()
  set(BIN_ROOT ${CMAKE_BINARY_DIR})
  set(LIB_OUT "${BIN_ROOT}/../lib")
  set(BIN_OUT "${BIN_ROOT}/../bin")
endif()

# path for platform depends files, use full name for default
# (e.g, i686-linux)
if (DEFINED TARGET_OS AND DEFINED TARGET_ARCH)
  string(TOLOWER "${TARGET_ARCH}-${TARGET_OS}" TARGET_PLATFORM_L)
  message(">>### ${TARGET_PLATFORM_L} <<")
  set(TUV_PLATFORM_PATH "${TARGET_PLATFORM_L}")
  include("cmake/option/option_${TARGET_PLATFORM_L}.cmake") 
endif()

string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE_L)
if (${BUILD_TYPE_L} STREQUAL "debug")
  set(FLAGS_COMMON "${FLAGS_COMMON} -DENABLE_DEBUG_LOG")
endif()

foreach(FLAG ${FLAGS_COMMON})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
endforeach()

# generate configure file
configure_file("${SOURCE_ROOT}/tuv__config.h.in"
               "${CMAKE_BINARY_DIR}/tuv__config.h")
configure_file("${SOURCE_ROOT}/tuv__config.h.in"
               "${INCLUDE_ROOT}/tuv__config.h")

message(STATUS "Build Type: [${CMAKE_BUILD_TYPE}]")

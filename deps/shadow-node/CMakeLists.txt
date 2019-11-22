# Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
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

cmake_minimum_required(VERSION 2.8)

project(IOTJS C)

set(IOTJS_VERSION_MAJOR 0)
set(IOTJS_VERSION_MINOR 11)
set(IOTJS_VERSION_PATCH 0)

# Do a few default checks
if(NOT DEFINED PLATFORM_DESCRIPTOR)
  message(FATAL_ERROR "No PLATFORM_DESCRIPTOR specified (format: <arch>-<os>)")
endif()

string(REPLACE "-" ";" PLATFORM_ARGS ${PLATFORM_DESCRIPTOR})
if(NOT DEFINED TARGET_OS)
  list(GET PLATFORM_ARGS 1 TARGET_OS)
  message(
    "TARGET_OS not specified, using '${TARGET_OS}' from PLATFORM_DESCRIPTOR")
endif()

if(NOT CMAKE_BUILD_TYPE)
  message("CMAKE_BUILD_TYPE was not set! Configuring for Debug build!")
  set(CMAKE_BUILD_TYPE Debug)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  message("CMAKE_BUILD_TYPE was set to Release, switching to MinSizeRel")
  set(CMAKE_BUILD_TYPE MinSizeRel)
endif()

if(NOT DEFINED BUILD_LIB_ONLY)
  set(BUILD_LIB_ONLY OFF)
endif()

if(NOT DEFINED ENABLE_SNAPSHOT)
  message("Snapshot mode force enabled")
  set(ENABLE_SNAPSHOT ON)
endif()

if(NOT DEFINED ENABLE_LTO)
  message("LTO force disabled")
  set(ENABLE_LTO OFF)
endif()

if(NOT DEFINED DISABLE_ES2015)
  set(FEATURE_PROFILE "es2015-subset")
endif()

set(FEATURE_PROFILE ${CMAKE_CURRENT_SOURCE_DIR}/profiles/jerry/default.profile)

macro(iotjs_add_flags VAR)
  foreach(_flag ${ARGN})
    set(${VAR} "${${VAR}} ${_flag}")
  endforeach()
endmacro()

macro(iotjs_add_compile_flags)
  iotjs_add_flags(CMAKE_C_FLAGS ${ARGV})
endmacro()

macro(iotjs_add_link_flags)
  iotjs_add_flags(IOTJS_LINKER_FLAGS ${ARGV})
endmacro()

# Add buildtype-related flags
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  iotjs_add_compile_flags(-DDEBUG -DENABLE_DEBUG_LOG)
endif()

# Add arch-dependant flags
if("${TARGET_ARCH}" STREQUAL "aarch64")
  iotjs_add_compile_flags(-DARM64 -fno-short-enums -mlittle-endian)
elseif("${TARGET_ARCH}" STREQUAL "arm" OR "${TARGET_ARCH}" STREQUAL "openwrt")
  iotjs_add_compile_flags(-D__arm__ -mthumb -fno-short-enums -mlittle-endian)
elseif("${TARGET_ARCH}" STREQUAL "i686")
  iotjs_add_compile_flags(-D__i686__ -D__x86__ -march=i686 -m32)
elseif("${TARGET_ARCH}" STREQUAL "x86_64")
  iotjs_add_compile_flags(-D__x86_64__)
else()
  message(WARNING "Unknown target arch: ${TARGET_ARCH}.")
endif()

# Add board-dependant flags
iotjs_add_compile_flags(-DTARGET_BOARD=${TARGET_BOARD})
iotjs_add_compile_flags(-fPIC)

if("${TARGET_BOARD}" STREQUAL "artik05x")
  iotjs_add_compile_flags(-mcpu=cortex-r4 -mfpu=vfp3)
elseif("${TARGET_BOARD}" STREQUAL "artik10")
  iotjs_add_compile_flags(-mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=softfp)
elseif("${TARGET_BOARD}" STREQUAL "rpi2")
  iotjs_add_compile_flags(-mcpu=cortex-a7 -mfpu=neon-vfpv4)
elseif("${TARGET_BOARD}" STREQUAL "stm32f4dis")
  iotjs_add_compile_flags(-mcpu=cortex-m4 -march=armv7e-m -mfpu=fpv4-sp-d16)
  iotjs_add_compile_flags(-mfloat-abi=hard)
endif()


string(TOUPPER "${TARGET_OS}" TARGET_OS)
# Add os-dependant flags
if(${TARGET_OS} STREQUAL "DARWIN")
  iotjs_add_compile_flags(-D__DARWIN__ -fno-builtin)
  iotjs_add_flags(EXTERNAL_LIBS dbus-1 z)
elseif(${TARGET_OS} STREQUAL "LINUX")
  iotjs_add_compile_flags(-D__LINUX__ -fno-builtin)
  iotjs_add_link_flags(-pthread)
  iotjs_add_flags(EXTERNAL_LIBS m rt dbus-1 z)
elseif(${TARGET_OS} STREQUAL "NUTTX")
  iotjs_add_compile_flags(-D__NUTTX__ -Os -fno-strict-aliasing)
  iotjs_add_compile_flags(-fno-strength-reduce -fomit-frame-pointer)
elseif(${TARGET_OS} STREQUAL "TIZEN")
  iotjs_add_compile_flags(-D__LINUX__ -fno-builtin)
  iotjs_add_link_flags(-pthread)
  iotjs_add_flags(EXTERNAL_LIBS m rt)
elseif(${TARGET_OS} STREQUAL "TIZENRT")
  iotjs_add_compile_flags(-D__TIZENRT__ -Os -fno-strict-aliasing)
  iotjs_add_compile_flags(-fno-strength-reduce -fomit-frame-pointer)
elseif(${TARGET_OS} STREQUAL "LINUX-ANDROID")
  iotjs_add_compile_flags(-D__LINUX__ -fno-builtin)
  iotjs_add_flags(EXTERNAL_LIBS m)
else()
  message(WARNING "Unknown target os: ${TARGET_OS}.")
endif()

# Add external options
if(DEFINED EXTERNAL_COMPILE_FLAGS)
  iotjs_add_compile_flags(${EXTERNAL_COMPILE_FLAGS})
endif()

if(DEFINED EXTERNAL_LINKER_FLAGS)
  iotjs_add_link_flags(${EXTERNAL_LINKER_FLAGS})
endif()

set(ROOT_DIR ${CMAKE_SOURCE_DIR})
set(ARCHIVE_DIR ${CMAKE_BINARY_DIR}/lib)

# if generates Xcode projects, /usr/local/lib shall be explicitly linked
link_directories(/usr/local/lib)

include(ExternalProject)

message("cmake compiler is ${CMAKE_C_COMPILER_ID}")
# Include external projects
include(cmake/jerry.cmake)
include(cmake/http-parser.cmake)
include(cmake/libtuv.cmake)
include(cmake/libmqtt.cmake)
include(cmake/mbedtls.cmake)
include(cmake/iotjs.cmake)

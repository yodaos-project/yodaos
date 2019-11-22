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

cmake_minimum_required(VERSION 2.8.12)

set(TUVTESTNAME "tuvtester")

add_definitions("-D__TUV_APIEMUL__")

if(DEFINED BUILD_TEST_LIB AND BUILD_TEST_LIB STREQUAL "yes")
  add_library(${TUVTESTNAME} ${TEST_MAINFILE} ${TEST_APIEMULFILES})
else()
  #add_executable(${TUVTESTNAME} ${TEST_MAINFILE} ${APIEMUL_FILES})
  message(FATAL_ERROR "BUILD_TEST_LIB must be 'yes'")
endif()


target_include_directories(${TUVTESTNAME} SYSTEM PRIVATE ${TARGET_INC})
target_include_directories(${TUVTESTNAME} PUBLIC ${LIB_TUV_INCDIRS}
                                                 ${TUV_TEST_INCDIRS})
target_link_libraries(${TUVTESTNAME} LINK_PUBLIC ${TARGETLIBNAME}
                      ${TUV_LINK_LIBS})
set_target_properties(${TUVTESTNAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUT}"
    LIBRARY_OUTPUT_DIRECTORY "${LIB_OUT}"
    RUNTIME_OUTPUT_DIRECTORY "${BIN_OUT}")

if(DEFINED BUILD_TEST_LIB AND BUILD_TEST_LIB STREQUAL "yes")
  if(DEFINED COPY_TARGET_LIB)
    add_custom_command(TARGET ${TUVTESTNAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TUVTESTNAME}>
                                        "${COPY_TARGET_LIB}"
        COMMENT "Copying lib${TUVTESTNAME} to ${COPY_TARGET_LIB}")
  endif()
endif()

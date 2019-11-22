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


set(TEST_PLATFORMFILES )

set(TUVTESTNAME "tuvtester")

foreach(FILES ${PLATFORM_TESTFILES})
  set(TEST_PLATFORMFILES ${TEST_PLATFORMFILES} ${FILES})
endforeach()

if(DEFINED BUILD_TEST_LIB AND BUILD_TEST_LIB STREQUAL "yes")
  add_library(${TUVTESTNAME} ${TEST_MAINFILE} ${TEST_UNITFILES}
              ${TEST_PLATFORMFILES})
else()
  add_executable(${TUVTESTNAME} ${TEST_MAINFILE} ${TEST_UNITFILES}
                 ${TEST_PLATFORMFILES})
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

if(DEFINED BUILD_HOST_HELPER AND BUILD_HOST_HELPER STREQUAL "yes")
  set(TUVHOSTHELPER "tuvhelper")
  add_executable(${TUVHOSTHELPER} ${TEST_MAINFILE} ${TEST_UNITFILES}
                 ${PLATFORM_HOSTHELPERFILES})

  target_include_directories(${TUVHOSTHELPER} SYSTEM PRIVATE ${TARGET_INC})
  target_include_directories(${TUVHOSTHELPER} PUBLIC ${LIB_TUV_INCDIRS})
  target_link_libraries(${TUVHOSTHELPER} LINK_PUBLIC ${TARGETLIBNAME}
                        ${TUV_LINK_LIBS})
  set_target_properties(${TUVHOSTHELPER} PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY "${LIB_OUT}"
      LIBRARY_OUTPUT_DIRECTORY "${LIB_OUT}"
      RUNTIME_OUTPUT_DIRECTORY "${BIN_OUT}")
endif()

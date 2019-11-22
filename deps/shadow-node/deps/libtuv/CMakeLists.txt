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
project(libtuv C)

set(LIBTUV_VERSION_MAJOR 0)
set(LIBTUV_VERSION_MINOR 1)

#
# + unix_common: unix like, posix, multi-tasking
#     + linux_common
#         + arm-linux
#         + i686-linux
#         + x86_64-linux
#     + arm-nuttx
# + (mtsk_common: multi-tasking
#     + (TBD)
# + raw_common: single task, big-while-loop, event driven
#     + arm-mbed
#
include(cmake/options.cmake)
include(cmake/libtuv.cmake)
if (DEFINED BUILDTESTER AND "${BUILDTESTER}" STREQUAL "yes")
  include(cmake/tuvtest.cmake)
elseif (DEFINED BUILDAPIEMULTESTER AND "${BUILDAPIEMULTESTER}" STREQUAL "yes")
  include(cmake/apiemultest.cmake)
endif()

unset(BUILDTESTER CACHE)
unset(BUILDAPIEMULTESTER CACHE)
unset(BUILD_TEST_LIB CACHE)
unset(BUILD_HOST_HELPER CACHE)

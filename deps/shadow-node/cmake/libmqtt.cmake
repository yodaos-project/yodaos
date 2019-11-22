# Copyright 2017-present Samsung Electronics Co., Ltd. and other contributors
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

set(DEPS_MQTT deps/libmqtt)
set(DEPS_MQTT_SRC ${ROOT_DIR}/${DEPS_MQTT})

ExternalProject_Add(libmqtt
  PREFIX ${DEPS_MQTT}
  SOURCE_DIR ${DEPS_MQTT_SRC}
  BUILD_IN_SOURCE 0
  BINARY_DIR ${DEPS_MQTT}
  CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}
    -DCMAKE_TOOLCHAIN_ROOT=${CMAKE_TOOLCHAIN_ROOT}
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
    -DENABLE_PROGRAMS=OFF
    -DENABLE_TESTING=OFF
)

add_library(libmqtt_packet STATIC IMPORTED)
add_dependencies(libmqtt_packet mqtt_packet)
set_property(TARGET libmqtt_packet PROPERTY
  IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/libmqtt_packet.a)
set_property(DIRECTORY APPEND PROPERTY
  ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/lib/libmqtt_packet.a)

set(MQTT_LIBS libmqtt_packet)
set(MQTT_INCLUDE_DIR ${DEPS_MQTT_SRC})

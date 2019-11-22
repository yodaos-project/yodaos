#*******************************************************************************
#  Copyright (c) 2017 IBM Corp.
#
#  All rights reserved. This program and the accompanying materials
#  are made available under the terms of the Eclipse Public License v1.0
#  and Eclipse Distribution License v1.0 which accompany this distribution.
#
#  The Eclipse Public License is available at
#     http://www.eclipse.org/legal/epl-v10.html
#  and the Eclipse Distribution License is available at
#    http://www.eclipse.org/org/documents/edl-v10.php.
#
#  Contributors:
#     Ian Craggs - initial version
#*******************************************************************************/

cmake_minimum_required(VERSION 2.8)
project(libmqtt)
set(CMAKE_BUILD_TYPE Debug)

message(STATUS "CMake version: " ${CMAKE_VERSION})
message(STATUS "CMake system name: " ${CMAKE_SYSTEM_NAME})

set(CMAKE_SCRIPTS "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

## build settings
set(LIBMQTT_VERSION_MAJOR 1)
set(LIBMQTT_VERSION_MINOR 0)
set(LIBMQTT_VERSION_PATCH 0)
set(CLIENT_VERSION ${LIBMQTT_VERSION_MAJOR}.${LIBMQTT_VERSION_MINOR}.${LIBMQTT_VERSION_PATCH})

string(TIMESTAMP BUILD_TIMESTAMP UTC)
message(STATUS "Timestamp is ${BUILD_TIMESTAMP}")

set(CPACK_PACKAGE_VERSION_MAJOR ${LIBMQTT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LIBMQTT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LIBMQTT_VERSION_PATCH})
include(CPack)

include_directories(./)
ADD_SUBDIRECTORY(MQTTPacket)

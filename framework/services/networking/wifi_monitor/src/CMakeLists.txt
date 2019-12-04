cmake_minimum_required(VERSION 2.8)
project(wifi_monitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")


set(COMMON_FLAGS "-include rokid/rokid_types.h")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
aux_source_directory(. DIR_SRCS)

add_executable(wifi_monitor ${DIR_SRCS})

target_link_libraries(wifi_monitor wpa_ctrl)
target_link_libraries(wifi_monitor rkamsex)
target_link_libraries(wifi_monitor json-c)

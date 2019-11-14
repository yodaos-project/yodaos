cmake_minimum_required(VERSION 2.8)
project(vold)

include(GNUInstallDirs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g \
  -DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=1 -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 \
  -DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=1  -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

message(STATUS "This is BINARY dir " ${CMAKE_C_FLAGS})

set(COMMON_FLAGS "-include rokid/rokid_types.h")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

aux_source_directory(./vold_manager.c DIR_SRCS)
add_executable(vold ./vold_manager.c)
target_link_libraries(vold recovery)

install(TARGETS vold DESTINATION bin)
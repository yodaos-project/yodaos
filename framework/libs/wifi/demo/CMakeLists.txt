cmake_minimum_required(VERSION 2.8)
project(wpa_ctrl_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(wpa_ctrl_test ./main.c)

target_link_libraries(wpa_ctrl_test wpa_ctrl)
target_link_libraries(wpa_ctrl_test property)
target_link_libraries(wpa_ctrl_test android_cutils)


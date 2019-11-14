
cmake_minimum_required(VERSION 2.8)
project(rtk_hciattach)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude -Wall -ggdb")

aux_source_directory(. DIR_SRCS)
add_executable(rtk_hciattach ${DIR_SRCS})
#target_link_libraries(rtk_hciattach)

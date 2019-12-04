cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -lrt -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -lrt -D_GNU_SOURCE")

project(eloop_mini)

set(ELOOP_LIB eloop_mini)
set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BUILD_DIR ${ROOT_DIR}/build/)

message("Root dir: " ${ROOT_DIR})
 
file(GLOB SOURCES src/*.c)
include_directories(${ROOT_DIR}/include)


add_library(${ELOOP_LIB} SHARED ${SOURCES})
set_target_properties(${ELOOP_LIB}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIR}
)

file(GLOB INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


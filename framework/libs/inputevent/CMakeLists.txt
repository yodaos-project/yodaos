cmake_minimum_required(VERSION 2.8)
project(input-event)

include(GNUInstallDirs)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude")

include_directories("./include")
file(GLOB INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/input-event)

add_subdirectory(src)
add_subdirectory(demo)


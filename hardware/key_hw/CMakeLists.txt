cmake_minimum_required(VERSION 2.8)
project(key_hw)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude -ggdb")

include_directories("./include")
link_directories(${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)

set(HEADER_FILES
    ./include/key_hw.h
)

install(FILES ${HEADER_FILES}
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/key_hw
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

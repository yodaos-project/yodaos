cmake_minimum_required(VERSION 2.8)
project(rokid_bt_service)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11" )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude" )

FILE(GLOB INCLUDE_FILES "include/*.h")
INSTALL(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/usr/include/bluetooth_service)

add_subdirectory(src)
add_subdirectory(demo)


install(FILES ${HEADER_FILES}
	DESTINATION
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

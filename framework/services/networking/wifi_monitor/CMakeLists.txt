cmake_minimum_required(VERSION 2.8)
project(rokid_wifi_monitor)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11 -lpthread" )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude  -lpthread")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


add_subdirectory(src)

install(FILES ${HEADER_FILES}
	DESTINATION
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

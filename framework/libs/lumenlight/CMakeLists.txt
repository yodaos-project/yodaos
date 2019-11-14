cmake_minimum_required(VERSION 3.0)
project(rklog)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

add_library(rklumen_light SHARED LumenLight.cpp LumenEffects.cpp)
target_link_libraries(rklumen_light android_hardware pthread)

install(TARGETS rklumen_light
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

#add_subdirectory(sample)

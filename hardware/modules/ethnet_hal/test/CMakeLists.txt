cmake_minimum_required(VERSION 2.8)
project(ethnet_test)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(ethnet_hal_test ${CMAKE_CURRENT_SOURCE_DIR}/ethnet_test.c)

target_link_libraries(ethnet_hal_test
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS ethnet_hal_test
	RUNTIME DESTINATION bin
)

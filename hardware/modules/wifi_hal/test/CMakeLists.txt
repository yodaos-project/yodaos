cmake_minimum_required(VERSION 2.8)
project(wifi_test)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(wifi_hal_test ${CMAKE_CURRENT_SOURCE_DIR}/wifi_test.c)

target_link_libraries(wifi_hal_test
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS wifi_hal_test
	RUNTIME DESTINATION bin
)

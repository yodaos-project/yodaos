cmake_minimum_required(VERSION 2.8)
project(led_test)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(led_test ${CMAKE_CURRENT_SOURCE_DIR}/led_test.c)

target_link_libraries(led_test
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS led_test
	RUNTIME DESTINATION bin
)

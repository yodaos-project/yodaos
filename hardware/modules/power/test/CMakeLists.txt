cmake_minimum_required(VERSION 2.8)
project(power_test)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(power_test ${CMAKE_CURRENT_SOURCE_DIR}/power_test.c)

target_link_libraries(power_test
  android_hardware ${ANDROID_CUTILS_LIBRARIES})

install(TARGETS power_test
	RUNTIME DESTINATION bin
)

cmake_minimum_required(VERSION 2.8)
project(bat_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(bat_test ./bat_test.c)

target_link_libraries(bat_test json-c)

add_executable(bat_monitor ./bat_monitor.c)

target_link_libraries(bat_monitor json-c)

add_executable(keytest ./key_test.c)

include(../common.mk)

findPackage(mutils REQUIRED
	HINTS ${mutilsPrefix}
	HEADERS rlog.h caps.h clargs.h
	INC_PATH_SUFFIX include/log include/caps include/misc
	SHARED_LIBS rlog caps
)

findPackage(flora REQUIRED
	HINTS ${floraPrefix}
	HEADERS flora-cli.h
	SHARED_LIBS flora-cli
)

target_include_directories(bat_test PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bat_test
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

target_include_directories(bat_monitor PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bat_monitor
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)


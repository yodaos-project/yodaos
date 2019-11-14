cmake_minimum_required(VERSION 2.8)
project(pms_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_executable(pms_test ./pms_test.c)

target_link_libraries(pms_test json-c rklog)

add_executable(pms_monitor ./pms_monitor.c)

target_link_libraries(pms_monitor json-c rklog)

add_executable(rokid_reboot ./rokid_reboot.c)

target_link_libraries(rokid_reboot rklog)

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

target_include_directories(pms_test PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(pms_test
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

target_include_directories(pms_monitor PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(pms_monitor
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)


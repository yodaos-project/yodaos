cmake_minimum_required(VERSION 2.8)
project(powermanager_service)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -I..   -Iinclude -I ../include -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE -lpthread")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -I..   -Iinclude -I ../include -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE -lpthread")

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

aux_source_directory(. DIR_SRCS)

add_executable(powermanager_service ${DIR_SRCS} ${SRC_FILES})
target_include_directories(powermanager_service PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(powermanager_service
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

target_link_libraries(powermanager_service android_hardware)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

target_link_libraries(powermanager_service json-c rklog)
target_link_libraries(powermanager_service android_cutils)
target_link_libraries(powermanager_service eloop_mini)

cmake_minimum_required(VERSION 2.8)
project(bluetooth_service)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -I ../include -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE -lpthread")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude -I ../include -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE -lpthread")

aux_source_directory(. DIR_SRCS)
aux_source_directory(./bt/. DIR_SRCS)
aux_source_directory(./mic/. DIR_SRCS)

include_directories("./mic")

if(BT_SERVICE_IPC_BY_ZEROMQ)
ADD_DEFINITIONS(-DBT_SERVICE_IPC_BY_ZEROMQ)
set(SRC_FILES
  ./ipc/ipc.c
  ./ipc/zeromq.c
  )

add_executable(bluetooth_service ${DIR_SRCS} ${SRC_FILES})
target_link_libraries(bluetooth_service zeromq_handle)
elseif(BT_SERVICE_IPC_BY_FLORA)
include(../common.mk)

ADD_DEFINITIONS(-DBT_SERVICE_IPC_BY_FLORA)
findPackage(mutils REQUIRED
	HINTS ${mutilsPrefix}
	HEADERS rlog.h caps.h clargs.h
	INC_PATH_SUFFIX include/log include/caps include/misc
	SHARED_LIBS rlog caps
	STATIC_LIBS misc
)

findPackage(flora REQUIRED
	HINTS ${floraPrefix}
	HEADERS flora-agent.h
	SHARED_LIBS flora-cli
)
set(SRC_FILES
  ./ipc/ipc.c
  ./ipc/flora.c
  )

add_executable(bluetooth_service ${DIR_SRCS} ${SRC_FILES})

target_include_directories(bluetooth_service PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bluetooth_service
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)
endif()

if(ROKID_YODAOS)
ADD_DEFINITIONS(-DROKID_YODAOS)
endif()
if(BT_SERVICE_HAVE_HFP)
ADD_DEFINITIONS(-DBT_SERVICE_HAVE_HFP)
endif()

target_link_libraries(bluetooth_service json-c rklog)
target_link_libraries(bluetooth_service eloop_mini)
target_link_libraries(bluetooth_service android_cutils)
target_link_libraries(bluetooth_service android_hardware)

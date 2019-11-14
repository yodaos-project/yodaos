cmake_minimum_required(VERSION 2.8)
project(bt_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g  -Wall -Wno-unused-parameter")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -Wall -Wno-unused-parameter")



include_directories("../include")

if(BT_SERVICE_USE_K18_HW_OPUS)
ADD_DEFINITIONS(-DUSE_K18_HW_OPUS)
elseif(BT_SERVICE_USE_SW_OPUS)
ADD_DEFINITIONS(-DUSE_SW_OPUS)
endif()


add_executable(bt_service_test ./bluetooth_test.c)

target_link_libraries(bt_service_test json-c)

add_executable(bt_atvv ./bluetooth_atvv_test.c ./adpcm_decoder.c)

target_link_libraries(bt_atvv json-c avcodec avformat swresample avutil avdevice)
if(BT_SERVICE_USE_K18_HW_OPUS)
target_link_libraries(bt_atvv opus.gx.hw)
elseif(BT_SERVICE_USE_SW_OPUS)
target_link_libraries(bt_atvv opus)
endif()

add_executable(bt_monitor ./bluetooth_monitor.c)

target_link_libraries(bt_monitor json-c)


if(BT_SERVICE_IPC_BY_ZEROMQ)
target_link_libraries(bt_monitor zeromq_handle)
target_link_libraries(bt_service_test zeromq_handle)
ADD_DEFINITIONS(-DBT_SERVICE_IPC_BY_ZEROMQ)

elseif(BT_SERVICE_IPC_BY_FLORA)
include(../common.mk)

findPackage(mutils REQUIRED
	HINTS ${mutilsPrefix}
	HEADERS rlog.h caps.h clargs.h
	INC_PATH_SUFFIX include/log include/caps include/misc
	SHARED_LIBS rlog caps
	STATIC_LIBS misc
)

findPackage(flora REQUIRED
	HINTS ${floraPrefix}
	HEADERS flora-cli.h flora-agent.h
	SHARED_LIBS flora-cli
)

ADD_DEFINITIONS(-DBT_SERVICE_IPC_BY_FLORA)

target_include_directories(bt_service_test PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bt_service_test
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

target_include_directories(bt_atvv PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bt_atvv
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

target_include_directories(bt_monitor PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)
target_link_libraries(bt_monitor
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

endif()



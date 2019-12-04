cmake_minimum_required(VERSION 2.8)
project(net_manager)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -I../include  -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -Werror -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE")

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -I../include  -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Werror -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -I../include  -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -lrt -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(COMMON_FLAGS "-include rokid/rokid_types.h")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(../common.mk)

# aux_source_directory(. DIR_SRCS)
set(SRC_FILES
  ./main.c
  ./ipc/ipc.c
  ./ipc/flora_upload.c
  ./net/network_monitor.c
  ./net/network_ctrl.c
  ./net/network_report.c
  ./net/network_switch.c
  ./net/network_mode.c
  ./net/network_sched.c
  ./wifi/wifi_event.c
  ./wifi/wifi_hal.c
  ./modem/modem_hal.c
  ./modem/modem_event.c
  ./etherent/etherent_hal.c
  ./etherent/etherent_event.c
)
add_executable(net_manager ${SRC_FILES})

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

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
target_link_libraries(net_manager android_hardware)

target_link_libraries(net_manager rklog)
target_link_libraries(net_manager pthread)
target_link_libraries(net_manager wpa_ctrl)
target_link_libraries(net_manager property)
target_link_libraries(net_manager json-c)
target_link_libraries(net_manager android_cutils)
target_link_libraries(net_manager eloop_mini)
target_link_libraries(net_manager wpa_client)

target_include_directories(net_manager PRIVATE
     ${mutils_INCLUDE_DIRS}
     ${flora_INCLUDE_DIRS}
)

target_link_libraries(net_manager
     ${mutils_LIBRARIES}
     ${flora_LIBRARIES}
)

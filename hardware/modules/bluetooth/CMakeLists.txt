
cmake_minimum_required(VERSION 2.8)

if(BUILD_BLUETOOTH_BSA_AMPAK)
project(libbluetooth.bsa_ampak)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -DBUILDCFG -Iinclude -Ibsa_ampak -Wall -ggdb -DAPP_TRACE_NODEBUG -Ibsa_ampak/linux/app_common/include")

include_directories("./include")
include_directories("./bsa_ampak")
include_directories("./bsa_ampak/linux/app_common/include")

#link_directories(${CMAKE_SOURCE_DIR}/bsa_ampak)

set(SRC_FILES
    ./bluetooth.c
    ./bsa_ampak/bsa_common.c
    ./bsa_ampak/app_manager.h
    ./bsa_ampak/app_manager.c
    ./bsa_ampak/ble.c
    ./bsa_ampak/ble_server.h
    ./bsa_ampak/ble_server.c
    ./bsa_ampak/a2dp.h
    ./bsa_ampak/a2dp.c
    ./bsa_ampak/mypulse.h
    ./bsa_ampak/mypulse.c
    ./bsa_ampak/aac_latm_decoder.c
    ./bsa_ampak/aac_latm_decoder.h
    ./bsa_ampak/a2dp_sink.h
    ./bsa_ampak/a2dp_sink.c
    ./bsa_ampak/hfp_hs.c
    ./bsa_ampak/hid_host.c
    ./bsa_ampak/app_bthid.c
    ./bsa_ampak/app_hh_db.c
    ./bsa_ampak/app_hh_xml.c
    ./bsa_ampak/app_ble_client_xml.c
    ./bsa_ampak/app_ble_client_db.c
    ./bsa_ampak/ble_client.c
    ./bsa_ampak/linux/app_common/include/app_3dtv.h
    ./bsa_ampak/linux/app_common/include/app_utils.h
    ./bsa_ampak/linux/app_common/include/app_xml_utils.h
    ./bsa_ampak/linux/app_common/include/app_xml_param.h
    ./bsa_ampak/linux/app_common/include/nanoxml.h
    ./bsa_ampak/linux/app_common/include/app_dm.h
    ./bsa_ampak/linux/app_common/include/app_mgt.h
    ./bsa_ampak/linux/app_common/include/app_services.h
    ./bsa_ampak/linux/app_common/include/app_disc.h
    ./bsa_ampak/linux/app_common/include/app_sec.h
    ./bsa_ampak/linux/app_common/include/app_link.h
    ./bsa_ampak/linux/app_common/include/app_thread.h
    ./bsa_ampak/linux/app_common/include/app_op_fmt.h
    ./bsa_ampak/linux/app_common/source/app_utils.c
    ./bsa_ampak/linux/app_common/source/app_xml_utils.c
    ./bsa_ampak/linux/app_common/source/app_xml_param.c
    ./bsa_ampak/linux/app_common/source/app_dm.c
    ./bsa_ampak/linux/app_common/source/app_mgt.c
    ./bsa_ampak/linux/app_common/source/nanoxml.c
    ./bsa_ampak/linux/app_common/source/app_services.c
    ./bsa_ampak/linux/app_common/source/app_disc.c
    ./bsa_ampak/linux/app_common/source/app_sec.c
    ./bsa_ampak/linux/app_common/source/app_link.c
    ./bsa_ampak/linux/app_common/source/app_thread.c
    ./bsa_ampak/linux/app_common/source/app_3dtv.c
    ./bsa_ampak/linux/app_common/source/app_op_fmt.c
    )

add_library(bluetooth.default SHARED ${SRC_FILES})

target_link_libraries(bluetooth.default bsa android_cutils pulse pulse-simple pulse-mainloop-glib)
target_link_libraries(bluetooth.default pthread avcodec avformat asound rklog)
INSTALL( TARGETS bluetooth.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_subdirectory(test)

elseif(BUILD_BLUETOOTH_BSA_CYPRESS)
project(libbluetooth.bsa_cypress)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -DBUILDCFG -Iinclude -Wall -ggdb -DAPP_TRACE_NODEBUG -Ibsa_cypress/linux/app_common/include")

include_directories("./include")
include_directories("./bsa_cypress")
include_directories("./bsa_cypress/linux/app_common/include")

#link_directories(${CMAKE_SOURCE_DIR}/bsa_cypress)

set(SRC_FILES
    ./bluetooth.c
    ./bsa_cypress/bsa_common.c
    ./bsa_cypress/app_manager.h
    ./bsa_cypress/app_manager.c
    ./bsa_cypress/ble.c
    ./bsa_cypress/ble_server.h
    ./bsa_cypress/ble_server.c
    ./bsa_cypress/a2dp.h
    ./bsa_cypress/a2dp.c
    ./bsa_cypress/mypulse.h
    ./bsa_cypress/mypulse.c
    ./bsa_cypress/aac_latm_decoder.c
    ./bsa_cypress/aac_latm_decoder.h
    ./bsa_cypress/a2dp_sink.h
    ./bsa_cypress/a2dp_sink.c
    ./bsa_cypress/hfp_hs.c
    ./bsa_cypress/hid_host.c
    ./bsa_cypress/app_bthid.c
    ./bsa_cypress/app_hh_db.c
    ./bsa_cypress/app_hh_xml.c
    ./bsa_cypress/app_ble_client_xml.c
    ./bsa_cypress/app_ble_client_db.c
    ./bsa_cypress/ble_client.c
    ./bsa_cypress/linux/app_common/include/app_3dtv.h
    ./bsa_cypress/linux/app_common/include/app_utils.h
    ./bsa_cypress/linux/app_common/include/app_xml_utils.h
    ./bsa_cypress/linux/app_common/include/app_xml_param.h
    ./bsa_cypress/linux/app_common/include/nanoxml.h
    ./bsa_cypress/linux/app_common/include/app_dm.h
    ./bsa_cypress/linux/app_common/include/app_mgt.h
    ./bsa_cypress/linux/app_common/include/app_services.h
    ./bsa_cypress/linux/app_common/include/app_disc.h
    ./bsa_cypress/linux/app_common/include/app_sec.h
    ./bsa_cypress/linux/app_common/include/app_link.h
    ./bsa_cypress/linux/app_common/include/app_thread.h
    ./bsa_cypress/linux/app_common/include/app_op_fmt.h
    ./bsa_cypress/linux/app_common/source/app_utils.c
    ./bsa_cypress/linux/app_common/source/app_xml_utils.c
    ./bsa_cypress/linux/app_common/source/app_xml_param.c
    ./bsa_cypress/linux/app_common/source/app_dm.c
    ./bsa_cypress/linux/app_common/source/app_mgt.c
    ./bsa_cypress/linux/app_common/source/nanoxml.c
    ./bsa_cypress/linux/app_common/source/app_services.c
    ./bsa_cypress/linux/app_common/source/app_disc.c
    ./bsa_cypress/linux/app_common/source/app_sec.c
    ./bsa_cypress/linux/app_common/source/app_link.c
    ./bsa_cypress/linux/app_common/source/app_thread.c
    ./bsa_cypress/linux/app_common/source/app_3dtv.c
    ./bsa_cypress/linux/app_common/source/app_op_fmt.c
    )

add_library(bluetooth.default SHARED ${SRC_FILES})


target_link_libraries(bluetooth.default bsa android_cutils pulse pulse-simple pulse-mainloop-glib)
target_link_libraries(bluetooth.default pthread avcodec avformat asound rklog)
INSTALL( TARGETS bluetooth.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_subdirectory(test)
elseif(BUILD_BLUETOOTH_BLUEZ)
project(libbluetooth.bluez)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -I./ -Iinclude -Wall -g -ggdb -Wno-unused-function" )
include(GNUInstallDirs)
include_directories("./include")
include_directories("./bluez/")
set(SRC_FILES
    ./bluetooth.c
    ./bluez/bluez_common.c
    ./bluez/app_common.c
    ./bluez/app_manager.c
    ./bluez/ble_server.c
    ./bluez/a2dp_sink.c
    ./bluez/a2dp.c
    ./bluez/lib/uuid.c
    ./bluez/shared/io-glib.c
    ./bluez/shared/queue.c
    ./bluez/shared/util.c
    ./bluez/shared/mgmt.c
    ./bluez/shared/ad.c
    
    )
file(GLOB COMMON_FILES "bluez/gdbus/*.c")
file(GLOB CLIENT_FILES "bluez/client/*.c")

add_library(bluetooth.default SHARED ${SRC_FILES} ${COMMON_FILES} ${CLIENT_FILES})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
target_link_libraries(bluetooth.default ${GLIB2_LIBRARIES})
include_directories(bluetooth.default PUBLIC ${GLIB2_INCLUDE_DIRS})

pkg_check_modules(DBUS REQUIRED dbus-1)
target_link_libraries(bluetooth.default ${DBUS_LIBRARIES})
include_directories(bluetooth.default PUBLIC ${DBUS_INCLUDE_DIRS})

target_link_libraries(bluetooth.default pthread bluetooth glib-2.0 dbus-1 readline ncurses)

target_link_libraries(bluetooth.default pulse pulse-simple pulse-mainloop-glib android_cutils rklog)
INSTALL( TARGETS bluetooth.default ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

add_subdirectory(test)
endif()


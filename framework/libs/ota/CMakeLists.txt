cmake_minimum_required(VERSION 2.8)
project(unpack)

INCLUDE(GNUInstallDirs)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11 -O2 -g \
  -DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=1 \
  -DADB_HOST=0 -Wall -Wno-unused-parameter \
  -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude -O2 -g -DADB_HOST=0 \
  -DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=1 \
  -Wall -Wno-unused-parameter \
  -D_XOPEN_SOURCE -D_GNU_SOURCE")

MESSAGE(STATUS "This is BINARY dir " ${CMAKE_C_FLAGS})

set(COMMON_FLAGS "-include rokid/rokid_types.h")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(OTA_AUTH)
	ADD_DEFINITIONS(-DOTA_AUTH)
endif()

aux_source_directory(./unpack.c DIR_SRCS)
aux_source_directory(./ota_unpack.c DIR_SRCS)
aux_source_directory(./recovery/*.c DIR_SRCS)
aux_source_directory(./sha1.c DIR_SRCS)

add_executable(unpack ./ota_unpack.c)
add_executable(unpack_cli ./demo/ota_demo.c)

add_library(recovery SHARED ./recovery/recovery.c)
add_library(libunpack SHARED ./unpack.c ./sha1.c)

target_link_libraries(libunpack recovery)
target_link_libraries(unpack recovery libunpack)
target_link_libraries(unpack_cli recovery libunpack)

install(TARGETS recovery ${INSTALL_EXPORT}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/recovery/recovery.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/recovery/recovery)

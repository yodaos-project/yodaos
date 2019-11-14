cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

INCLUDE(GNUInstallDirs)

aux_source_directory(. DIR_SRCS)
add_library(wpa_ctrl SHARED ${DIR_SRCS})
target_link_libraries(wpa_ctrl wpa_client)
target_link_libraries(wpa_ctrl curl)
target_link_libraries(wpa_ctrl property)
target_link_libraries(wpa_ctrl android_cutils)

file(GLOB INCLUDE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_subdirectory(demo)

install( TARGETS wpa_ctrl ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

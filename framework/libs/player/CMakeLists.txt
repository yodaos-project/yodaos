cmake_minimum_required(VERSION 3.0.2)
project(media)

include(GNUInstallDirs)
include_directories(./include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb" )
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

add_subdirectory(src)
add_subdirectory(sample)
# add_subdirectory(test)

set(HEADER_FILES
  ./include/Utils.h
  ./include/MediaPlayer.h
  ./include/WavPlayer.h
  ./include/SimplePlayer.h
  ./include/OpusPlayer.h
  ./include/OpusCodec.h
)

install(FILES ${HEADER_FILES}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/librplayer
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)

cmake_minimum_required(VERSION 2.8)
project(property-service)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALLOW_LOCAL_PROP_OVERRIDE=1 -DALLOW_DISABLE_SELINUX=1 -I.. -Iinclude -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALLOW_LOCAL_PROP_OVERRIDE=1 -DALLOW_DISABLE_SELINUX=1 -I.. -Iinclude")

aux_source_directory(./src DIR_SRCS)

add_executable(property_service ${DIR_SRCS})
target_link_libraries(property_service property android_cutils android_utils)
#add_subdirectory(demo)
add_subdirectory(tools)

INCLUDE(GNUInstallDirs)

INSTALL( TARGETS property_service ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

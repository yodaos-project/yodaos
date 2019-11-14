cmake_minimum_required(VERSION 2.8)
project(proptools)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude")


add_executable(setprop setprop.c)
target_link_libraries(setprop android_cutils)

add_executable(getprop getprop.c)
target_link_libraries(getprop android_cutils)

INCLUDE(GNUInstallDirs)

INSTALL( TARGETS setprop ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL( TARGETS getprop ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

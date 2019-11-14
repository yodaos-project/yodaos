
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

project(property)
enable_testing()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I.. -Iinclude -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I.. -Iinclude")

include(GNUInstallDirs)

aux_source_directory(./src DIR_SRCS)
add_library(property SHARED ${DIR_SRCS})

install( TARGETS property ${INSTALL_EXPORT}
     RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
     LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
     ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB INCLUDE_FILES "include/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/property)

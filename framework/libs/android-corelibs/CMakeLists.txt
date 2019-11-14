cmake_minimum_required(VERSION 3.0)
project(android-corelibs)

include(GNUInstallDirs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(libcutils)
add_subdirectory(libutils)

file(GLOB INCLUDE_FILES "include/cutils/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cutils)

file(GLOB INCLUDE_FILES "include/utils/*.h")
install(FILES ${INCLUDE_FILES} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utils)

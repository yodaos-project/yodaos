cmake_minimum_required(VERSION 2.8)
project(android-headers)

set(VERSION 0.1.0)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(COMMON_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/include/private/rokid_types.h")

include(TargetArch)
target_architecture(DETECTED_ARCH)

if (DETECTED_ARCH MATCHES "armv8")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
elseif (DETECTED_ARCH MATCHES "arm.*")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -DBINDER_IPC_32BIT")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -DBINDER_IPC_32BIT")
endif()

include(GNUInstallDirs)
find_package(AndroidCUtils REQUIRED)
find_package(AndroidUtils REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_subdirectory(cmds/fw_env)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/android" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/private/rokid_types.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rokid)

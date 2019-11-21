CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(rokid_utils)
set(VERSION 0.1)

# CMake policies: enable MACOSX_RPATH by default
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
  set(COMPILER_IS_CLANG true)
else()
  set(COMPILER_IS_CLANG false)
endif()

option(BUILD_DEBUG "debug or release" OFF)
option(BUILD_DEMO "build demo" OFF)

if (BUILD_DEBUG)
  set (common_cflags "-g -O0")
  set (common_cxxflags "-g -O0")
else()
  set (common_cflags -O3)
  set (common_cxxflags -O3)
endif()
if (NOT ${COMPILER_IS_CLANG})
  set (common_ldflags "${common_ldflags} -pthread")
endif()
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${common_cflags}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${common_cxxflags}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${common_ldflags}")
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${common_ldflags}")

# target 'misc'
file(GLOB misc_src_files
  include/misc/*.h
  src/misc/*.cc
)
add_library(misc SHARED
  ${misc_src_files}
)
target_include_directories(misc PUBLIC
  include/misc
)
add_library(misc_static STATIC
  ${misc_src_files}
)
set_target_properties(misc_static PROPERTIES
  OUTPUT_NAME misc
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(misc_static PUBLIC
  include/misc
)
install(DIRECTORY include/misc
  DESTINATION include
)

# rlog src files
file (GLOB_RECURSE rlog_src_files
  src/log/*.h
  src/log/*.cc
  include/log/*.h
)

add_library (rlog SHARED
  ${rlog_src_files}
)
target_include_directories (rlog PUBLIC
  include/log
  include/misc
)
target_link_libraries (rlog
  misc
)
add_library (rlog_static STATIC
  ${rlog_src_files}
)
set_target_properties(rlog_static PROPERTIES
  OUTPUT_NAME rlog
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories (rlog_static PUBLIC
  include/log
  include/misc
)
target_link_libraries (rlog_static
  misc
)
install (DIRECTORY include/log
  DESTINATION include
)

# target 'caps'
file(GLOB caps_src_files
  include/caps/*.h
  src/caps/*.cc
)
add_library(caps SHARED
  ${caps_src_files}
)
target_include_directories(caps PUBLIC
  include/caps
)
add_library(caps_static STATIC
  ${caps_src_files}
)
set_target_properties(caps_static PROPERTIES
  OUTPUT_NAME caps
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(caps_static PUBLIC
  include/caps
)
install(DIRECTORY include/caps
  DESTINATION include
)

# install targets
install (TARGETS rlog misc caps rlog_static misc_static caps_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
# build demo
if (BUILD_DEMO)
include(${CUSTOM_CMAKE_MODULES}/common.mk)
findPackage(gtest REQUIRED
  HINTS ${gtestPrefix}
  HEADERS gtest/gtest.h
  STATIC_LIBS gtest
)
set(clargs_demo_src_files
  demo/misc/clargs_demo.cc
)
add_executable (clargs_demo ${clargs_demo_src_files})
target_link_libraries (clargs_demo
  misc
)
if (COMPILER_IS_CLANG)
  target_compile_options(clargs_demo PRIVATE -Wno-writable-strings)
else()
  target_compile_options(clargs_demo PRIVATE -Wno-write-strings)
endif()

add_executable (xmopt-demo demo/misc/xmopt-demo.cc)
target_include_directories(xmopt-demo PRIVATE include/misc)

set(vq_demo_src_files
  demo/misc/vq_demo.cc
)
add_executable (vq_demo ${vq_demo_src_files})
target_link_libraries (vq_demo
  misc
)

set(uri_demo_src_files
  demo/misc/uri_demo.cc
)
add_executable (uri_demo ${uri_demo_src_files})
target_link_libraries (uri_demo
  misc
)

set(caps_demo_src_files
  demo/caps/caps_demo.cc
  demo/caps/random_caps_factory.cc
  demo/caps/random_caps_factory.h
  demo/caps/demo_defs.h
)
add_executable (caps_demo ${caps_demo_src_files})
target_link_libraries (caps_demo
  caps
  misc
)
set(caps_size_src_files
  demo/caps/caps_size.cc
)
add_executable(caps_size ${caps_size_src_files})
target_link_libraries(caps_size
  caps
)
set(rlog_demo_src_files
  demo/log/rlog_demo.cc
)
add_executable(rlog_demo ${rlog_demo_src_files})
target_link_libraries(rlog_demo
  rlog
)
set(tcp_rlogcat_src_files
  demo/log/tcp-rlogcat.cc
)
add_executable(tcp-rlogcat ${tcp_rlogcat_src_files})
add_executable(heapsort-demo
  demo/misc/heapsort_demo.cc
)
target_include_directories(heapsort-demo PRIVATE
  include/misc
)
add_executable(thr-pool-demo demo/misc/thr-pool-demo.cc)
target_include_directories(thr-pool-demo PRIVATE
  include/misc
)
add_library(global-error1 SHARED tests/misc/global-error-shared1.cpp)
target_include_directories(global-error1 PRIVATE include/misc)
target_link_libraries(global-error1 misc_static)
add_library(global-error2 SHARED tests/misc/global-error-shared2.cpp)
target_include_directories(global-error2 PRIVATE include/misc)
target_link_libraries(global-error2 misc_static)
add_executable(tests
  tests/main.cpp
  tests/misc/test-circle-stream.cpp
  tests/misc/test-thr-pool.cpp
  tests/misc/test-global-error.cpp
)
target_include_directories(tests PRIVATE
  include/misc
  ${gtest_INCLUDE_DIRS}
)
target_link_libraries(tests
  ${gtest_LIBRARIES}
  global-error1
  global-error2
)
endif(BUILD_DEMO)

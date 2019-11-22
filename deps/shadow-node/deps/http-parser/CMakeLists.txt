cmake_minimum_required(VERSION 2.8)

PROJECT(http-parser C)

set(targetName httpparser)

INCLUDE_DIRECTORIES(./)

if(${OS} MATCHES "NUTTX|TIZENRT")
  set(TARGET_INCLUDE "${NUTTX_HOME}/include")
  set(TARGET_INCLUDE ${TARGET_INCLUDE} "${NUTTX_HOME}/include/cxx")
  INCLUDE_DIRECTORIES(./ ${TARGET_INCLUDE})
endif()


if("${CMAKE_BUILD_TYPE}" MATCHES "Release|MinSizeRel")
  # If we are building for release modify it to MinSizeRel for -Os option.
  set(CMAKE_BUILD_TYPE "MinSizeRel")
else()
  # Any other case just assume debug build.
  set(CMAKE_BUILD_TYPE "Debug")
endif()

message("HTTP Parser configured with:")
message(STATUS "CMAKE_BUILD_TYPE          ${CMAKE_BUILD_TYPE}")
message(STATUS "OS                        ${OS}")
message(STATUS "NUTTX_HOME                ${NUTTX_HOME}")
message(STATUS "ENABLE_MEMORY_CONSTRAINTS ${ENABLE_MEMORY_CONSTRAINTS}")

if(ENABLE_MEMORY_CONSTRAINTS)
  set(DEFINES_HTTPPARSER ${DEFINES_HTTPPARSER} HTTPPARSER_ENABLE_MEMORY_CONSTRAINTS)
endif()

ADD_LIBRARY(${targetName} STATIC http_parser.c)
target_compile_definitions(${targetName} PRIVATE ${DEFINES_HTTPPARSER})
install(TARGETS ${targetName} DESTINATION ./lib)

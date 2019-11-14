cmake_minimum_required(VERSION 3.0)
project(httpsession)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(include)

add_library(httpsession SHARED src/lib/HttpSession.cpp)
target_link_libraries(httpsession curl pthread)

add_executable(httpsession_cli src/cli/main.cpp)
target_link_libraries(httpsession_cli httpsession)

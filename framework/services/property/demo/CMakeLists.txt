cmake_minimum_required(VERSION 2.8)
project(property_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DALLOW_LOCAL_PROP_OVERRIDE=1 -DALLOW_DISABLE_SELINUX=1 -I.. -Iinclude -std=c++11" )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DALLOW_LOCAL_PROP_OVERRIDE=1 -DALLOW_DISABLE_SELINUX=1 -I.. -Iinclude")


add_executable(property_test demo.cpp)
target_link_libraries(property_test property android_cutils)
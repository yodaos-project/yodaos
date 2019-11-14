cmake_minimum_required(VERSION 3.0)
project(raspberry_yodaos_product CXX)
include(ExternalProject)

install(DIRECTORY ./res/media       DESTINATION ${CMAKE_INSTALL_DIR}/opt)
install(DIRECTORY ./res/light       DESTINATION ${CMAKE_INSTALL_DIR}/opt)

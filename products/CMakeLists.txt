cmake_minimum_required(VERSION 3.0)
project(products CXX)
include(ExternalProject)
message("Product Name: ${PRODUCT_NAME}")
message("Product Pathname: ${PRODUCT_PATHNAME}")
message("Target Platform: ${TARGET_PLATFORM}")
message("Install Directory: ${CMAKE_INSTALL_DIR}")
message("Include Directory: ${CMAKE_INCLUDE_DIR}")
if (PRODUCT_NAME AND PRODUCT_PATHNAME)
	ExternalProject_Add(${PRODUCT_NAME}
		SOURCE_DIR ${PRODUCT_PATHNAME}
		CMAKE_ARGS
			-DTARGET_PLATFORM=${TARGET_PLATFORM}
			-DCMAKE_INSTALL_DIR=${CMAKE_INSTALL_DIR}
			-DCMAKE_INCLUDE_DIR=${CMAKE_INCLUDE_DIR}
			-DPRODUCT_NAME=${PRODUCT_NAME})
endif()


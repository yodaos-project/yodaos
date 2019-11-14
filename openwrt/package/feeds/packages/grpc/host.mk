TOP_DIR ?= $(abspath ../..)
INCLUDE_DIR := $(TOP_DIR)/include/make

include $(INCLUDE_DIR)/defaults.mk

PKG_VERSION_FILE := config.mk
include $(PKG_VERSION_FILE)
QSDK_PKG_DIR := $(PLATFORM_BSB002_BUILD_DIR)/$(PKG_NAME)
PACKAGE_DEPS := protobuf
REQUIRED_EXECUTABLES += cmake make cc

include $(INCLUDE_DIR)/utils.mk
include $(INCLUDE_DIR)/buildReqs.mk
include $(INCLUDE_DIR)/dependencies.mk
include $(INCLUDE_DIR)/extract.mk
include $(INCLUDE_DIR)/package/cmake.mk

CMAKE_OPTIONS += \
  -DgRPC_ZLIB_PROVIDER=package \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_PROTOBUF_PROVIDER=package \
  -DgRPC_GFLAGS_PROVIDER=package \
  -DgRPC_BENCHMARK_PROVIDER=package \
  -DBUILD_SHARED_LIBS=ON \
  -D_gRPC_PROTOBUF_LIBRARIES=protobuf \
  -D_gRPC_PROTOBUF_PROTOC_LIBRARIES=protoc \
  -D_gRPC_PROTOBUF_PROTOC=$(STAGING_DIR)/usr/local/bin/protoc \
  -DCMAKE_DISABLE_FIND_PACKAGE_protobuf=TRUE \
  -DCMAKE_DISABLE_FIND_PACKAGE_gflags=TRUE \
  -DCMAKE_DISABLE_FIND_PACKAGE_benchmark=TRUE \
  -DPROTOBUF_ROOT_DIR=${STAGING_DIR}/usr/local \
  -DBORINGSSL_ROOT_DIR=${STAGING_DIR}/usr/local \

#
# gRPC Makefile for openwrt
#

include $(TOPDIR)/rules.mk
PKG_NAME:=grpc

include config.mk
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/yorkie/grpc.git

GRPC_TARGETS:=grpc++_unsecure
GRPC_HOST_TARGETS:=grpc_cpp_plugin

HOST_BUILD_DEPENDS:=protobuf/host
PKG_BUILD_DEPENDS:=grpc/host protobuf/host

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk
include $(INCLUDE_DIR)/host-build.mk

HOST_LDFLAGS += -L$(STAGING_DIR_HOST)/lib 

CMAKE_OPTIONS += \
  -DTHREADS_PTHREAD_ARG=true \
  -DGO_EXECUTABLE=echo \
  -DGRPC_CROSS_COMPILE=true \
  -DgRPC_ZLIB_PROVIDER=package \
  -DgRPC_SSL_PROVIDER=package \
  -DgRPC_CARES_PROVIDER=module \
  -DgRPC_PROTOBUF_PROVIDER=package \
  -DgRPC_GFLAGS_PROVIDER=module \
  -DgRPC_BENCHMARK_PROVIDER=module \
  -DBUILD_SHARED_LIBS=ON

TARGET_CFLAGS += \
  -D_GLIBCXX_USE_C99=1

HOST_CFLAGS += \
  -std=c99 \
  -Wno-implicit-fallthrough \
  -Wno-implicit-function-declaration

PKG_INSTALL:=1
 
define Package/grpc
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=BSB002 platform gRPC libraries
	DEPENDS:=+libpthread +librt +libstdcpp +openssl +protobuf
endef

# Attributions only required when statically linking against boringssl. We use OpenSSL so we don't care.
define Package/grpc/attributions
endef

define Package/grpc/description
	gRPC
	This package contains gRPC libraries
endef

define Build/Compile
	$(call Build/Compile/Default,$(GRPC_TARGETS))
endef

define Build/Install
	$(call Build/Install/Default,$(GRPC_TARGETS))
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/include/grpc/ $(1)/usr/include/
	$(CP) $(PKG_BUILD_DIR)/include/grpc++/ $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/libgpr.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/libgrpc*.so* $(1)/usr/lib/
endef

define Package/grpc/install
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/libgpr.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/libgrpc*.so* $(1)/usr/lib/
endef

define Host/Compile
	$(call Host/Compile/Default,$(GRPC_HOST_TARGETS) LDFLAGS="$(HOST_LDFLAGS)" CFLAGS="$(HOST_CFLAGS)")
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/bin
	$(INSTALL_BIN) $(HOST_BUILD_DIR)/bins/opt/grpc* $(STAGING_DIR_HOST)/bin
endef

$(eval $(call BuildPackage,grpc))
$(eval $(call HostBuild))


include $(TOPDIR)/rules.mk

PKG_NAME:=shadow-node
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/deps/shadow-node
PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=MIT
PKG_BUILD_DEPENDS:=shadow-node/host

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/cmake.mk

SHADOW_NODE_SOURCE:=$(BUILD_DIR)/shadow-node
SHADOW_NODE_TARGET_ARCH:=$(shell echo ${CONFIG_ARCH})
SHADOW_NODE_TARGET_BOARD:=$(shell echo ${CONFIG_TARGET_BOARD})

CMAKE_HOST_OPTIONS += \
  -B$(HOST_BUILD_DIR)/out \
  -H$(HOST_BUILD_DIR) \
	-DTARGET_ARCH=$(HOST_ARCH) \
	-DTARGET_OS=$(HOST_OS) \
	-DTARGET_BOARD=None \
	-DPLATFORM_DESCRIPTOR=$(HOST_ARCH)-$(HOST_OS) \
	-DCMAKE_BUILD_TYPE=Release \
	-DFEATURE_DEBUGGER=OFF \
	-DFEATURE_HEAP_PROFILER=OFF \
	-DFEATURE_CPU_PROFILER=OFF \
	-DFEATURE_PROFILE='$(SHADOW_NODE_SOURCE)/deps/jerry/jerry-core/profiles/es5.1.profile' \
	-DENABLE_LTO=OFF \
	-DENABLE_SNAPSHOT=ON \
	-DENABLE_NAPI=ON \
	-DENABLE_JERRYX=ON \
	-DBUILD_LIB_ONLY=OFF \
	-DINSTALL_PREFIX='$(STAGING_DIR_HOSTPKG)/usr'

CMAKE_OPTIONS += \
  -B$(PKG_BUILD_DIR)/out \
  -H$(PKG_BUILD_DIR) \
	-DTARGET_ARCH=$(CONFIG_ARCH) \
	-DTARGET_OS=linux \
	-DTARGET_BOARD=$(SHADOW_NODE_TARGET_BOARD) \
	-DPLATFORM_DESCRIPTOR=$(CONFIG_ARCH)-linux \
	-DCMAKE_BUILD_TYPE=Release \
	-DFEATURE_DEBUGGER=OFF \
	-DFEATURE_MEM_STATS=OFF \
	-DFEATURE_HEAP_PROFILER=ON \
	-DFEATURE_CPU_PROFILER=ON \
	-DFEATURE_PROFILE='$(SHADOW_NODE_SOURCE)/deps/jerry/jerry-core/profiles/es5.1.profile' \
	-DENABLE_LTO=OFF \
	-DENABLE_SNAPSHOT=ON \
	-DENABLE_NAPI=ON \
	-DENABLE_JERRYX=ON \
	-DBUILD_LIB_ONLY=OFF \
	-DEXTERNAL_MODULES='' \
	-DMEM_HEAP_SIZE_KB=2048 \
	-DEXTERNAL_LIBS='' \
	-DEXTERNAL_COMPILE_FLAGS='' \
	-DEXTERNAL_LINKER_FLAGS='' \
	-DEXTERNAL_INCLUDE_DIR='' \
	-DEXTERNAL_SNAPSHOT_TOOL='$(HOST_BUILD_DIR)/out/deps/jerry-host/bin/jerry-snapshot' \
	-DINSTALL_PREFIX='/usr'

HOST_MAKE_FLAGS += -C $(HOST_BUILD_DIR)/out
MAKE_FLAGS += -C $(PKG_BUILD_DIR)/out

define Package/shadow-node
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=shadow-node
  DEPENDS:=+libstdcpp +dbus +zlib +librt
endef

define Host/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(HOST_BUILD_DIR)
	$(RM) $(HOST_BUILD_DIR)/deps/jerry/jerry-core/include/jerryscript-snapshot.h.rej
	$(RM) $(HOST_BUILD_DIR)/deps/jerry/tests/unit-core/test-snapshot.c.rej
	$(call Host/Prepare/Default,)
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(RM) $(PKG_BUILD_DIR)/deps/jerry/jerry-core/include/jerryscript-snapshot.h.rej
	$(RM) $(PKG_BUILD_DIR)/deps/jerry/tests/unit-core/test-snapshot.c.rej
	$(call Build/Prepare/Default,)
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOSTPKG)/usr/lib
	$(INSTALL_DIR) $(STAGING_DIR_HOSTPKG)/usr/bin
	$(call $(Host/Install/Default,))
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/shadow-node/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/iotjs $(1)/usr/bin/node
endef


$(eval $(call BuildPackage,shadow-node))
$(eval $(call HostBuild))

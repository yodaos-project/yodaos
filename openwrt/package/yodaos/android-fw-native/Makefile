
include $(TOPDIR)/rules.mk

PKG_NAME:=android-fw-native
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/3rd/android_lib/native

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

ifeq ($(CONFIG_USE_GLIBC),)
TARGET_CFLAGS += -D_GNU_SOURCE=1
endif

define Package/android-fw-native
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=android-fw-native
  DEPENDS:=+libstdcpp +property +android-system-core +android-hardware +libopenssl
endef

define Package/android-fw-native/description
	property_service
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

ifeq ($(CONFIG_TARGET_leo),y)

define Package/android-fw-native/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

else ifeq ($(CONFIG_TARGET_amlogic),y)

define Package/android-fw-native/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/tool_env/fw_printenv $(1)/usr/bin/
	$(LN) fw_printenv $(1)/usr/bin/fw_setenv
endef

else
endif

$(eval $(call BuildPackage,android-fw-native))


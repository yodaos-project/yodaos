
include $(TOPDIR)/rules.mk

PKG_NAME:=key_hw
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/hardware/key_hw

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/key_hw
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=key_hw
	DEPENDS:=
endef

define Package/key_hw/config
        source "$(SOURCE)/Config.in"
endef

define Package/key_hw/description
	key_hw
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/key_hw/
	$(CP) $(PKG_BUILD_DIR)/include/*.h $(1)/usr/include/key_hw/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so $(1)/usr/lib/
endef

define Package/key_hw/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,key_hw))


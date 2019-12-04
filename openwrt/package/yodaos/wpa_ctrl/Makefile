
include $(TOPDIR)/rules.mk

PKG_NAME:=wpa_ctrl
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/wifi

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/wpa_ctrl
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=wpa_ctrl
  DEPENDS:=+libstdcpp +wpa-supplicant +curl +property +android-system-core
endef

define Package/wpa_ctrl/description
	wpa_ctrl
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

define Package/wpa_ctrl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,wpa_ctrl))

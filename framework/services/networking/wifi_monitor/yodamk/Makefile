include $(TOPDIR)/rules.mk

PKG_NAME:=wifi_monitor
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/services/wifi_monitor


PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/wifi_monitor
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=wifi_monitor
	DEPENDS:=+libstdcpp +wpa-supplicant +wpa_ctrl +rkamsex +libjson-c
endef

define Package/wifi_monitor/description
	wifi monitor services
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	echo "Here is wifi monitor fileware Build/InstallDev"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/wifi_monitor $(1)/usr/bin/
endef

define Package/wifi_monitor/install
	echo "Here is wifi monitor fileware Install"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/wifi_monitor $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/wifi_monitor $(1)/etc/init.d/wifi_monitor
endef

$(eval $(call BuildPackage,wifi_monitor))


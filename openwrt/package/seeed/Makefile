#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=seeed
PKG_RELEASE:=2

PKG_LICENSE:=FSFULLR
PKG_LICENSE_FILES:=LICENSE

PKG_MAINTAINER:=fanqiuren <qiuren.fan@rokid.com>

include $(INCLUDE_DIR)/package.mk


define Package/seeed
  SECTION:=libs
  CATEGORY:=Seeed
  TITLE:=seeed voice card
endef

define Build/Prepare
	$(INSTALL_DIR) $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef      

define Build/Configure
endef

define Build/Compile
endef

define Build/InstallDev
	
endef


define Package/seeed/install
    $(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/seeed-voicecard $(1)/etc/init.d/
	$(INSTALL_DIR) $(1)/etc/voicecard
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/*.state $(1)/etc/voicecard
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/*.conf $(1)/etc/voicecard
	ln -s $(1)/etc/init.d/seeed-voicecard $(1)/etc/rc.d/S99seeed-voicecard
endef

$(eval $(call BuildPackage,seeed))



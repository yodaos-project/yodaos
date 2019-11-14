#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=freecwmp
PKG_VERSION:=2014-06-12
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://dev.freecwmp.org/freecwmp
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=8f3c163fc85337e63bfa64da3c02f10d1fe3b169
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += \
	-DACS:STRING=multi \
	-DDEBUG=ON \
	-DDEVEL=ON

define Package/freecwmp
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=CWMP client
  DEPENDS:=+libcurl +libfreecwmp +libuci +libubox +libubus +libmicroxml +shflags
  MAINTAINER:=Luka Perkov <luka@openwrt.org>
endef

define Package/freecwmp/description
 A free client implementation of CWMP (TR-069) protocol
endef

define Package/freecwmp/conffiles
/etc/config/freecwmp
/usr/share/freecwmp/defaults
endef

define Package/freecwmp/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/bin/freecwmpd $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/ext/openwrt/config/freecwmp $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/freecwmpd.init $(1)/etc/init.d/freecwmpd
	$(INSTALL_DIR) $(1)/usr/share/freecwmp
	$(CP) $(PKG_BUILD_DIR)/ext/openwrt/scripts/defaults $(1)/usr/share/freecwmp
	$(CP) $(PKG_BUILD_DIR)/ext/openwrt/scripts/functions $(1)/usr/share/freecwmp
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ext/openwrt/scripts/freecwmp.sh $(1)/usr/sbin/freecwmp
endef

$(eval $(call BuildPackage,freecwmp))

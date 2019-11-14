#
# Copyright (C) 2014-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=freenetconfd
PKG_VERSION:=2015-11-12
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_LICENSE:=GPL-2.0+
PKG_MAINTAINER:=Luka Perkov <luka@openwrt.org>

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/freenetconf/freenetconfd.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=9b4dc3f45982ac9ede2db2218f5578f7268528ba
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/freenetconfd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=netconf server
  URL:=http://www.freenetconf.org/
  DEPENDS:=+libuci +libubox +libubus +libroxml
endef

CMAKE_OPTIONS = \
	-DCMAKE_INSTALL_PREFIX=/ \

define Package/freenetconfd/conffiles
/etc/config/freenetconfd
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/freenetconfd
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/freenetconfd \
		$(1)/usr/include
endef

define Package/freenetconfd/install
	$(INSTALL_DIR) $(1)/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/freenetconfd \
		$(1)/sbin/

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) \
		$(PKG_BUILD_DIR)/config/freenetconfd \
		$(1)/etc/config/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) \
		./files/freenetconfd.init \
		$(1)/etc/init.d/freenetconfd

	$(INSTALL_DIR) $(1)/usr/lib/freenetconfd/
endef

$(eval $(call BuildPackage,freenetconfd))

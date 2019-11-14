#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=freenetconfd-plugin-examples
PKG_VERSION:=2015-06-28
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_LICENSE:=GPL-2.0+
PKG_MAINTAINER:=Luka Perkov <luka@openwrt.org>

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/freenetconf/freenetconfd-plugins.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=4a5dd9be91aae502b3f6a930558159d4ee6a775a
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/freenetconfd-plugin-examples
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=freenetconfd example plugins
  URL:=http://www.freenetconf.org/
  DEPENDS:=+freenetconfd
endef

CMAKE_OPTIONS += \
	-DEXAMPLE_PLUGINS=ON

define Package/freenetconfd-plugin-examples/install
	$(INSTALL_DIR) $(1)/usr/lib/freenetconfd
	$(CP) \
		$(PKG_BUILD_DIR)/lib/freenetconfd/*.so \
		$(1)/usr/lib/freenetconfd/

	$(INSTALL_DIR) $(1)/etc/yang
	$(INSTALL_CONF) \
		$(PKG_BUILD_DIR)/examples/yang/*.yang \
		$(1)/etc/yang/
endef

$(eval $(call BuildPackage,freenetconfd-plugin-examples))

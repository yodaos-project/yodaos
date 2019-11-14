#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=shtool
PKG_VERSION:=2.0.8
PKG_RELEASE:=1

PKG_SOURCE:=${PKG_NAME}-${PKG_VERSION}.tar.gz
PKG_SOURCE_URL:=@GNU/$(PKG_NAME)
PKG_MD5SUM:=c5f7c6836882d48bc79049846a5f9c5b
PKG_LICENSE:=GPL-2.0+

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/shtool
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=The GNU Portable Shell Tool
endef

define Package/shtool/description
 GNU shtool is a compilation of small but very stable and portable shell
 scripts into a single shell tool.
endef

define Package/shtool/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/shtool $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/shtoolize $(1)/usr/bin/
endef

$(eval $(call BuildPackage,shtool))

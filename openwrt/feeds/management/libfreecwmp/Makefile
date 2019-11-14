#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libfreecwmp
PKG_VERSION:=2014-06-12
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://dev.libfreecwmp.org/libfreecwmp
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=d2fdd97d66fde14859c06228a922066d9e8b669b
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/libfreecwmp
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=CWMP library
  URL:=http://libfreecwmp.org/
  MAINTAINER:=Luka Perkov <luka@openwrt.org>
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libfreecwmp.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libfreecwmp.{a,so} $(1)/usr/lib
endef

define Package/libfreecwmp/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libfreecwmp.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,libfreecwmp))

#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libmicroxml
PKG_VERSION:=2012-06-11
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://dev.freecwmp.org/microxml
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=72965423184f24cc0b963d91c2d1863cdb01b6aa
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_LICENSE:=LGPL-2.0
PKG_LICENSE_FILES:=COPYING

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/libmicroxml
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=XML library
  MAINTAINER:=Luka Perkov <luka@openwrt.org>
endef

define Package/libmicroxml/description
 A micro sized XML library
endef

CONFIGURE_ARGS += \
	--disable-threads \
	--enable-static \
	--enable-shared

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/microxml.h $(1)/usr/include
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libmicroxml.so* $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_BUILD_DIR)/microxml.pc $(1)/usr/lib/pkgconfig
endef

define Package/libmicroxml/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/libmicroxml.so* $(1)/usr/lib
endef

$(eval $(call BuildPackage,libmicroxml))

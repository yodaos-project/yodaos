#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=spandsp
PKG_VERSION:=0.0.6
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.soft-switch.org/downloads/spandsp/
PKG_MD5SUM:=897d839516a6d4edb20397d4757a7ca3

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_FIXUP:=autoreconf

PKG_LICENSE:=LGPL-2.1 GPL-2.0
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk

define Package/libspandsp
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=spandsp library
  DEPENDS:=+libtiff
endef

TARGET_CFLAGS += $(FPIC)

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libspandsp* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/spandsp.h $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/include/spandsp
	$(CP) $(PKG_INSTALL_DIR)/usr/include/spandsp/*.h $(1)/usr/include/spandsp/

	$(INSTALL_DIR) $(1)/usr/include/spandsp/private
	$(CP) $(PKG_INSTALL_DIR)/usr/include/spandsp/private/*.h $(1)/usr/include/spandsp/private/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/spandsp.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libspandsp/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libspandsp*so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libspandsp))

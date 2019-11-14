#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libpri
PKG_VERSION:=1.6.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://downloads.asterisk.org/pub/telephony/libpri/releases
PKG_HASH:=7225ea7ec334a115f9dc08e71f55589c38cb4e00b13964cd2f08cc4e6123e3f6
PKG_MAINTAINER:=Daniel Golle <daniel@makrotopia.org>

PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/libpri
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=libpri Primary Rate ISDN implementation
  URL:=http://www.asterisk.org/
  DEPENDS:=+dahdi-tools-libtonezone
endef

define Package/libpri/description
	libpri is a C implementation of the Primary Rate ISDN specification. It was
	based on the Bellcore specification SR-NWT-002343 for National ISDN. As of
	May 12, 2001, it has been tested work with NI-2, Nortel DMS-100, and
	Lucent 5E Custom protocols on switches from Nortel and Lucent.
endef

define Build/Configure
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpri*.a $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpri*.so* $(1)/usr/lib/
endef

define Package/libpri/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libpri*.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libpri))

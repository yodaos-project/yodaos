#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=iksemel
PKG_VERSION:=1.4
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://iksemel.googlecode.com/files/
PKG_MD5SUM:=532e77181694f87ad5eb59435d11c1ca

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

PKG_LICENSE:=LGPL-2.1
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk

define Package/libiksemel
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Iksemel Jabber Library
  URL:=http://code.google.com/p/iksemel/
  DEPENDS:= +libgnutls +libtasn1 +libgcrypt +libgpg-error
endef

define Package/libiksemel/description
Iksemel is an XML parser library mainly designed for Jabber applications.
It provides SAX, DOM, and special Jabber stream APIs. Library is coded
in ANSI C except the network code (which is POSIX compatible), thus
highly portable.
endef

TARGET_CFLAGS += $(FPIC)
TARGET_LDFLAGS += \
	-Wl,-rpath-link,$(STAGING_DIR)/usr/lib \
	-lgnutls -lgcrypt -lgpg-error

define Build/Configure
	$(call Build/Configure/Default, \
		--enable-shared \
		--enable-static \
		--with-libgnutls-prefix="$(STAGING_DIR)/usr" \
		, \
		LIBS="$(TARGET_LDFLAGS)" \
	)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/iksemel.h $(1)/usr/include/
	
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libiksemel.{a,so*} $(1)/usr/lib/
	
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/iksemel.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libiksemel/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libiksemel.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libiksemel))

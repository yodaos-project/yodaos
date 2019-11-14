# 
# Copyright (C) 2006-2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=bcg729
PKG_VERSION:=1.0.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://download-mirror.savannah.gnu.org/releases/linphone/plugins/sources/
PKG_HASH:=77c923edc57a53014da5f31788b63efbabfc7277c06deaeada06574628e4b03f

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_INSTALL:=1

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Alex Samorukov <samm@os2.kiev.ua>

include $(INCLUDE_DIR)/package.mk

define Package/bcg729
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Software G729A encoder and decoder library written in C
  URL:=http://www.linphone.org/technical-corner/bcg729.html
  DEPENDS:=@BUILD_PATENTED
endef

define Package/bcg729/description
 Bcg729 is a software G729A encoder and decoder library written in C, developed
 by Belledonne Communications, the company supporting the Linphone project.
 It was written from scratch and is NOT a derivative work of ITU reference
 source code in any kind.
endef

TARGET_CFLAGS += $(FPIC)
CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static=no \
	--disable-msplugin

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) -R $(PKG_INSTALL_DIR)/usr/include/bcg729 $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libbcg729.so* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libbcg729.pc $(1)/usr/lib/pkgconfig/
endef

define Package/bcg729/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libbcg729.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,bcg729))

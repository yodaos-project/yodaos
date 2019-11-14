#
# Copyright (C) 2010-2016 OpenWrt.org
# Copyright (C) 2010 Alfred E. Heggestad
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rem
PKG_VERSION:=0.4.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.creytiv.com/pub
PKG_MD5SUM:=dc7f76de6d3aff19200eb6d0a41dd332

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=docs/COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk

define Package/librem
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libre +libpthread
  TITLE:=Audio and video processing media library
  URL:=http://www.creytiv.com/
endef

TARGET_CFLAGS+=$(FPIC)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LIBRE_MK=$(STAGING_DIR)/usr/share/re/re.mk \
		LIBRE_INC=$(STAGING_DIR)/usr/include/re \
		LIBRE_SO=$(STAGING_DIR)/usr/lib \
		CC="$(TARGET_CC)" \
		EXTRA_CFLAGS="$(TARGET_CFLAGS) -DOPENWRT" \
		EXTRA_LFLAGS="-lm" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		SYSROOT="$$$$($(FIND) $(TOOLCHAIN_DIR) -path '*/include/pthread.h' | sed -ne '1s#/include/pthread.h##p')" \
		SYSROOT_ALT="$(STAGING_DIR)/usr" \
		RELEASE=1 \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		OS=linux \
		all install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/rem $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librem.{a,so} $(1)/usr/lib/
endef

define Package/librem/install
	rm -f $(BUILD_DIR)/$(PKG_NAME)
	ln -sf $(PKG_NAME)-$(PKG_VERSION) $(BUILD_DIR)/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librem.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,librem))

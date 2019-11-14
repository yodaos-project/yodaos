#
# Copyright (C) 2010-2016 OpenWrt.org
# Copyright (C) 2010 Alfred E. Heggestad
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=re
PKG_VERSION:=0.4.17
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.creytiv.com/pub
PKG_MD5SUM:=5900337fd8c77515c87eae5bf0f8c091

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=docs/COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk

define Package/libre
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+libopenssl +libpthread +zlib
  TITLE:=Generic library for real-time communications with async IO support
  URL:=http://www.creytiv.com/
endef

TARGET_CFLAGS += $(FPIC)

ifneq ($(CONFIG_USE_GLIBC)$(CONFIG_USE_MUSL),)
TARGET_CFLAGS += -D_GNU_SOURCE
endif

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		HAVE_LIBRESOLV= \
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
	$(INSTALL_DIR) $(1)/usr/share/re
	$(CP) $(PKG_BUILD_DIR)/mk/re.mk $(1)/usr/share/re/
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/re $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libre.{a,so} $(1)/usr/lib/
endef

define Package/libre/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libre.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,libre))

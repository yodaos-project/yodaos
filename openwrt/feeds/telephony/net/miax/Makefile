#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=miax
PKG_VERSION:=1.4
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)_$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/miax
PKG_MD5SUM:=44f0d2ef46ee2697d890b7b96846adc7
PKG_CHECK_FORMAT_SECURITY:=0

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING LICENSE

include $(INCLUDE_DIR)/package.mk

define Package/miax
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  DEPENDS:=+libpthread
  TITLE:=A console iax (asterisk) client
  URL:=http://www.eja.it/?l=en&n=miax
endef

define Package/miax/description
  miax is a console iax (asterisk) client, it can work with 
  a soundcard as a normal voip phone, taking input/output from 
  keyboard or analog/gsm/isdn modem.
endef

define Build/Compile
	$(MAKE) -C "$(PKG_BUILD_DIR)" \
		CC="$(TARGET_CC)" \
		OFLAGS="$(TARGET_CFLAGS)" \
		CPPFLAGS="$(TARGET_CPPFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		all
endef

define Package/miax/install	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/miax $(1)/usr/bin/
endef

$(eval $(call BuildPackage,miax))

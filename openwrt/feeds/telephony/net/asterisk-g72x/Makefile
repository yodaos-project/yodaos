#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=asterisk-g72x
PKG_VERSION:=1.4.1
PKG_RELEASE:=2

PKG_SOURCE:=asterisk-g72x-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=http://asterisk.hosting.lv/src/
PKG_HASH:=1d1356202de12ee3e636d5146e0e905533b4bf4c6b68eaa7d7789cbaa305059c

PKG_BUILD_DIR=$(BUILD_DIR)/asterisk-g72x-$(PKG_VERSION)
PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

PKG_LICENSE:=GPL-3.0
PKG_LICENSE_FILES:=README.md
PKG_MAINTAINER:=Alex Samorukov <samm@os2.kiev.ua>

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/asterisk-g72x/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  TITLE:=G.729 codec for Asterisk open source PBX
  URL:=http://asterisk.hosting.lv
  DEPENDS:= +bcg729
endef

define Package/asterisk11-codec-g729
$(call Package/asterisk-g72x/Default)
  DEPENDS+= asterisk11
  VARIANT:=asterisk11
endef

define Package/asterisk13-codec-g729
$(call Package/asterisk-g72x/Default)
  DEPENDS+= asterisk13
  VARIANT:=asterisk13
endef

define Package/description/Default
 Asterisk G.729 codec based on bcg729 implementation.
endef

Package/asterisk11-codec-g729/description = $(Package/description/Default)
Package/asterisk13-codec-g729/description = $(Package/description/Default)

ifeq ($(BUILD_VARIANT),asterisk11)
  MAKE_ARGS:= \
	CC="$(TARGET_CC)" \
	LD="$(TARGET_LD)" \
	CFLAGS="$(TARGET_CFLAGS) -DASTERISK_VERSION_NUM=110000 -DLOW_MEMORY -D_XOPEN_SOURCE=600 $(TARGET_CPPFLAGS) -I$(STAGING_DIR)/usr/include/asterisk-11/include -DHAVE_CONFIG_H -I. -fPIC" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	DESTDIR="$(PKG_INSTALL_DIR)"

  CONFIGURE_ARGS+=\
	--with-asterisk-includes=$(STAGING_DIR)/usr/include/asterisk-11/include \
	--with-asterisk100 \
	--with-bcg729 \
	--enable-shared \
	$(MAKE_ARGS)
endif

ifeq ($(BUILD_VARIANT),asterisk13)
  MAKE_ARGS:= \
	CC="$(TARGET_CC)" \
	LD="$(TARGET_LD)" \
	CFLAGS="$(TARGET_CFLAGS) -DASTERISK_VERSION_NUM=130000 -DLOW_MEMORY -D_XOPEN_SOURCE=600 $(TARGET_CPPFLAGS) -I$(STAGING_DIR)/usr/include/asterisk-13/include -DHAVE_CONFIG_H -I. -fPIC" \
	LDFLAGS="$(TARGET_LDFLAGS)" \
	DESTDIR="$(PKG_INSTALL_DIR)"

  CONFIGURE_ARGS+=\
	--with-asterisk-includes=$(STAGING_DIR)/usr/include/asterisk-13/include \
	--with-asterisk130 \
	--with-bcg729 \
	--enable-shared \
	$(MAKE_ARGS)
endif

define Package/Install/Default
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/asterisk/modules/codec_g729.so $(1)/usr/lib/asterisk/modules/
endef

Package/asterisk11-codec-g729/install = $(Package/Install/Default)
Package/asterisk13-codec-g729/install = $(Package/Install/Default)

$(eval $(call BuildPackage,asterisk11-codec-g729))
$(eval $(call BuildPackage,asterisk13-codec-g729))

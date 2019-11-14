#
# Copyright (C) 2012-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=shflags
PKG_VERSION:=2012-06-11
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/kward/shflags.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=c12b30cebb92b49c5edb6df255c0952c65b4ddc6
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_LICENSE:=LGPL-2.1

include $(INCLUDE_DIR)/package.mk

define Package/shflags
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=shflags
  DEPENDS:=+getopt
  URL:=http://code.google.com/p/shflags/
  MAINTAINER:=Luka Perkov <luka@openwrt.org>
endef

define Package/shflags/description
 command-line flags module for Unix shell scripts
endef

define Build/Configure
endef

define Build/Compile
endef

define Build/Install
endef

define Package/shflags/install
	$(INSTALL_DIR) $(1)/usr/share/shflags
	$(CP) $(PKG_BUILD_DIR)/source/1.0/src/shflags  $(1)/usr/share/shflags/shflags.sh
endef

$(eval $(call BuildPackage,shflags))

#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=freesub
PKG_VERSION:=2014-12-03
PKG_RELEASE=$(PKG_SOURCE_VERSION)

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/freenetconf/freesub.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=a1a38e80c6642af723d6aa65f64910dcf27cb3da
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz
PKG_LICENSE:=GPL-2.0+

PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/freesub
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=SSH subsystem helper
  URL:=http://www.freenetconf.org/
  DEPENDS:=+libubox
  MAINTAINER:=Luka Perkov <luka@openwrt.org>
endef

CMAKE_OPTIONS = \
	-DCMAKE_INSTALL_PREFIX=/usr \

define Package/freesub/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/freesub \
		$(1)/usr/bin/

	ln -s /usr/bin/freesub $(1)/usr/bin/netconf
endef

$(eval $(call BuildPackage,freesub))

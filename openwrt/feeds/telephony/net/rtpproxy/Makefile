#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=rtpproxy
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://github.com/sippy/rtpproxy.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=d5926892630139bcbe55d1f3fa36764c881648bc
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)
PKG_CHECK_FORMAT_SECURITY:=0
PKG_INSTALL:=1

PKG_LICENSE:=BSD-2-Clause
PKG_LICENSE_FILES:=COPYING

include $(INCLUDE_DIR)/package.mk

define Package/rtpproxy
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  DEPENDS:=+libpthread +librt
  TITLE:=RTP (Realtime Transport Protocol) proxy
  URL:=http://www.rtpproxy.org/
endef

define Package/rtpproxy/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/rtpproxy $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/rtpproxy.init $(1)/etc/init.d/rtpproxy

	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) ./files/rtpproxy.config $(1)/etc/config/rtpproxy
endef

$(eval $(call BuildPackage,rtpproxy))

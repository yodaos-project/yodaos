#
# Copyright (C) 2017 OpenWrt.org
# Copyright (C) 2017 Jiri Slachta <jiri@slachta.eu>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=libsrtp2
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=git://github.com/cisco/libsrtp.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=12ea269db3e4e9af7ce5de524fe07312fa6e035b
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION)-$(PKG_SOURCE_VERSION).tar.gz

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk

define Package/libsrtp2
  SUBMENU:=Telephony
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Secure RTP (SRTP) library, v$(PKG_VERSION)
  URL:=http://sourceforge.net/projects/srtp
  DEPENDS:=+libpcap
endef

define Package/libsrtp2/description
Open-source implementation of the Secure Real-time Transport
Protocol (SRTP) originally authored by Cisco Systems, Inc.
It is available under a BSD-style license.
endef

define Build/Install
	$(call Build/Install/Default,all shared_library install)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/srtp2 $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libsrtp2.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libsrtp2.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libsrtp2/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/lib/libsrtp2.so.* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,libsrtp2))

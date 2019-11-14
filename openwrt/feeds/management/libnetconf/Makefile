#
# Copyright (C) 2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libnetconf
PKG_VERSION:=0.10.0
PKG_RELEASE:=2
PKG_REV:=414c7e2293bd6c3108e895f111a1ab5614545ce2
PKG_INSTALL:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/cesnet/libnetconf
PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=$(PKG_REV)
PKG_SOURCE_SUBDIR:=$(PKG_NAME)
PKG_LICENSE:=BSD-3-Clause

PKG_BUILD_DIR=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/libnetconf
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=NETCONF library
  URL:=$(PKG_SOURCE_URL)
  DEPENDS:= \
	+libxml2 \
	+zlib \
	+LIBNETCONF_ENABLE_TLS:libopenssl \
	+LIBNETCONF_ENABLE_VALIDATION:libxslt \
	+LIBNETCONF_ENABLE_URL:libcurl \
	+LIBNETCONF_USE_LIBSSH:libssh
  MAINTAINER:=Jiri Slachta <jiri@slachta.eu>
endef

define Package/libnetconf/description
 libnetconf is the NETCONF library in C intended for building NETCONF clients and servers.

 libnetconf provides basic functions to connect NETCONF client and server to each other via
 SSH, to send and receive NETCONF messages and to store and work with the configuration data
 in a datastore.
endef

define Package/libnetconf/config
  source "$(SOURCE)/Config.in"
endef

CONFIGURE_ARGS+=\
	--prefix=/usr/ \
	--exec-prefix=/usr/ \
	--without-dnssec \
	$(call autoconf_bool,CONFIG_LIBNETCONF_USE_LIBSSH,libssh) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_DNSSEC,dnssec) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_NOTIFICATIONS,notifications) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_TLS,tls) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_URL,url) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_VALIDATION,validation) \
	$(call autoconf_bool,CONFIG_LIBNETCONF_ENABLE_YANG_SCHEMAS,yang-schemas) \
	--with-workingdir=$(CONFIG_LIBNETCONF_WORKING_DIR)

#TODO pyapi

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libnetconf{,_ssh,_xml}*.h $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/include/libnetconf
	$(CP) $(PKG_INSTALL_DIR)/usr/include/libnetconf/*.h $(1)/usr/include/libnetconf/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnetconf* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/libnetconf.pc $(1)/usr/lib/pkgconfig/
endef

define Package/libnetconf/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libnetconf.so* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)$(CONFIG_LIBNETCONF_WORKING_DIR)

	$(INSTALL_DIR) $(1)/usr/share/libnetconf
	$(CP) $(PKG_INSTALL_DIR)/usr/share/libnetconf/rnglib $(1)/usr/share/libnetconf/
endef

$(eval $(call BuildPackage,libnetconf))

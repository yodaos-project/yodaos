# 
# Copyright (C) 2006 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=lz4
PKG_VERSION:=1.8.2
#https://github.com/lz4/lz4/archive/v1.8.2.zip

PKG_SOURCE:=v$(PKG_VERSION).zip
PKG_SOURCE_URL:=https://github.com/lz4/lz4/archive
PKG_MD5SUM:=805180da1be418a4092f53b0241ce170

HOST_BUILD_DIR:=$(BUILD_DIR_HOST)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/host-build.mk

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) 
endef

define Host/Install
	$(INSTALL_DIR) $(STAGING_DIR_HOST)/{lib,include}
	$(CP) $(HOST_BUILD_DIR)/lz4 $(STAGING_DIR_HOST)/bin
	$(CP) $(HOST_BUILD_DIR)/lib/*.h $(STAGING_DIR_HOST)/include/
	$(CP) $(HOST_BUILD_DIR)/lib/liblz4.a $(STAGING_DIR_HOST)/lib/
endef

define Host/Clean
endef

$(eval $(call HostBuild))

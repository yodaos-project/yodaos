#
# Copyright (C) 2007-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=protobuf
PKG_VERSION:=3.6.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-all-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/google/protobuf/releases/download/v$(PKG_VERSION)
#PKG_MD5SUM:=bd5e3eed635a8d32e2b99658633815ef

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=LICENSE

PKG_USE_MIPS16:=0# MIPS16 prevents protobuf's usage of the 'sync' asm-opcode
PKG_INSTALL:=1
PKG_BUILD_DEPENDS:=protobuf/host
HOST_BUILD_DEPENDS:=python/host

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/protobuf
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A structured data encoding library
  URL:=https://github.com/google/protobuf
  DEPENDS:=+zlib +libpthread +libstdcpp
  MAINTAINER:=Per Sandström <per.j.sandstrom@gmail.com>
endef

define Package/protobuf/description
 Protocol Buffers are a way of encoding structured data in an efficient
 yet extensible format. Google uses Protocol Buffers for almost all
 of its internal RPC protocols and file formats.
endef

CONFIGURE_ARGS += --with-protoc=$(STAGING_DIR_HOSTPKG)/bin/protoc
define Host/Install
	(cd $(HOST_BUILD_DIR)/python; $(STAGING_DIR_HOSTPKG)/bin/python setup.py build)
	(cp $(HOST_BUILD_DIR)/src/protoc $(STAGING_DIR_HOSTPKG)/bin/protoc)
	$(call $(Host/Install/Default,))
endef

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/lib \
		$(1)/usr/include

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/* \
		$(1)/usr/include/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/* \
		$(1)/usr/lib/
endef

define Package/protobuf/install
	$(INSTALL_DIR) \
		$(1)/usr/lib

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotoc.so*  \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotobuf-lite.so* \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libprotobuf.so* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,protobuf))
$(eval $(call HostBuild))

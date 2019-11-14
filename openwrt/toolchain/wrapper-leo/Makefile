#
# Copyright (C) 2012 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=wrapper-leo
PKG_VERSION:=1

include $(INCLUDE_DIR)/toolchain-build.mk

define Host/Prepare
endef

define Host/Configure
endef

define Host/Compile
endef

define Host/Install
	rm -rf $(TOOLCHAIN_DIR)
	$(CP) $(CONFIG_EXTERNAL_TOOLCHAIN_DIR) $(TOOLCHAIN_DIR)
endef

define Host/Clean
	rm -rf $(TOOLCHAIN_DIR)
endef

$(eval $(call HostBuild))

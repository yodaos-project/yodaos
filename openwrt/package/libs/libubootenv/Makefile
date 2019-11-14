#
# Copyright (C) 2009-2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=libubootenv
PKG_VERSION:=2016.09
PKG_RELEASE:=1
USE_SOURCE_DIR := $(CONFIG_EXTERNAL_UBOOT_TREE)
include $(INCLUDE_DIR)/package.mk

define Package/libubootenv
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Uboot env libraries and tools
  DEPENDS:=+uboot-leo
endef

define Package/libubootenv/description
  Create libubootenv for swupdate
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		TARGET_CFLAGS="$(TARGET_CFLAGS)" \
		HOSTLDFLAGS= \
		env
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/tools/env/lib.a $(1)/usr/lib/libubootenv.a
	$(CP) $(PKG_BUILD_DIR)/tools/env/fw_env.h $(1)/usr/include/fw_env.h
endef

define Package/libubootenv/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_BUILD_DIR)/tools/env/fw_printenv $(1)/usr/sbin
	$(LN) fw_printenv $(1)/usr/sbin/fw_setenv
endef

$(eval $(call BuildPackage,libubootenv))

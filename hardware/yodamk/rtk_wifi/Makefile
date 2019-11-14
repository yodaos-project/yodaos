#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtk_wifi
PKG_RELEASE:=2
USE_SOURCE_DIR := $(ROKID_DIR)/hardware/wifi/realtek/driver_5.6.7

include $(INCLUDE_DIR)/package.mk

define KernelPackage/rtk_wifi
  SUBMENU:=WIFI
  SECTION:=WIFI
  CATEGORY:=rokid
  TITLE:=Realtek common drivers
ifeq ($(CONFIG_TARGET_quanzhi),y)
#  FILES:=$(PKG_BUILD_DIR)/8723ds.ko
else
  FILES:=$(PKG_BUILD_DIR)/8723ds.ko
endif
endef

define KernelPackage/rtk_wifi/description
 Kernel module for register a custom rtk_wifi platform device.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(USE_SOURCE_DIR)/* $(PKG_BUILD_DIR)/
ifeq ($(CONFIG_TARGET_amlogic),y)
	$(CP) $(PKG_BUILD_DIR)/Makefile_a113 $(PKG_BUILD_DIR)/Makefile
endif
ifeq ($(CONFIG_TARGET_hisilicon),y)
	$(CP) $(PKG_BUILD_DIR)/Makefile_hisi $(PKG_BUILD_DIR)/Makefile
endif
ifeq ($(CONFIG_TARGET_rockchip_rk3308),y)
	$(CP) $(PKG_BUILD_DIR)/Makefile_rk3308 $(PKG_BUILD_DIR)/Makefile
endif
ifeq ($(CONFIG_TARGET_quanzhi),y)
	$(CP) $(PKG_BUILD_DIR)/Makefile_quanzhi $(PKG_BUILD_DIR)/Makefile
endif
endef

$(eval $(call KernelPackage,rtk_wifi))

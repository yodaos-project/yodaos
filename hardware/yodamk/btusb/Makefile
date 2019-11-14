#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=btusb
PKG_VERSION:=1.0
PKG_RELEASE:=2

USE_SOURCE_DIR := $(ROKID_DIR)/hardware/bt/ampak/btusb

include $(INCLUDE_DIR)/package.mk


define KernelPackage/btusb
  SUBMENU:=bluetooth
  SECTION:=bluetooth
  CATEGORY:=rokid
  TITLE:=Broadcom bluetooth usb drivers
  FILES:=$(PKG_BUILD_DIR)/btusb.ko
endef

define KernelPackage/btusb/description
 Kernel module for register a custom btusb platform device.
endef

EXTRA_KCONFIG := CONFIG_BTUSB=m
EXTRA_CFLAGS += -DEXPORT_SYMTAB -Wno-error=date-time -Wno-date-time
EXTRA_CFLAGS += -I$(PKG_BUILD_DIR)/inc -I$(PKG_BUILD_DIR) -I$(PKG_BUILD_DIR)/src -I$(PKG_BUILD_DIR)/src/gki -I$(STAGING_DIR)/usr/include

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="$(EXTRA_CFLAGS)" \
	$(EXTRA_KCONFIG)


define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(USE_SOURCE_DIR)/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,btusb))

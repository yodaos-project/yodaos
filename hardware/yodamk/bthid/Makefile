#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bthid
PKG_VERSION:=1.0
PKG_RELEASE:=2

USE_SOURCE_DIR := $(ROKID_DIR)/hardware/bt/ampak/bthid

include $(INCLUDE_DIR)/package.mk


define KernelPackage/bthid
  SUBMENU:=bluetooth
  SECTION:=bluetooth
  CATEGORY:=rokid
  TITLE:=Broadcom bluetooth usb drivers
  FILES:=$(PKG_BUILD_DIR)/bthid.ko
endef

define KernelPackage/bthid/description
 Kernel module for register a custom bthid platform device.
endef

EXTRA_KCONFIG := CONFIG_BTHID=m
EXTRA_CFLAGS += -DLINUX_KERNEL -D_LINUX_ -DEXPORT_SYMTAB -Wno-error=date-time -Wno-date-time

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

$(eval $(call KernelPackage,bthid))

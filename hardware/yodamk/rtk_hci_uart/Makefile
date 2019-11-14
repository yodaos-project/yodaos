#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtk_hci_uart
PKG_RELEASE:=2

USE_SOURCE_DIR := $(ROKID_DIR)/hardware/bt/realtek/driver_v3.10

include $(INCLUDE_DIR)/package.mk


define KernelPackage/rtk_hci_uart
  SUBMENU:=bluetooth
  SECTION:=bluetooth
  CATEGORY:=rokid
  TITLE:=REALTEK hci uart drivers
  FILES:=$(PKG_BUILD_DIR)/hci_uart.ko
endef

define KernelPackage/rtk_hci_uart/description
 Kernel module for register a custom rtk_hci_uart platform device.
endef

EXTRA_KCONFIG:= \
	CONFIG_BT_HCIUART=m

EXTRA_CFLAGS:= \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=m,%,$(filter %=m,$(EXTRA_KCONFIG)))) \
	$(patsubst CONFIG_%, -DCONFIG_%=1, $(patsubst %=y,%,$(filter %=y,$(EXTRA_KCONFIG)))) \

HCI_CFLAGS:= -Wall -Wstrict-prototypes -Wno-unused-variable -Dlinux


EXTRA_CFLAGS += -I$(PKG_BUILD_DIR)/include -I$(PKG_BUILD_DIR) -I$(STAGING_DIR)/usr/include
EXTRA_CFLAGS += $(HCI_CFLAGS)

EXTRA_LDFLAGS += --strip-debug

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
	echo $(CPPFLAGS)
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,rtk_hci_uart))

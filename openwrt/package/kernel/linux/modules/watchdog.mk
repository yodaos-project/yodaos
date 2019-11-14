#
# Copyright (C) 2006-2011 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

WATCHDOG_MENU:=watchdog modules

define KernelPackage/watchdog
  SUBMENU:=$(WATCHDOG_MENU)
  TITLE:=no reboot with watchdog timeout
  KCONFIG:=CONFIG_LEDS_SOFTDOG_ALARM
endef

define KernelPackage/watchdog/description
  Kernel module for Watchdog debug
endef

$(eval $(call KernelPackage,watchdog))

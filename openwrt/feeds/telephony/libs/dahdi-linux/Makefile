# 
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=dahdi-linux
PKG_VERSION:=2.11.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://downloads.asterisk.org/pub/telephony/dahdi-linux/releases/
PKG_MD5SUM:=af96917e15eebc36519948133ec7e36b

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Vittorio Gambaletta <openwrt@vittgam.net>

DAHDI_MODULES_EXTRA:=

include $(INCLUDE_DIR)/package.mk

define KernelPackage/dahdi
  SUBMENU:=Voice over IP
  TITLE:=DAHDI basic infrastructure
  DEPENDS:=@USB_SUPPORT +kmod-lib-crc-ccitt
  URL:=http://www.asterisk.org/
  FILES:= $(PKG_BUILD_DIR)/drivers/dahdi/dahdi.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoProbe,dahdi)
endef

define KernelPackage/dahdi/description
  This package contains DAHDI basic infrastructure.
endef

define KernelPackage/dahdi-echocan-oslec
  SUBMENU:=Voice over IP
  TITLE:=DAHDI OSLEC echo canceller support
  DEPENDS:=kmod-dahdi +kmod-echo
  URL:=http://www.asterisk.org/
  FILES:=$(PKG_BUILD_DIR)/drivers/dahdi/dahdi_echocan_oslec.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoProbe,dahdi_echocan_oslec)
endef

define KernelPackage/dahdi-echocan-oslec/description
  This package contains DAHDI OSLEC echo canceller support.
endef

define KernelPackage/dahdi-hfcs
  SUBMENU:=Voice over IP
  TITLE:=DAHDI driver for HFC-S PCI
  DEPENDS:=@PCI_SUPPORT kmod-dahdi
  URL:=http://sourceforge.net/projects/dahdi-hfcs/
  FILES:= $(PKG_BUILD_DIR)/drivers/dahdi/hfcs/dahdi_hfcs.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoProbe,dahdi_hfcs)
endef

define KernelPackage/dahdi-hfcs/description
  This package contains DAHDI driver for HFC-S based PCI BRI adapters.
endef

define KernelPackage/dahdi-dummy
  SUBMENU:=Voice over IP
  TITLE:=dummy DAHDI driver
  DEPENDS:=kmod-dahdi
  URL:=http://www.asterisk.org/
  FILES:= $(PKG_BUILD_DIR)/drivers/dahdi/dahdi_dummy.$(LINUX_KMOD_SUFFIX)
  AUTOLOAD:=$(call AutoProbe,dahdi_dummy)
endef

define KernelPackage/dahdi-dummy/description
  This package contains the dummy DAHDI driver providing only DAHDI timing
  without any real telephony hardware.
endef


define Build/Configure
endef

TARGET_CFLAGS += $(FPIC)

define Build/Prepare
	$(Build/Prepare/Default)
	mkdir -p $(PKG_BUILD_DIR)/drivers/staging/echo/
	$(CP) ./files/oslec.h $(PKG_BUILD_DIR)/drivers/staging/echo/
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		ARCH="$(LINUX_KARCH)" \
		$(TARGET_CONFIGURE_OPTS) \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		KERNELVERSION="$(KERNEL)" \
		KERNEL_VERSION="$(LINUX_VERSION)" \
		KERNELDIR="$(LINUX_DIR)" \
		KSRC:="$(LINUX_DIR)" \
		KVERS:="$(LINUX_VERSION)" \
		TOPDIR:="$(TOPDIR)" \
		INCLUDE_DIR:="$(INCLUDE_DIR)" \
		PWD:="$(PKG_BUILD_DIR)" \
		MODULES_EXTRA:="$(DAHDI_MODULES_EXTRA)"
endef

define Build/InstallDev
	mkdir -p $(1)/usr/include/dahdi
	$(CP) $(PKG_BUILD_DIR)/include/dahdi/dahdi_config.h $(1)/usr/include/dahdi/
	$(CP) $(PKG_BUILD_DIR)/include/dahdi/fasthdlc.h     $(1)/usr/include/dahdi/
	$(CP) $(PKG_BUILD_DIR)/include/dahdi/kernel.h       $(1)/usr/include/dahdi/
	$(CP) $(PKG_BUILD_DIR)/include/dahdi/user.h         $(1)/usr/include/dahdi/
	$(CP) $(PKG_BUILD_DIR)/include/dahdi/wctdm_user.h   $(1)/usr/include/dahdi/
endef

$(eval $(call KernelPackage,dahdi))
$(eval $(call KernelPackage,dahdi-echocan-oslec))
$(eval $(call KernelPackage,dahdi-dummy))
$(eval $(call KernelPackage,dahdi-hfcs))

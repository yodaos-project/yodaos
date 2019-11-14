#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=u-boot
PKG_VERSION:=2015.01
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2

# use gcc-linaro-aarch64-linux-gnu-4.9 to compile uboot
UBOOT_CROSS_COMPILE=$(TOPDIR)/../toolchains/aarch64/gcc-linaro-aarch64-linux-gnu-4.9/bin/aarch64-linux-gnu-

ifneq ($(CONFIG_EXTERNAL_UBOOT_TREE),"")
	USE_SOURCE_DIR := $(CONFIG_EXTERNAL_UBOOT_TREE)
endif
include $(INCLUDE_DIR)/package.mk

define uboot/amlogic-a113
  TITLE := U-Boot for a113 board
  CONFIG := rokidme
  IMAGE := all
endef

define uboot/amlogic-a113-dev11
  TITLE := U-Boot for a113 dev1.1 board
  CONFIG := a113_dev11
  IMAGE := all
endef

define uboot/amlogic-a113-rokidme
  TITIL := U-Boot for a113 rokidme board
  CONFIG := a113_rokidme
  IMAGE := all
endef

define uboot/amlogic-a113-xmlyhorn
  TITIL := U-Boot for a113 rokidme board
  CONFIG := a113_xmlyhorn
  IMAGE := all
endef

define uboot/amlogic-a113-tuyamini
  TITIL := U-Boot for a113 tuyamini board
  CONFIG := a113_tuyamini
  IMAGE := all
endef

define uboot/amlogic-a113-rokidmini
  TITIL := U-Boot for a113 rokidmini board
  CONFIG := a113_rokidmini
  IMAGE := all
endef

UBOOTS :=\
	amlogic-a113 \
	amlogic-a113-tuyamini \
	amlogic-a113-rokidmini \
	amlogic-a113-dev11 \
	amlogic-a113-xmlyhorn \
	amlogic-a113-rokidme

define Package/uboot/template
define Package/uboot-$(1)
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=$(2)
  VARIANT:=$(1)
endef
endef

define BuildUBootPackage
	$(eval $(uboot/Default))
	$(eval $(uboot/$(1)))
	$(call Package/uboot/template,$(1),$(TITLE))
endef

ifdef BUILD_VARIANT
$(eval $(call uboot/$(BUILD_VARIANT)))
UBOOT_CONFIG:=$(if $(CONFIG),$(CONFIG),$(BUILD_VARIANT))
UBOOT_IMAGE:=$(if $(IMAGE),$(IMAGE),openwrt-$(BOARD)-$(BUILD_VARIANT)-u-boot.bin)
UBOOT_MAKE_OPTS += \
     UBOOT_CUSTOM_PLAT=$(UBOOT_CONFIG)
endif


define Build/Configure
	+$(MAKE) -C $(PKG_BUILD_DIR) $(UBOOT_MAKE_OPTS) $(UBOOT_CONFIG)_defconfig
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) $(UBOOT_MAKE_OPTS) CROSS_COMPILE=$(UBOOT_CROSS_COMPILE) $(IMAGE)
endef

define Package/uboot/install/default
	$(INSTALL_DIR) $(BIN_DIR)
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.encrypt $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.encrypt.efuse $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.usb.bl2 $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.usb.tpl $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.sd.bin $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.encrypt.usb.bl2 $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.encrypt.usb.tpl $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/fip/u-boot.bin.encrypt.sd.bin $(BIN_DIR)/
endef

define Package/uboot/install/template
define Package/uboot-$(1)/install
	$(call Package/uboot/install/default,$(2))
endef
endef

$(foreach u,$(UBOOTS), \
	$(eval $(call Package/uboot/install/template,$(u),$(u))) \
)

$(foreach u,$(UBOOTS), \
	$(eval $(call BuildUBootPackage,$(u))) \
	$(eval $(call BuildPackage,uboot-$(u))) \
)

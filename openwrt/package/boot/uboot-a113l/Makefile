#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=u-boot
PKG_VERSION:=2019
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2


ifneq ($(CONFIG_EXTERNAL_UBOOT_TREE),"")
	USE_SOURCE_DIR := $(CONFIG_EXTERNAL_UBOOT_TREE)
endif
include $(INCLUDE_DIR)/package.mk

define uboot/amlogic-a113l
  TITLE := U-Boot for a113l board
  CONFIG := a1_ad401_nand_linux
endef


UBOOTS :=\
	amlogic-a113l

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
endef

define Build/Compile
	cd $(PKG_BUILD_DIR); ./mk a1_ad401_nand_linux
endef

define Build/Clean
	cd $(PKG_BUILD_DIR); ./mk clean
endef

define Package/uboot/install/default
	$(INSTALL_DIR) $(BIN_DIR)
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.encrypt $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.usb.bl2 $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.usb.tpl $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.sd.bin $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.encrypt.usb.bl2 $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.encrypt.usb.tpl $(BIN_DIR)/
	$(CP) $(PKG_BUILD_DIR)/build/u-boot.bin.encrypt.sd.bin $(BIN_DIR)/
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

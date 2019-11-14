#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=u-boot
PKG_VERSION:=2016.03
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
ifneq ($(CONFIG_EXTERNAL_UBOOT_TREE),"")
	USE_SOURCE_DIR := $(CONFIG_EXTERNAL_UBOOT_TREE)
endif
include $(INCLUDE_DIR)/package.mk

define uboot/leo-gx8010-ssd-1v
  TITLE := U-Boot for LEO GX8010 SSD 1V board
  CONFIG := leo_gx8010_ssd_1v
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-gx8010-rkd-1v
  TITLE := U-Boot for LEO GX8010 RKD 1V board
  CONFIG := leo_gx8010_rkd_1v
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-gx8010-rkd-2v
  TITLE := U-Boot for LEO GX8010 RKD 2V board
  CONFIG := leo_gx8010_rkd_2v
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-gx8010-rkd-3v
  TITLE := U-Boot for LEO GX8010 RKD 3V board
  CONFIG := leo_gx8010_rkd_3v
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-gx8010-rkd-4v
  TITLE := U-Boot for LEO GX8010 RKD 4V board
  CONFIG := leo_gx8010_rkd_4v
  IMAGE := u-boot-with-spl.sfp
endef


define uboot/leo-gx8009b-bd3m
  TITLE := U-Boot for LEO GX8009B BD3M
  CONFIG := leo_gx8009b_bd3m
  IMAGE := u-boot-with-spl.sfp
endef
define uboot/leo-gx8010-rkd-naboo
  TITLE := U-Boot for LEO GX8010 RKD NABOO board
  CONFIG := leo_gx8010_rkd_naboo
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-universal
  TITLE := U-Boot for LEO K18 UNIVERSAL board
  CONFIG := leo_k18_universal
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-dev31
  TITLE := U-Boot for LEO K18 DEV3.1 board
  CONFIG := leo_k18_dev31
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-dev32
  TITLE := U-Boot for LEO K18 DEV3.2 board
  CONFIG := leo_k18_dev32
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-dev33
  TITLE := U-Boot for LEO K18 DEV3.3 board
  CONFIG := leo_k18_dev33
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-dev34
  TITLE := U-Boot for LEO K18 DEV3.4 board
  CONFIG := leo_k18_dev34
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-dev311
  TITLE := U-Boot for LEO K18 DEV3.11 board
  CONFIG := leo_k18_dev311
  IMAGE := u-boot-with-spl.sfp
endef

define uboot/leo-k18-cus360
  TITLE := U-Boot for LEO K18 customer360 board
  CONFIG := leo_k18_cus360
  IMAGE := u-boot-with-spl.sfp
endef

UBOOTS := \
	leo-gx8010-ssd-1v \
	leo-gx8010-rkd-1v \
	leo-gx8010-rkd-2v \
	leo-gx8010-rkd-3v \
	leo-gx8010-rkd-4v \
	leo-gx8009b-bd3m \
	leo-gx8010-rkd-naboo \
	leo-k18-universal \
	leo-k18-dev31 \
	leo-k18-dev32 \
	leo-k18-dev33 \
	leo-k18-dev34 \
	leo-k18-dev311 \
	leo-k18-cus360

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
endif

ifneq ($(CONFIG_LEO_SD_ROOTFS_PARTSIZE),"")
SIZE_UTIL:="$(TOPDIR)/../products/rokid/common/rootfs_size_util"
ROOTFS_SIZE:=$(shell $(SIZE_UTIL) size_to_hex $(CONFIG_LEO_SD_ROOTFS_PARTSIZE))
endif

define Build/Configure
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) $(UBOOT_CONFIG)_defconfig
	if [ "$(ROOTFS_SIZE)" != "" ]; then \
		sed -i -e 's/CONFIG_SPECIFIED_ROOTFS_SIZE=0x0/CONFIG_SPECIFIED_ROOTFS_SIZE=$(ROOTFS_SIZE)/' $(PKG_BUILD_DIR)/.config ; \
	fi
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C $(PKG_BUILD_DIR) CROSS_COMPILE=$(TARGET_CROSS) $(IMAGE)
endef

ifneq ($(CONFIG_TARGET_leo_k18_universal),y)
define Package/uboot/install/default
	$(CP) $(PKG_BUILD_DIR)/u-boot.img		\
		$(BIN_DIR)/openwrt-$(1)-u-boot.img
	$(CP) $(PKG_BUILD_DIR)/spl/u-boot-spl.bin	\
		$(BIN_DIR)/openwrt-$(1)-u-boot-spl.bin
endef
else
define Package/uboot/install/default
	if [ "$(CONFIG_BINARY_PREFIX)" = "" ]; then \
	  $(CP) $(PKG_BUILD_DIR)/u-boot.img		\
		  $(BIN_DIR)/openwrt-leo-k18-universal-u-boot.img ; \
	  $(CP) $(PKG_BUILD_DIR)/spl/u-boot-spl.bin	\
		  $(BIN_DIR)/openwrt-leo-k18-universal-u-boot-spl.bin ;\
	else \
	  $(CP) $(PKG_BUILD_DIR)/u-boot.img		\
		  $(BIN_DIR)/$(CONFIG_BINARY_PREFIX)-u-boot.img ;\
	  $(CP) $(PKG_BUILD_DIR)/spl/u-boot-spl.bin	\
		  $(BIN_DIR)/$(CONFIG_BINARY_PREFIX)-u-boot-spl.bin ;\
	fi
endef
endif

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

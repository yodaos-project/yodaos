#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=u-boot
PKG_VERSION:=2017.06
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)
#PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2


ifneq ($(CONFIG_EXTERNAL_UBOOT_TREE),"")
	USE_SOURCE_DIR := $(CONFIG_EXTERNAL_UBOOT_TREE)
endif

include $(INCLUDE_DIR)/package.mk


define uboot/r328
  TITLE := U-Boot for r328 board
  IMAGE := all
  CONFIG := r328
endef


UBOOTS :=\
	r328

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


define Build/Configure
	echo "Build configure"
endef

define Build/Compile
	( cd $(PKG_BUILD_DIR)/brandy-2.0;bash ./build.sh -p sun8iw18p1; )
endef

define Build/Clean
#	( cd $(PKG_BUILD_DIR); ./make_clean.sh )
endef

define Package/uboot/install/default
	$(INSTALL_DIR) $(BIN_DIR)
	$(CP) $(PKG_BUILD_DIR)/out/u-boot-sun8iw18p1.bin $(BIN_DIR)/u-boot.fex
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

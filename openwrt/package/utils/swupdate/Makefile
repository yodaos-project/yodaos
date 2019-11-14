#
# Copyright (C) 2009-2010 Jo-Philipp Wich <xm@subsignal.org>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=swupdate
PKG_VERSION:=2017.07
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/swupdate
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Update the software on an embedded system
  DEPENDS:=+ubi-utils +libopenssl +zlib +zeromq +libconfig +libarchive +libjson-c +libcurl +libubootenv
endef

define Package/swupdate/description
  Swupdate provides a reliable way to update the software on an
  embedded system.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include

define Build/Configure
	+$(MAKE) -C $(PKG_BUILD_DIR) leo_defconfig
endef

define Build/Compile
	+$(MAKE) -C $(PKG_BUILD_DIR) CROSS_COMPILE=$(TARGET_CROSS) TARGET_CFLAGS="$(TARGET_CFLAGS)"
endef

define Package/swupdate/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/swupdate $(1)/usr/bin
endef

$(eval $(call BuildPackage,swupdate))

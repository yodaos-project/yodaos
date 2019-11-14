include $(TOPDIR)/rules.mk

PKG_NAME:=vold
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/libs/vold
PKG_CONFIG_DIR:=$(TOPDIR)/../products/$(CONFIG_PRODUCT_PATHNAME)/vold

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/vold
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=vold
	DEPENDS:=+ota_unpack
endef

define Package/vold/config
        source "$(SOURCE)/Config.in"
endef

define Package/vold/description
	vold
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	echo "Here is vold fileware Build/InstallDev"
endef

define Package/vold/install
	$(INSTALL_DIR) $(1)/etc/
ifeq ($(CONFIG_FSTAB_SUBNAME),"")
ifeq ($(CONFIG_subtarget_version),"-firefly-ubifs")
	$(CP) $(PKG_CONFIG_DIR)/yodabase.fstab.ubi $(1)/etc/yodabase.recovery.fstab
else
	$(CP) $(PKG_CONFIG_DIR)/yodabase.fstab $(1)/etc/yodabase.recovery.fstab
endif
else
	$(CP) $(PKG_CONFIG_DIR)/yodabase.$(CONFIG_FSTAB_SUBNAME).fstab $(1)/etc/yodabase.recovery.fstab
endif
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/vold $(1)/usr/bin/
ifneq ($(CONFIG_TARGET_ROOTFS_CPIOLZMA),y)
ifeq ($(CONFIG_ETC_INITD_MOUNT),y)
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./mount_data $(1)/etc/init.d/
endif
endif
endef

$(eval $(call BuildPackage,vold))



include $(TOPDIR)/rules.mk

PKG_NAME:=ota_unpack
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/ota
PKG_CONFIG_DIR:=$(ROKID_DIR)/$(CONFIG_PRODUCT_PATHNAME)/ota
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

ifeq ($(CONFIG_TARGET_leo_gx8010_ssd_1v),y)
		CMAKE_OPTIONS += -DARCH_SSD=ON
else ifeq ($(CONFIG_TARGET_leo_gx8010_rkd_1v),y)
		CMAKE_OPTIONS += -DARCH_MINI=ON
else ifeq ($(CONFIG_TARGET_leo_gx8010_rkd_2v),y)
		CMAKE_OPTIONS += -DARCH_NANO=ON
else
		CMAKE_OPTIONS += -DARCH_COMMON=ON
endif

define Package/ota_unpack/config
  source "$(SOURCE)/Config.in"
endef

define Package/ota_unpack
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=ota_unpack
  DEPENDS+= +libpthread
endef

define Package/ota_unpack/description
	ota recovery package
endef

define Build/Prepare
	echo $(PKG_BUILD_DIR)
	echo ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE $(ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE)
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

CMAKE_OPTIONS += \
	-DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=1

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include/recovery
	$(CP) $(PKG_BUILD_DIR)/recovery/recovery.h $(1)/usr/include/recovery/
endef

define Package/ota_unpack/install

ifeq ($(CONFIG_YODABASE_RECOVERY_BOOT),y)
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(CP) $(PKG_BUILD_DIR)/S12otaunpack $(1)/etc/rc.d/
endif

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/unpack $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/unpack_cli $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib

endef

$(eval $(call BuildPackage,ota_unpack))


include $(TOPDIR)/rules.mk

PKG_NAME:=lumenflinger
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/services/lumenflinger

PKG_FIXUP:=libtool
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
else ifeq ($(CONFIG_TARGET_leo_gx8010_rkd_3v),y)
		CMAKE_OPTIONS += -DARCH_SSD=ON
else
		CMAKE_OPTIONS += -DARCH_COMMON=ON
endif

define Package/lumenflinger
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=lumenflinger
  DEPENDS:=+libstdcpp +android-hardware +android-system-core +android-fw-native +vsp
endef

define Package/lumenflinger/description
	lumenflinger
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin

	$(INSTALL_DIR) $(1)/usr/include/lumenflinger
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/lumenflinger
	$(CP) $(PKG_BUILD_DIR)/client/include/* $(1)/usr/include/lumenflinger
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/client/librklumen_light.so $(1)/usr/lib
endef

define Package/lumenflinger/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/lumenflinger $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/librklumen_light.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./files/lumenflinger $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,lumenflinger))


include $(TOPDIR)/rules.mk

PKG_NAME:=vol_ctrl
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/libs/vol_ctrl

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/vol_ctrl
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=vol_ctrl
  DEPENDS:=+libstdcpp +pulseaudio
endef

define Package/vol_ctrl/description
	vol_ctrl
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/vol_ctrl/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/

ifeq ($(CONFIG_TARGET_rockchip_rk3308),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./gain_ctrld $(1)/etc/init.d
endif

ifeq ($(CONFIG_TARGET_quanzhi),y)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./gain_ctrld_quanzhi $(1)/etc/init.d
endif

ifeq ($(CONFIG_TARGET_hisilicon),y)
#	$(INSTALL_DIR) $(1)/usr/bin/hisitools
#	$(CP)  ./hisitools/*  $(1)/usr/bin/hisitools/
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./hisi_nau8540_ctrld $(1)/etc/init.d
endif

endef

$(eval $(call BuildPackage,vol_ctrl))


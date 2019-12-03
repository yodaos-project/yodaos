
include $(TOPDIR)/rules.mk

PKG_NAME:=property
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/property

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/property
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=property
  DEPENDS:=+libstdcpp +libpthread
endef

define Package/property/config
        source "$(SOURCE)/Config.in"
endef

define Package/property/description
	property
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

define Package/property/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/system
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
	$(CP) $(PKG_BUILD_DIR)/prop/default.prop $(1)/
	chmod 644 $(1)/default.prop
	if [ $(CONFIG_TARGET_leo_gx8010_rkd_1v) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino_mini#g" $(1)/default.prop; \
	fi

	if [ $(CONFIG_TARGET_leo_gx8010_rkd_2v) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino#g" $(1)/default.prop; \
	fi
	if [ $(CONFIG_TARGET_leo_gx8010_rkd_3v) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino_me#g" $(1)/default.prop; \
	fi
	if [ $(CONFIG_TARGET_leo_gx8010_ssd_1v) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino_ssd#g" $(1)/default.prop; \
	fi
	if [ $(CONFIG_TARGET_leo_gx8010_rkd_naboo) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino_naboo#g" $(1)/default.prop; \
	fi
	if [ $(CONFIG_TARGET_leo_k18_universal) = y ]; then \
		   $(SED) "s#xxxxxxxx#kamino_universal#g" $(1)/default.prop; \
	fi
	if [ ! -z $(CONFIG_PRODUCT_OS_NAME) ]; then \
		echo "ro.yodaos.build.os=$(CONFIG_PRODUCT_OS_NAME)" >> $(1)/default.prop; \
	fi
	if [ ! -z $(CONFIG_PRODUCT_NAME) ]; then \
		echo "ro.yodaos.build.productname=$(CONFIG_PRODUCT_NAME)" >> $(1)/default.prop; \
	fi
	if [ $(CONFIG_AUDIO_LINK_CUT) = y ]; then \
		echo "persist.player.awake.holdcon=0" >> $(1)/default.prop; \
		echo "persist.player.sound.holdcon=0" >> $(1)/default.prop; \
		echo "persist.player.ttsd.holdcon=0" >> $(1)/default.prop; \
	fi
endef

$(eval $(call BuildPackage,property))


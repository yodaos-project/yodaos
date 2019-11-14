include $(TOPDIR)/rules.mk

PKG_NAME:=hciattach
PKG_VERSION:=
PKG_RELEASE:=1

PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
PKG_INSTALL:=


define Package/hciattach
	SUBMENU:=bluetooth
	SECTION:=bluetooth
	CATEGORY:=rokid
	TITLE:=hciattach
	DEPENDS:=+pulseaudio_enable_bluez
endef

define Package/hciattach/config
	#source "$(SOURCE)/Config.in"
endef

define Package/hciattach/description
	hciattach
endef

define Build/Prepare
	#$(CP) $(PKG_BT_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	#$(CP) $(PKG_BT_SOURCE_DIR)/../firmware/* $(PKG_BUILD_DIR)/
	#$(call Build/Prepare/Default,)
endef

define Build/Compile
	echo "Build/Compile"
endef

define Build/InstallDev
	echo "Here is hciattach Build/InstallDev"
endef

define Package/hciattach/install
	echo "Package hciattach install"
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/lib/firmware/brcm/

	$(INSTALL_BIN) ./S59btsetup $(1)/etc/rc.d/

ifeq ($(CONFIG_WIFI_BT_RPI3B_PLUS),y)
	$(INSTALL_BIN) ./S57bluetooth_rpi3b+ $(1)/etc/rc.d/S57bluetooth
	$(CP) -rf ./firmware/RPI3B_PLUS/* $(1)/lib/firmware/brcm/
endif
endef

$(eval $(call BuildPackage,hciattach))


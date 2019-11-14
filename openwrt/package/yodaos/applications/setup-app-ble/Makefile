
include $(TOPDIR)/rules.mk

PKG_NAME:=setup-app-ble
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/applications/cmcc-setup-ble

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_FIND_ROOT_PATH := ${CMAKE_HOST_FIND_ROOT_PATH};${STAGING_DIR}

define Package/setup-app-ble
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=setup-app-ble
  DEPENDS:=+PACKAGE_flora:flora +PACKAGE_flora:mutils +httpsession +ssl +libandroid_cutils +libavformat +libwavplayer +librplayer +jsoncpp +libmqtt +libev +jsruntime
endef

ifeq ($(CONFIG_MQTT_STATIC), y)
	CMAKE_OPTIONS += -DMQTT_LIB_STATIC=ON
else ifeq ($(CONFIG_MQTT_SHARED), y)
	CMAKE_OPTIONS += -DMQTT_LIB_SHARED=ON
endif

define Package/setup-app-ble/config
	source "$(SOURCE)/Config.in"
endef


define Package/setup-app-ble/description
	setup-app-ble
endef


define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/opt/apps/network/lib
	$(INSTALL_DIR) $(1)/opt/apps/network
	$(INSTALL_DIR) $(1)/opt/apps/network/media
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"

ifeq ($(CONFIG_TARGET_hisilicon_hi3561),y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/hi3651/lib*.so $(1)/opt/apps/network/lib/
else ifeq ($(CONFIG_TARGET_amlogic_a113),y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/a113/lib*.so $(1)/opt/apps/network/lib/
else ifeq ($(CONFIG_TARGET_leo_k18_universal),y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/kamino/lib*.so $(1)/opt/apps/network/lib/
endif

ifeq ($(CONFIG_MQTT_SHARED), y)
	$(CP) $(PKG_BUILD_DIR)/deps/libmqtt/src/lib*.so.* $(1)/opt/apps/network/lib/
	$(CP) $(PKG_BUILD_DIR)/deps/libmqtt/src/lib*.so $(1)/opt/apps/network/lib/
endif
	$(CP) $(PKG_BUILD_DIR)/setup-app-ble $(1)/opt/apps/network
endef

define Package/setup-app-ble/install
	$(INSTALL_DIR) $(1)/opt/apps/network/lib
	$(INSTALL_DIR) $(1)/opt/apps/network
	$(INSTALL_DIR) $(1)/opt/apps/network/media
	echo "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@2"

ifeq ($(CONFIG_TARGET_hisilicon_hi3561), y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-HI3561/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/hi3651/lib*.so $(1)/opt/apps/network/lib/
else ifeq ($(CONFIG_TARGET_amlogic_a113), y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-Me/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/a113/lib*.so $(1)/opt/apps/network/lib/
else ifeq ($(CONFIG_TARGET_leo_k18_universal), y)
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/package.json $(1)/opt/apps/network/
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/ble_connected.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/connect_common_failure.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/openbluetooth.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/prepare_connect_wifi.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/wifi_is_connecting.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/setup_network.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/login_failed.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/history_networks_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/network_not_found.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/key_config_notify.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/custom/Rokid-dev33/media/connect_with_wrong_pwd.wav $(1)/opt/apps/network/media
	$(CP) $(PKG_BUILD_DIR)/deps/qlink/kamino/lib*.so $(1)/opt/apps/network/lib/
endif

ifeq ($(CONFIG_MQTT_SHARED), y)
	$(CP) $(PKG_BUILD_DIR)/deps/libmqtt/src/lib*.so.* $(1)/opt/apps/network/lib/
	$(CP) $(PKG_BUILD_DIR)/deps/libmqtt/src/lib*.so $(1)/opt/apps/network/lib/
endif
	$(CP) $(PKG_BUILD_DIR)/setup-app-ble $(1)/opt/apps/network
endef

$(eval $(call BuildPackage,setup-app-ble))

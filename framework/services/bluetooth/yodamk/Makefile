include $(TOPDIR)/rules.mk

PKG_NAME:=bluetooth_service
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/services/bluetooth_service


PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/bluetooth_service
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=bluetooth_service
	DEPENDS:=+libstdcpp +libjson-c +libffmpeg-audio-dec +android-hardware +libeloop_mini +PACKAGE_flora:flora +PACKAGE_flora:mutils +libopus +PACKAGE_gxopus:gxopus
endef

CMAKE_OPTIONS += -DBT_SERVICE_IPC_BY_FLORA=1

ifeq ($(CONFIG_PRODUCT_OS_NAME),"yodaos")
    CMAKE_OPTIONS += -DROKID_YODAOS=1
endif

ifeq ($(CONFIG_BT_SERVICE_HAVE_HFP),y)
    CMAKE_OPTIONS += -DBT_SERVICE_HAVE_HFP=1
endif

ifeq ($(CONFIG_TARGET_leo),y)
    CMAKE_OPTIONS += -DBT_SERVICE_USE_K18_HW_OPUS=1
else
    CMAKE_OPTIONS += -DBT_SERVICE_USE_SW_OPUS=1
endif

define Package/bluetooth_service/config
	source "$(SOURCE)/Config.in"
endef

define Package/bluetooth_service/description
	bluetooth service
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${PKG_SOURCE_DIR}/../cmake-modules
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/InstallDev
	echo "Here is bluetooth fileware Build/InstallDev"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/bluetooth_service $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/
endef

define Package/bluetooth_service/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/bluetooth_service $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/demo/bt_service_test $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/demo/bt_monitor $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./bt_service $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,bluetooth_service))


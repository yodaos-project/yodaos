include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bcmdhd_fw
PKG_VERSION:=1.0
PKG_RELEASE:=$(PKG_SOURCE_VERSION)
USE_SOURCE_DIR := $(ROKID_DIR)/hardware/wifi/bcmdhd/tools/bcmdhd_fw

ifeq ($(CONFIG_aarch64),y)
USE_EXE_DIR := $(ROKID_DIR)/vendor/broadcom/wifi/arm64
USE_EXE_BT_DIR := $(ROKID_DIR)/vendor/broadcom/bt/arm64
else
USE_EXE_DIR := $(ROKID_DIR)/vendor/broadcom/wifi/arm
USE_EXE_BT_DIR := $(ROKID_DIR)/vendor/broadcom/bt/arm
endif

ifeq ($(CONFIG_TARGET_leo),y)
PKG_KO_DIR:=$(ROKID_DIR)/vendor/nationalchip/gx8010/ko
endif

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

WIFI_TYPE_NAME=${CONFIG_WIFI_TYPE_NAME}

define Package/bcmdhd_fw
	SUBMENU:=WIFI
	SECTION:=WIFI
	CATEGORY:=rokid
	DEPENDS:=libusb-1.0 +kmod-bcmdhd +kmod-bthid
	TITLE:= Bcmd WiFi && BT module.
endef

define Package/bcmdhd_fw/description
	Bcmd WiFi && BT module.
endef

define Build/Prepare
	echo "Here is bcmd firmware Package/Prepare"
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) $(USE_SOURCE_DIR)/* $(PKG_BUILD_DIR)/
	$(CP) $(USE_SOURCE_DIR)/../../firmware/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	echo "Here is bcmd firmware Build/Compile"
	+$(MAKE) -C $(PKG_BUILD_DIR)/ CROSS_COMPILE=$(TARGET_CROSS) TARGET_CFLAGS="$(TARGET_CFLAGS)" WIFI_TYPE_NAME=$(WIFI_TYPE_NAME)
endef

define Build/InstallDev
	echo "Here is bcmd firmware Build/InstallDev"
endef

define Package/bcmdhd_fw/install
	echo "Here is bcmd firmware install"
	echo $(1)

	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bcmdhd_fw $(1)/usr/bin/
	$(INSTALL_BIN) $(USE_EXE_DIR)/*  $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_BIN) ./S40wlan $(1)/etc/rc.d/
	$(INSTALL_BIN) ./K2wlan $(1)/etc/rc.d/
	$(INSTALL_BIN) ./S41bthid $(1)/etc/rc.d/

ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(INSTALL_BIN) $(USE_EXE_BT_DIR)/*  $(1)/usr/bin/
	$(RM) -rf $(1)/etc/rc.d/S41bthid*
	$(INSTALL_BIN) ./hw_test/S59btsetup $(1)/etc/rc.d/
endif

	$(INSTALL_DIR) $(1)/etc/wifi/

ifeq ($(CONFIG_PACKAGE_AMPAK_6255),y)
	$(INSTALL_DIR) $(1)/etc/wifi/6255/
	$(CP) -rf $(PKG_BUILD_DIR)/6255/* $(1)/etc/wifi/6255/
	$(RM) -rf $(1)/etc/wifi/6255/hw_test
ifeq ($(CONFIG_WIFI_BT_HW_TEST),y)
	$(CP) -rf $(PKG_BUILD_DIR)/6255/hw_test/*.bin $(1)/etc/wifi/6255/
endif
ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(INSTALL_BIN) ./hw_test/S57bluetooth_6255 $(1)/etc/rc.d/S57bluetooth
endif
endif

ifeq ($(CONFIG_PACKAGE_AMPAK_6236),y)
	$(INSTALL_DIR) $(1)/etc/wifi/6212/
	$(CP) -rf $(PKG_BUILD_DIR)/6236/* $(1)/etc/wifi/6212/
	$(RM) -rf $(1)/etc/wifi/6212/hw_test
ifeq ($(CONFIG_WIFI_BT_HW_TEST),y)
	$(CP) -rf $(PKG_BUILD_DIR)/6236/hw_test/*.bin $(1)/etc/wifi/6212/
endif
ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(INSTALL_BIN) ./hw_test/S57bluetooth_6236 $(1)/etc/rc.d/S57bluetooth
endif
endif

ifeq ($(CONFIG_PACKAGE_AMPAK_6212),y)
	$(INSTALL_DIR) $(1)/etc/wifi/6212/
	$(CP) -rf $(PKG_BUILD_DIR)/6212/* $(1)/etc/wifi/6212/
	$(RM) -rf $(1)/etc/wifi/6212/hw_test
ifeq ($(CONFIG_WIFI_BT_HW_TEST),y)
	$(CP) -rf $(PKG_BUILD_DIR)/6212/hw_test/*.bin $(1)/etc/wifi/6212/
endif
ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(INSTALL_BIN) ./hw_test/S57bluetooth_6212a $(1)/etc/rc.d/S57bluetooth
endif
endif

ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(SED) "s#ttyS1#$(CONFIG_BT_UART_DEV)#g" $(1)/etc/rc.d/S57bluetooth
	$(SED) "s#WIFI_TYPE=mmc0#WIFI_TYPE=$(CONFIG_WIFI_TYPE_NAME)#g" $(1)/etc/rc.d/S57bluetooth
endif

ifeq ($(CONFIG_TARGET_leo),y)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	$(CP) $(PKG_KO_DIR)/uart_control.ko $(1)/$(MODULES_SUBDIR)
endif
endef

$(eval $(call BuildPackage,bcmdhd_fw))

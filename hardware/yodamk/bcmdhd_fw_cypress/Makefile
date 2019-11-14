include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=bcmdhd_fw_cypress
PKG_VERSION:=1.0
PKG_RELEASE:=$(PKG_SOURCE_VERSION)

USE_SOURCE_DIR=$(ROKID_DIR)/hardware/wifi/bcmdhd_cypress/firmware
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

include $(INCLUDE_DIR)/package.mk


define Package/bcmdhd_fw_cypress
  SUBMENU:=WIFI
  SECTION:=WIFI
  CATEGORY:=rokid
  DEPENDS:=+PACKAGE_kmod-bcmdhd_cypress:kmod-bcmdhd_cypress +libusb-1.0 +kmod-bthid
  TITLE:=Cypress WiFi && BT module.
endef

define Package/bcmdhd_fw_cypress/description
    Cypress WiFi && BT module.
endef

define Build/Prepare
    echo "Here is Cypress firmware Package/Prepare"
endef

define Build/Compile
    echo "Here is Cypress firmware Build/Compile"
endef

define Build/InstallDev
    echo "Here is Cypress firmware Build/InstallDev"
endef

define Package/bcmdhd_fw_cypress/install
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_BIN) ./K2wlan $(1)/etc/rc.d/
	$(INSTALL_BIN) ./S41bthid $(1)/etc/rc.d/

	$(INSTALL_DIR) $(1)/usr/bin
ifneq ($(CONFIG_YODA_SIMPLIFY_ROOTFS),y)
	$(INSTALL_BIN) $(USE_EXE_DIR)/*  $(1)/usr/bin/
endif

ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
	$(INSTALL_BIN) $(USE_EXE_BT_DIR)/*  $(1)/usr/bin/
	$(INSTALL_BIN) ./hw_test/S57bluetooth $(1)/etc/rc.d/
	$(RM) -rf $(1)/etc/rc.d/S41bthid*
	$(INSTALL_BIN) ./hw_test/S59btsetup $(1)/etc/rc.d/
	$(SED) "s#ttyS1#$(CONFIG_BT_UART_DEV)#g" $(1)/etc/rc.d/S57bluetooth
	$(SED) "s#WIFI_TYPE=mmc0#WIFI_TYPE=$(CONFIG_WIFI_TYPE_NAME)#g" $(1)/etc/rc.d/S57bluetooth
endif

	$(INSTALL_DIR) $(1)/etc/wifi/

ifeq ($(CONFIG_PACKAGE_CYPRESS_NB197),y)
	$(INSTALL_DIR) $(1)/etc/wifi/nb197/
	$(INSTALL_BIN) ./S40wlan_nb197 $(1)/etc/rc.d/S40wlan
	$(CP) -rf $(USE_SOURCE_DIR)/nb197/* $(1)/etc/wifi/nb197/
	$(RM) -rf $(1)/etc/wifi/nb197/hw_test
ifeq ($(CONFIG_WIFI_BT_HW_TEST),y)
	$(CP) -rf $(USE_SOURCE_DIR)/nb197/hw_test/*.bin $(1)/etc/wifi/nb197/
endif
endif

ifeq ($(CONFIG_PACKAGE_CYPRESS_CM256),y)
	$(INSTALL_DIR) $(1)/etc/wifi/cm256/
	$(INSTALL_BIN) ./S40wlan_cm256 $(1)/etc/rc.d/S40wlan
	$(CP) -rf $(USE_SOURCE_DIR)/cm256/* $(1)/etc/wifi/cm256/
	$(RM) -rf $(1)/etc/wifi/cm256/hw_test
ifeq ($(CONFIG_WIFI_BT_HW_TEST),y)
	$(CP) -rf $(USE_SOURCE_DIR)/cm256/hw_test/*.bin $(1)/etc/wifi/cm256/
endif
endif

ifeq ($(CONFIG_TARGET_leo),y)
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)
	$(CP) $(PKG_KO_DIR)/uart_control.ko $(1)/$(MODULES_SUBDIR)
endif
endef

$(eval $(call BuildPackage,bcmdhd_fw_cypress))

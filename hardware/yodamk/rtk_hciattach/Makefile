include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=rtk_hciattach
PKG_VERSION:=
PKG_RELEASE:=1

PKG_BT_SOURCE_DIR:=$(ROKID_DIR)/hardware/bt/realtek/rtk_hciattach_v3.10
PKG_BT_TOOLS_DIR:=$(ROKID_DIR)/vendor/realtek/k18/arm
ifeq ($(CONFIG_TARGET_leo),y)
PKG_KO_DIR:=$(ROKID_DIR)/vendor/nationalchip/gx8010/ko
endif

PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk
PKG_INSTALL:=


define Package/rtk_hciattach
	SUBMENU:=bluetooth
	SECTION:=bluetooth
	CATEGORY:=rokid
	TITLE:=rtk_hciattach
	DEPENDS:=+kmod-rtk_hci_uart +librt
endef

define Package/rtk_hciattach/config
	#source "$(SOURCE)/Config.in"
endef

define Package/rtk_hciattach/description
	realtek rtk_hciattach
endef

define Build/Prepare
	$(CP) $(PKG_BT_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(CP) $(PKG_BT_SOURCE_DIR)/../firmware/* $(PKG_BUILD_DIR)/
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	echo "Here is realtek rtk_hciattach Build/InstallDev"
endef

define Package/rtk_hciattach/install
	echo "Package rtk_hciattach install $(MODULES_SUBDIR)"
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/rc.d/
	$(INSTALL_DIR) $(1)/lib/firmware/rtlbt/
	$(INSTALL_DIR) $(1)/$(MODULES_SUBDIR)

ifeq ($(CONFIG_TARGET_leo),y)
	$(CP) $(PKG_KO_DIR)/uart_control.ko $(1)/$(MODULES_SUBDIR)
endif

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/rtk_hciattach $(1)/usr/sbin/
	$(INSTALL_BIN) ./S40wlan $(1)/etc/rc.d/
	$(INSTALL_BIN) ./S57bluetooth $(1)/etc/rc.d/
	$(SED) "s#ttyS1#$(CONFIG_BT_UART_DEV)#g" $(1)/etc/rc.d/S57bluetooth

	$(INSTALL_BIN) ./S59btsetup $(1)/etc/rc.d/
	#$(INSTALL_BIN) ./wifi_connect.sh $(1)/usr/sbin/wifi_test
	$(INSTALL_BIN) $(PKG_BT_TOOLS_DIR)/rtwpriv $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BT_TOOLS_DIR)/rtlbtmp $(1)/usr/sbin/

ifeq ($(CONFIG_PACKAGE_REALTEK_8723DS),y)
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_fw_v3.10 $(1)/lib/firmware/rtlbt/rtl8723d_fw
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_config_UART1_5M_HWflowon $(1)/lib/firmware/rtlbt/rtl8723d_config
ifeq ($(CONFIG_TARGET_leo),y)
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_config_ACLdataRate2M_UART3M_HWflowoff_1ANT $(1)/lib/firmware/rtlbt/rtl8723d_config
else ifeq ($(CONFIG_TARGET_quanzhi),y)
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_fw_quanzhi $(1)/lib/firmware/rtlbt/rtl8723d_fw
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_config_quanzhi $(1)/lib/firmware/rtlbt/rtl8723d_config
else ifeq ($(CONFIG_TARGET_hisilicon),y)
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtl8723d_fw_v3.10 $(1)/lib/firmware/rtlbt/rtl8723d_fw
endif
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/rtk_btconfig.txt $(1)/etc/
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/hw/mp_rtl8723d_fw $(1)/lib/firmware/
	$(CP) -rf $(PKG_BUILD_DIR)/8723D/hw/mp_rtl8723d_config $(1)/lib/firmware/
endif
endef

$(eval $(call BuildPackage,rtk_hciattach))



include $(TOPDIR)/rules.mk

PKG_NAME:=android-hardware
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/hardware

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

TARGET_LDFLAGS += -Wl,-rpath-link=$(STAGING_DIR)/usr/lib/pulseaudio

ifeq ($(CONFIG_LED_HAL_PWM),y)
    CMAKE_OPTIONS += -DLED_HAL_PWM=ON
else ifeq ($(CONFIG_LED_HAL_VSP),y)
    CMAKE_OPTIONS += -DLED_HAL_VSP=ON
else ifeq ($(CONFIG_LED_HAL_I2C),y)
    CMAKE_OPTIONS += -DLED_HAL_I2C=ON
else ifeq ($(CONFIG_LED_HAL_YODASTAGE),y)
    CMAKE_OPTIONS += -DLED_HAL_YODASTAGE=ON
endif

ifeq ($(CONFIG_LED_HAL_GPIO),y)
    CMAKE_OPTIONS += -DLED_HAL_GPIO=ON
endif

CMAKE_OPTIONS += -DBOARD_LED_NUMS=${CONFIG_BOARD_LED_NUMS}

ifeq ($(CONFIG_POWER_HAL),y)
    CMAKE_OPTIONS += -DPOWER_HAL=ON
endif

ifeq ($(CONFIG_WIFI_HAL),y)
    CMAKE_OPTIONS += -DWIFI_HAL=ON
endif

ifeq ($(CONFIG_ETHNET_HAL),y)
    CMAKE_OPTIONS += -DETHNET_HAL=ON
endif

ifeq ($(CONFIG_MIC_ARRAY_HAL_VSP),y)
    CMAKE_OPTIONS += -DMIC_ARRAY_HAL_VSP=ON
else ifeq ($(CONFIG_MIC_ARRAY_HAL_PCM),y)
    CMAKE_OPTIONS += -DMIC_ARRAY_HAL_PCM=ON
    CMAKE_OPTIONS += -DROKIDOS_BOARDCONFIG_SOUND_CARDID=${CONFIG_ROKIDOS_BOARDCONFIG_SOUND_CARDID}
    CMAKE_OPTIONS += -DROKIDOS_BOARDCONFIG_CAPTURE_DEVICEID=${CONFIG_ROKIDOS_BOARDCONFIG_CAPTURE_DEVICEID}
endif
    CMAKE_OPTIONS += -DBOARD_MIC_CHANNEL=${CONFIG_BOARD_MIC_CHANNEL}
    CMAKE_OPTIONS += -DBOARD_MIC_SAMPLE_RATE=${CONFIG_BOARD_MIC_SAMPLE_RATE}
    CMAKE_OPTIONS += -DBOARD_MIC_BIT_FORMAT=${CONFIG_BOARD_MIC_BIT_FORMAT}

ifeq ($(CONFIG_MODEM_HAL_TTY),y)
    CMAKE_OPTIONS += -DMODEM_HAL_TTY=ON
    CMAKE_OPTIONS += -DBOARD_MODEM_TTY_DEV=${CONFIG_BOARD_MODEM_TTY_DEV}
    CMAKE_OPTIONS += -DBOARD_MODEM_TTY_BAUD=${CONFIG_BOARD_MODEM_TTY_BAUD}
    CMAKE_OPTIONS += -DBOARD_MODEM_TTY_DEBUG=${CONFIG_BOARD_MODEM_TTY_DEBUG}
else

endif

ifeq ($(CONFIG_BLUETOOTH_USE_BSA_AMPAK),y)
    CMAKE_OPTIONS += -DBUILD_BLUETOOTH_BSA_AMPAK=ON
else ifeq ($(CONFIG_BLUETOOTH_USE_BSA_CYPRESS),y)
    CMAKE_OPTIONS += -DBUILD_BLUETOOTH_BSA_CYPRESS=ON
else ifeq ($(CONFIG_BLUETOOTH_USE_BLUEZ),y)
    CMAKE_OPTIONS += -DBUILD_BLUETOOTH_BLUEZ=ON
endif

ifeq ($(CONFIG_KENOBI_MODE),y)
    CMAKE_OPTIONS += -DKENOBI_MODE=ON
endif

define Package/android-hardware
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=android-hardware
  DEPENDS:=+libstdcpp \
    +logging \
    +tinyalsa \
    +android-system-core \
    +libjson-c \
    +libpthread \
    +libusb-1.0 \
    +wpa-supplicant \
    +wpa_ctrl \
    +libffmpeg-audio-dec \
    +pulseaudio \
    +PACKAGE_bsa_server:bsa_server \
    +PACKAGE_vsp:vsp \
    +PACKAGE_vsp_firmware:vsp_firmware
endef

define Package/android-hardware/description
	rokid's android-hardware
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Package/android-hardware/config
	source "$(SOURCE)/Config.in"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(LN) -sf $(PKG_INSTALL_DIR)/usr/include/hardware $(1)/usr/include/r2hw
	$(LN) -sf $(PKG_INSTALL_DIR)/usr/include/hardware $(1)/usr/include/rokidware
	$(LN) -sf $(PKG_INSTALL_DIR)/usr/include/hardware $(1)/usr/include/mic
	
	$(INSTALL_DIR) $(1)/usr/lib/hw
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*hardware.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*default.so* $(1)/usr/lib/hw/

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

define Package/android-hardware/install
	$(INSTALL_DIR) $(1)/usr/lib/hw
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*hardware.so* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*default.so* $(1)/usr/lib/hw/

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/

	$(INSTALL_DIR) $(1)/etc/modem
	$(CP) ./files/* $(1)/etc/modem/
endef

$(eval $(call BuildPackage,android-hardware))


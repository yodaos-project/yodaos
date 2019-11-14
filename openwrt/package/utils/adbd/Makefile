include $(TOPDIR)/rules.mk

PKG_NAME:=adbd
PKG_VERSION:=android.5.0.2_r1
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/adbd
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Android Debug Bridge daemon
  URL:=http://tools.android.com/
  DEPENDS:=+libcap +libopenssl +libstdcpp +glib2
endef

define Package/adbd/description
  The 'adbd' program runs as a background process within an Android device
  or emulated system. Its purpose is to connect to the ADB server
  (through USB for devices, through TCP for emulators) and provide a
  few services for clients that run on the host.
endef


ifeq ($(CONFIG_ADB_TCP_CONNECTABLE), y)
  TARGET_CFLAGS += -DADB_TCP=1
else
  TARGET_CFLAGS += -DADB_TCP=0
endif

ifeq ($(CONFIG_ADB_CONNECT_SECURITY), y)
  TARGET_CFLAGS += -DADB_SECURITY_CONNECT=1
else
  TARGET_CFLAGS += -DADB_SECURITY_CONNECT=0
endif

ifeq ($(CONFIG_ADB_CONNECT_SECURITY_RK), y)
  TARGET_CFLAGS += -DADB_SECURITY_RK=1
else
  TARGET_CFLAGS += -DADB_SECURITY_RK=0
endif

ifeq ($(CONFIG_ADB_CONNECT_SECURITY_SIGN), y)
  TARGET_CFLAGS += -DADB_SECURITY_SIGN=1
else
  TARGET_CFLAGS += -DADB_SECURITY_SIGN=0
endif

ifeq ($(CONFIG_TARGET_quanzhi), y)
  TARGET_CFLAGS += -DPLATFROM_QUANZHI=1
endif

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Configure
	cd $(PKG_BUILD_DIR); \
	./configure --host=$(TARGET_CROSS) \
	CFLAGS="$(TARGET_CFLAGS) " \
	CXXFLAGS="$(TARGET_CXXFLAGS)" \
	LDFLAGS=-L$(STAGING_DIR)/usr/lib
endef

define Package/adbd/config
	source "$(SOURCE)/Config.in"
endef

define Package/adbd/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/adb/adbd $(1)/usr/bin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/adb/xdg-adbd $(1)/usr/bin/
ifeq ($(CONFIG_TARGET_amlogic_a113), y)
	$(CP) ./files/adbd_a113 $(1)/etc/init.d/
else ifeq ($(CONFIG_TARGET_leo), y)
	$(CP) ./files/adbd $(1)/etc/init.d/
else ifeq ($(CONFIG_TARGET_rockchip_rk3308), y)
	$(CP) ./files/adbd_rk3308 $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/usbdevice_rk3308 $(1)/usr/bin/usbdevice
else ifeq ($(CONFIG_TARGET_hisilicon_hi3561), y)
	$(CP) ./files/adbd_hi3561 $(1)/etc/init.d/
else ifeq ($(CONFIG_TARGET_quanzhi), y)
	$(CP) ./files/setusbconfig $(1)/usr/bin/
	$(CP) ./files/adbd_quanzhi $(1)/etc/init.d/
else ifeq ($(CONFIG_TARGET_a113l), y)
	$(CP) ./files/adbd_a113l $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/usbdevice_a113l $(1)/usr/bin/usbdevice
else
	$(CP) ./files/adbd $(1)/etc/init.d/
endif

endef

$(eval $(call BuildPackage,adbd))

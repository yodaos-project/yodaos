
include $(TOPDIR)/rules.mk

PKG_NAME:=jsruntime
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/jsruntime
PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

JSRUNTIME_SOURCE:=$(BUILD_DIR)/jsruntime
CMAKE_OPTIONS += -H$(JSRUNTIME_SOURCE)

ifeq ($(CONFIG_TARGET_amlogic_a113), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT=$(TOOLCHAIN_ROOT_DIR)/$(CONFIG_TARGET_NAME)/libc/
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
else ifeq ($(CONFIG_TARGET_rockchip_rk3308), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT=$(TOOLCHAIN_ROOT_DIR)/$(CONFIG_TARGET_NAME)/libc/
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
else ifeq ($(CONFIG_TARGET_hisilicon), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT='$(TOOLCHAIN_ROOT_DIR)/sysroot'
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
else ifeq ($(CONFIG_TARGET_quanzhi), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT='$(TOOLCHAIN_ROOT_DIR)/lib/'
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
else ifeq ($(CONFIG_TARGET_x86_64), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT='$(TOOLCHAIN_ROOT_DIR)/x86_64-unknown-linux-gnu/libc/'
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
else ifeq ($(CONFIG_TARGET_a113l_a113l), y)
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT=$(TOOLCHAIN_ROOT_DIR)/$(CONFIG_TARGET_NAME)/libc/
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
else
  CMAKE_OPTIONS += -DCMAKE_EXTERNAL_SYSROOT='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_STAGING_PREFIX='$(STAGING_DIR)'
  CMAKE_OPTIONS += -DCMAKE_INCLUDE_PATH='$(STAGING_DIR)/usr/include'
  CMAKE_OPTIONS += -DCMAKE_LIBRARY_PATH='$(STAGING_DIR)/usr/lib/;$(STAGING_DIR)/usr/lib/pulseaudio'
endif

ifeq ($(CONFIG_PRODUCT_HAS_TOUCHPAD), y)
  EXTRA_CFLAGS += -DHAS_TOUCHPAD
endif

CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'
CMAKE_OPTIONS += -DHOST_NODEJS_BIN='$(TOPDIR)/package/rokid/jsruntime/node'

define Package/jsruntime
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=jsruntime
  DEPENDS:=+libstdcpp +android-system-core +shadow-node \
	   +opus +vol_ctrl +wpa_ctrl +ota_unpack \
	   +lumenlight +libffmpeg-audio-dec +librplayer +input-event \
	   +flora +httpsession +property \
	   +node-caps +node-flock +node-flora
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rklogger $(1)/usr/bin/rklogger

	$(INSTALL_DIR) $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/manifest.json $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/hotplug.d $(1)/etc

	$(INSTALL_DIR) $(1)/etc/yoda
	$(CP) $(PKG_INSTALL_DIR)/etc/yoda/* $(1)/etc/yoda/

	$(INSTALL_DIR) $(1)/opt
	$(CP) $(PKG_INSTALL_DIR)/opt/* $(1)/opt/

	$(INSTALL_DIR) $(1)/usr/include/yoda
	$(CP) $(PKG_INSTALL_DIR)/usr/include/*.h $(1)/usr/include/yoda

	$(INSTALL_DIR) $(1)/usr/yoda
	$(CP) $(PKG_INSTALL_DIR)/usr/yoda/* $(1)/usr/yoda/

	$(INSTALL_DIR) $(1)/usr/lib/node_modules
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/* $(1)/usr/lib/node_modules/

	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/yodaos_*.h $(1)/usr/include

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libyodaosclient_c.so $(1)/usr/lib/
endef

define Package/jsruntime/install
	$(INSTALL_DIR) $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/manifest.json $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/hotplug.d $(1)/etc

	$(INSTALL_DIR) $(1)/etc/yoda
	$(CP) $(PKG_INSTALL_DIR)/etc/yoda/* $(1)/etc/yoda/

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./files/* $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/usr/bin
ifeq ($(CONFIG_ARCH), "aarch64")
	$(CP) ./test-stupid-arm64 $(1)/usr/bin/test-stupid
else ifeq ($(CONFIG_TOOLCHAIN_LIBC)$(CONFIG_ARCH), "musl""arm")
	$(CP) ./musl/test-stupid-arm $(1)/usr/bin/test-stupid
else ifeq ($(CONFIG_ARCH), "arm")
	$(CP) ./test-stupid-arm $(1)/usr/bin/test-stupid
endif

	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rklogger $(1)/usr/bin/rklogger

	$(INSTALL_DIR) $(1)/opt
	$(CP) $(PKG_INSTALL_DIR)/opt/* $(1)/opt/

	$(INSTALL_DIR) $(1)/usr/yoda
	$(CP) $(PKG_INSTALL_DIR)/usr/yoda/* $(1)/usr/yoda/

	$(INSTALL_DIR) $(1)/usr/lib/node_modules
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/* $(1)/usr/lib/node_modules/

	# creat dirs for apps
	$(INSTALL_DIR) $(1)/data/AppData

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libyodaosclient_c.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,jsruntime))

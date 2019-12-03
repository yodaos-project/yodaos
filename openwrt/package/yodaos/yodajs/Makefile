
include $(TOPDIR)/rules.mk

PKG_NAME:=yodajs
PKG_VERSION:=master
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/yodaos-project/yoda.js.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -H$(PKG_BUILD_DIR)
CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'
# CMAKE_OPTIONS += -DHOST_NODEJS_BIN='$(TOPDIR)/package/rokid/jsruntime/node'

define Package/jsruntime
  SECTION:=libs
  CATEGORY:=yodaos
  TITLE:=yodajs
  DEPENDS:=+libstdcpp \
  	+android-system-core \
  	+shadow-node \
	  +opus \
	  +vol_ctrl \
	  +wpa_ctrl \
	  +ota_unpack \
	  +lumenlight \
	  +libffmpeg-audio-dec \
	  +librplayer \
	  +input-event \
	  +flora \
	  +httpsession \
	  +property
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
endef

define Package/yodajs/install
	$(INSTALL_DIR) $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/manifest.json $(1)/etc
	$(CP) $(PKG_INSTALL_DIR)/etc/hotplug.d $(1)/etc

	$(INSTALL_DIR) $(1)/etc/yoda
	$(CP) $(PKG_INSTALL_DIR)/etc/yoda/* $(1)/etc/yoda/

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./files/* $(1)/etc/init.d/

	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rklogger $(1)/usr/bin/rklogger

	$(INSTALL_DIR) $(1)/opt
	$(CP) $(PKG_INSTALL_DIR)/opt/* $(1)/opt/

	$(INSTALL_DIR) $(1)/usr/yoda
	$(CP) $(PKG_INSTALL_DIR)/usr/yoda/* $(1)/usr/yoda/

	$(INSTALL_DIR) $(1)/usr/lib/node_modules
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/node_modules/* $(1)/usr/lib/node_modules/
endef

$(eval $(call BuildPackage,yodajs))

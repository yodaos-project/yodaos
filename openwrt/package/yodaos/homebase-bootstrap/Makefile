
include $(TOPDIR)/rules.mk

PKG_NAME:=homebase-bootstrap
PKG_VERSION:=v2.2.8

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=$(PKG_VERSION)
PKG_SOURCE_URL:=https://github.com/yodaos-project/homebase-bootstrap
PKG_SOURCE:=homebase-bootstrap-$(PKG_VERSION).tar.xz
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=GPL-2.0+
include $(INCLUDE_DIR)/package.mk

define Package/homebase-bootstrap
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=homebase-bootstrap
  DEPENDS:=+shadow-node +property_service +property +libcurl +aliyunloglite
endef

define Package/homebase-bootstrap/config
	source "$(SOURCE)/Config.in"
endef

define Build/Compile
	echo "Build/Compile"
endef

define Build/InstallDev
	echo "Build InstallDev"
endef


define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/homebase.sh $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/homebase
	$(CP) -r $(PKG_BUILD_DIR)/etc/* $(1)/etc/homebase/

	if test $(CONFIG_TARGET_amlogic_a113) = y; then \
		rm -rf $(1)/etc/homebase/node_modules/@rokid/core-cloud-logger/logger_bridge_kamino.node; \
	else \
		rm -rf $(1)/etc/homebase/node_modules/@rokid/core-cloud-logger/logger_bridge_amlogic.node; \
	fi
endef

define Package/homebase-bootstrap/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/homebase.sh $(1)/usr/bin/
	chmod +x $(1)/usr/bin/homebase.sh
	$(INSTALL_DIR) $(1)/etc/homebase
	$(CP) -r $(PKG_BUILD_DIR)/etc/* $(1)/etc/homebase/

	if test $(CONFIG_TARGET_amlogic_a113) = y; then \
		rm -rf $(1)/etc/homebase/node_modules/@rokid/core-cloud-logger/logger_bridge_kamino.node; \
	else \
		rm -rf $(1)/etc/homebase/node_modules/@rokid/core-cloud-logger/logger_bridge_amlogic.node; \
	fi
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/homebase $(1)/etc/init.d/homebase
	if [ "$(CONFIG_HOMEBASE_AUTO_RUN)" != "y" ]; then \
		$(SED) "s/^START/#START/g" $(1)/etc/init.d/homebase; \
	fi
endef

$(eval $(call BuildPackage,homebase-bootstrap))

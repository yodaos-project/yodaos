
include $(TOPDIR)/rules.mk

PKG_NAME:=property_service
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/services/property

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/property_service
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=property-service
  DEPENDS:=+libstdcpp +property +android-system-core
endef

define Package/property_service/config
	source "$(SOURCE)/Config.in"
endef

define Package/property_service/description
	property_service
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Package/property_service/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/property_service $(1)/etc/init.d/property_service
	if [ "$(CONFIG_PROPERTY_SERVICE_AUTO_RUN)" != "y" ]; then \
		$(SED) "s/^START/#START/g" $(1)/etc/init.d/property_service; \
	fi
endef

$(eval $(call BuildPackage,property_service))


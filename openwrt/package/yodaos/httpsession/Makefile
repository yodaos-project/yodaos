
include $(TOPDIR)/rules.mk

PKG_NAME:=httpsession
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/httpsession
PKG_FIXUP:=libtool                                                                                  
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

PKG_INSTALL:=

define Package/httpsession
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=httpsession
	DEPENDS:=+libcurl +libstdcpp +libpthread
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/include/*.h $(1)/usr/include
endef

define Package/httpsession/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/*.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,httpsession))

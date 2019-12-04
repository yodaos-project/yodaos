
include $(TOPDIR)/rules.mk

PKG_NAME:=libeloop_mini
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/libeloop_mini
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk


define Package/libeloop_mini
  SECTION:=libs
  CATEGORY:=rokid
  DEPENDS+= +libpthread
  TITLE:=libeloop_mini
endef

define Package/libeloop_mini/description
	eloop_run lib
endef

define Build/Prepare
	echo $(PKG_BUILD_DIR)
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	echo "zpershuai build stage"
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/build/*.so $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/include/eloop_mini
	$(CP) $(PKG_BUILD_DIR)/include/eloop.h $(1)/usr/include/eloop_mini/
	$(CP) $(PKG_BUILD_DIR)/include/eloop.h $(1)/usr/include/eloop_mini/
endef



define Package/libeloop_mini/install
	echo "zpershuai build adfadfasdf"
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_BUILD_DIR)/build/*.so $(1)/usr/lib
endef

$(eval $(call BuildPackage,libeloop_mini))

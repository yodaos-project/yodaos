include $(TOPDIR)/rules.mk

PKG_NAME:=input-event
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/libs/input-event

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/input-event
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=input-event
  DEPENDS:=+libstdcpp +rklog
endef

define Package/input-event/description
	input-event
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/input-event/install
	$(INSTALL_DIR) $(1)/etc

ifeq ($(CONFIG_TARGET_leo_gx8010_rkd_1v),y)
	$(CP) ./files/input-event_touch.conf $(1)/etc
	mv $(1)/etc/input-event_touch.conf $(1)/etc/input-event.conf
else
	$(CP) ./files/input-event.conf $(1)/etc
endif

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,input-event))


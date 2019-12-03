
include $(TOPDIR)/rules.mk

PKG_NAME:=android-system-core
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/android-corelibs

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/android-system-core
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=android-system-core
  DEPENDS:=+libstdcpp +property +android-kernel-headers +libpthread
endef

define Package/android-system-core/description
	android-system-core
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

define Package/android-system-core/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,android-system-core))


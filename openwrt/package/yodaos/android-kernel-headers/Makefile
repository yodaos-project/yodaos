
include $(TOPDIR)/rules.mk

PKG_NAME:=android-kernel-headers
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(TOPDIR)/../framework/libs/android-kernel-headers

PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk
PKG_INSTALL:=

define Package/android-kernel-headers
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=android-kernel-headers
  DEPENDS:=
endef

define Package/android-kernel-headers/description
	android-kernel-headers
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/linux/
	$(CP) $(PKG_BUILD_DIR)/original/uapi/linux/ashmem.h $(1)/usr/include/linux/
	$(CP) $(PKG_BUILD_DIR)/original/uapi/linux/binder.h $(1)/usr/include/linux/
endef

define Package/android-kernel-headers/install
endef

$(eval $(call BuildPackage,android-kernel-headers))

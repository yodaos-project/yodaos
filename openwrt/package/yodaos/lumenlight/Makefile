
include $(TOPDIR)/rules.mk

PKG_NAME:=lumenlight
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/lumenlight

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk


PRODUCT_NAME=kamino


define Package/lumenlight
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=lumenlight
  DEPENDS:=+libstdcpp +android-hardware
  URL:=
endef

define Package/lumenlight/description
	lumenlight lib
endef

TARGET_CFLAGS += $(FPIC)


CMAKE_OPTIONS += \
	-DBUILD_SHARED_LIBS=ON \
	-DSTAGING_DIR=
define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/lumenflinger/
	$(CP) $(PKG_BUILD_DIR)/*.h $(1)/usr/include/lumenflinger/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so $(1)/usr/lib/
endef

define Package/lumenlight/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,lumenlight))


include $(TOPDIR)/rules.mk

PKG_NAME:=blis
PKG_VERSION:=0.2.2
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/flame/blis.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk

define Package/blis
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=blis
  DEPENDS:=+libstdcpp
endef

define Package/blis/description
	blis
endef

TARGET_CFLAGS += $(FPIC)


CONFIGURE_ARGS = \
	--prefix=/usr \
	--enable-shared \
	--enable-cblas \
	-t pthreads \
	armv7a

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/blis/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

define Package/blis/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so $(1)/usr/lib/
endef

$(eval $(call BuildPackage,blis))

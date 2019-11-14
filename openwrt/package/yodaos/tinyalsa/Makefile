
include $(TOPDIR)/rules.mk

PKG_NAME:=tinyalsa
PKG_VERSION:=1.1.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/tinyalsa/tinyalsa.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

PKG_INSTALL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk

define Package/tinyalsa
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=tinyalsa
  DEPENDS:=+libstdcpp +libpthread
endef

define Package/tinyalsa/description
	tinyalsa
endef

TARGET_CFLAGS += $(FPIC)

MAKE_FLAGS += \
	PREFIX=/usr

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
endef

define Package/tinyalsa/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,tinyalsa))


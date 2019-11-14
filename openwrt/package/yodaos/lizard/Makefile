
include $(TOPDIR)/rules.mk

PKG_NAME:=lizard
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/allspark/lizard

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/lizard
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=lizard
  DEPENDS:=+mutils +libpolarssl
endef

define Package/lizard/description
	property
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${PKG_SOURCE_DIR}/../cmake-modules \
				 -DSSL_LINK=shared \
				 -DSSL_LIB=mbedtls
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(INSTALL_DIR) $(1)/usr/bin; \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/; \
	fi
endef

define Package/lizard/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(INSTALL_DIR) $(1)/usr/bin; \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/; \
	fi
endef

$(eval $(call BuildPackage,lizard))


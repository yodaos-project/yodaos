
include $(TOPDIR)/rules.mk

PKG_NAME:=flora
PKG_VERSION:=master
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/yodaos-project/flora.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/flora
  SECTION:=libs
  CATEGORY:=yodaos
  TITLE:=flora
  DEPENDS:=+libstdcpp +mingutils +libncursesw
endef

define Package/flora/description
	property
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=$(ROKID_DIR)/framework/cmake-modules \
				 -DCLI_LOGLEVEL=warning \
				 -DDEBUG_FOR_YODAV8=ON
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

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

define Package/flora/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(INSTALL_DIR) $(1)/usr/bin; \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/; \
	fi
endef

$(eval $(call BuildPackage,flora))


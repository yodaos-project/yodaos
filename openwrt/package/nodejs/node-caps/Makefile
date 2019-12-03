
include $(TOPDIR)/rules.mk

PKG_NAME:=node-caps
PKG_VERSION:=master
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/yodaos-project/node-caps.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

NODE_MODULE_DIR=/usr/lib/node_modules/@yoda/caps
CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'
CMAKE_OPTIONS += -DCMAKE_INSTALL_DIR=$(NODE_MODULE_DIR)

define Package/node-caps
	SECTION:=libs
	CATEGORY:=node_modules
	TITLE:=node-caps
	DEPENDS:=+libstdcpp +shadow-node +mingutils
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)$(NODE_MODULE_DIR)
	$(CP) $(PKG_INSTALL_DIR)$(NODE_MODULE_DIR)/* $(1)$(NODE_MODULE_DIR)
endef

define Package/node-caps/install
	$(INSTALL_DIR) $(1)$(NODE_MODULE_DIR)
	$(CP) $(PKG_INSTALL_DIR)$(NODE_MODULE_DIR)/* $(1)$(NODE_MODULE_DIR)
endef

$(eval $(call BuildPackage,node-caps))

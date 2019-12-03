
include $(TOPDIR)/rules.mk

PKG_NAME:=node-flora
PKG_VERSION:=master
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-v$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/yodaos-project/node-flora.git
PKG_SOURCE_VERSION:=$(PKG_VERSION)

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

JSRUNTIME_SOURCE:=$(BUILD_DIR)/jsruntime
NODE_MODULE_DIR=/usr/lib/node_modules/@yoda/flora
CMAKE_OPTIONS += -DJSRUNTIME_SOURCE_DIR=$(JSRUNTIME_SOURCE)
CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'
CMAKE_OPTIONS += -DCMAKE_INSTALL_DIR=$(NODE_MODULE_DIR)

define Package/node-flora
	SECTION:=libs
	CATEGORY:=nodejs
	TITLE:=node-flora
	DEPENDS:=+libstdcpp +shadow-node +flora
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)$(NODE_MODULE_DIR)
	$(CP) $(PKG_INSTALL_DIR)$(NODE_MODULE_DIR)/* $(1)$(NODE_MODULE_DIR)
endef

define Package/node-flora/install
	$(INSTALL_DIR) $(1)$(NODE_MODULE_DIR)
	$(CP) $(PKG_INSTALL_DIR)$(NODE_MODULE_DIR)/* $(1)$(NODE_MODULE_DIR)
endef

$(eval $(call BuildPackage,node-flora))

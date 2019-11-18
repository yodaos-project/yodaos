
include $(TOPDIR)/rules.mk

PKG_NAME:=node-flora
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/node_modules/@yoda/flora
PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

JSRUNTIME_SOURCE:=$(BUILD_DIR)/jsruntime
NODE_MODULE_DIR=/usr/lib/node_modules/@yoda/flora
CMAKE_OPTIONS += -DJSRUNTIME_SOURCE_DIR=$(JSRUNTIME_SOURCE)
CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'
CMAKE_OPTIONS += -DCMAKE_INSTALL_DIR=$(NODE_MODULE_DIR)

define Package/node-flora
	SECTION:=libs
	CATEGORY:=node_modules
	TITLE:=node-flora
	DEPENDS:=+libstdcpp +shadow-node +flora
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
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

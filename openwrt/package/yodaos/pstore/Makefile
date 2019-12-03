
include $(TOPDIR)/rules.mk

PKG_NAME:=pstore
PKG_VERSION:=1.0
PKG_RELEASE:=1

#PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/services/****

include $(INCLUDE_DIR)/package.mk

define Package/pstore
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=pstore
  DEPENDS:=
endef

define Package/pstore/description
	store kernel crash message
endef

define Build/Prepare
	echo "Build/Prepare pstore"
endef

define Build/Compile
	echo "Build/Compile pstore"
endef

define Build/InstallDev
	echo "Build/InstallDev pstore"
endef

define Package/pstore/install
	echo "Package/pstore/install pstore"
	$(INSTALL_DIR) $(1)/etc/rc.d
	$(INSTALL_BIN) ./files/S99pstore $(1)/etc/rc.d
endef

$(eval $(call BuildPackage,pstore))


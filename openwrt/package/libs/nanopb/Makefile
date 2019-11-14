
include $(TOPDIR)/rules.mk

PKG_NAME:=nanopb
PKG_VERSION:=
PKG_RELEASE:=1
PROTOC:=${STAGING_DIR}/host/bin/protoc

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/cmcc-vbs/nanopb

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/nanopb
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=nanopb
  DEPENDS:=+protobuf/host
endef

define Package/nanopb/description
	property
endef

CMAKE_OPTIONS += -Dnanopb_PROTOC_PATH=$(PROTOC)

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/Install
	(cd $(PKG_BUILD_DIR)/generator/proto;make)
	$(call Build/Install/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.a $(1)/usr/lib/
endef

define Package/nanopb/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.a $(1)/usr/lib/
endef

$(eval $(call BuildPackage,nanopb))

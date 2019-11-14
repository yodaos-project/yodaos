include $(TOPDIR)/rules.mk

PKG_NAME:=nlohmann-json
PKG_VERSION:=v3.6.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/nlohmann/json.git
PKG_SOURCE_DIR:=
PKG_SOURCE_VERSION:=$(PKG_VERSION)

PKG_FIXUP:=libtool
PKG_INSTALL:=
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DJSON_BuildTests=OFF # disable unit tests

define Package/nlohmann-json
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Nlohmann JSON
endef

define Package/blis/description
	nlohmann json
endef

define Build/Compile
	echo "empty build"
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/nlohmann
	$(CP) $(PKG_INSTALL_DIR)/usr/include/nlohmann/*.hpp $(1)/usr/include/nlohmann/
endef

$(eval $(call BuildPackage,nlohmann-json))

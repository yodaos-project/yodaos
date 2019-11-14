include $(TOPDIR)/rules.mk

PKG_NAME:=cJSON
PKG_VERSION:=1.7.7
PKG_RELEASE:=1
PKG_MAINTAINER:=DaveGamble

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/DaveGamble/cJSON.git
PKG_MIRROR_HASH:=9fe484dd954f6c573fee367ecea5aadfb743bee0a9d6aa917c29528b73fa5fa3
PKG_SOURCE_VERSION:=787d651e8131c6394c6ff844f108e1a53012949f
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/cJSON
  SECTION:=net
  CATEGORY:=Network
  TITLE:=cJSON
  DEPENDS:= +libpthread
  URL:=https://github.com/DaveGamble
endef

define Package/cJSON/description
    Ultralightweight JSON parser in ANSI C.
endef

CMAKE_OPTIONS += -DENABLE_CJSON_UTILS=On \
                -DENABLE_CJSON_TEST=On \
                -DBUILD_SHARED_AND_STATIC_LIBS=On\
                -DCMAKE_INSTALL_PREFIX=/usr

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/cjson $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcjson.{a,so*} $(1)/usr/lib/
endef

define Package/cJSON/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/cJSON_test $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcjson.so.* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libcjson_utils.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,cJSON))

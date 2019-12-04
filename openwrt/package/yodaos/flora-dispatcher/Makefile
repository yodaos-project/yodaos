
include $(TOPDIR)/rules.mk

PKG_NAME:=flora-dispatcher
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/services/flora-dispatcher

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/flora-dispatcher
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=flora-dispatcher
  DEPENDS:=+flora +mingutils
endef

define Package/flora-dispatcher/config
	source "$(SOURCE)/Config.in"
endef

define Package/flora-dispatcher/description
	property
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${ROKID_DIR}/framework/cmake-modules \
				 -DOVERRIDE_SOURCE_DIR=${PKG_SOURCE_DIR}
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/flora-dispatcher $(1)/usr/bin/
endef

define Package/flora-dispatcher/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/flora-dispatcher $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/flora-dispatcher $(1)/etc/init.d/flora-dispatcher
	if [ "$(CONFIG_FLORA_DISPATCHER_AUTO_RUN)" != "y" ]; then \
		$(SED) "s/^START/#START/g" $(1)/etc/init.d/flora-dispatcher; \
	fi
endef

$(eval $(call BuildPackage,flora-dispatcher))


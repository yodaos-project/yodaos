include $(TOPDIR)/rules.mk

PKG_NAME:=net_manager
PKG_VERSION:=
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/services/networking/manager

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/net_manager
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=net_manager
	DEPENDS:=+libstdcpp +wpa-supplicant +wpa_ctrl +libjson-c +property +android-system-core +libeloop_mini +flora +mutils +librt +rklog android-hardware
endef

define Package/net_manager/config
	source "$(SOURCE)/Config.in"
endef

define Package/net_manager/description
	net manager
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${PKG_SOURCE_DIR}/../cmake-modules
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/InstallDev
	echo "Here is net manager fileware Build/InstallDev"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/net_manager $(1)/usr/bin/
endef

define Package/net_manager/install
	echo "Here is net manager fileware Install"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/net_manager $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/net_manager $(1)/etc/init.d/net_manager
	if [ "$(CONFIG_NET_MANAGER_AUTO_RUN)" != "y" ]; then \
		$(SED) "s/^START/#START/g" $(1)/etc/init.d/net_manager; \
	fi
endef

$(eval $(call BuildPackage,net_manager))

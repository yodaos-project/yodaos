include $(TOPDIR)/rules.mk

PKG_NAME:=powermanager_service
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/services/powermanager_service


PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/powermanager_service
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=powermanager_service
	DEPENDS:=+libstdcpp +libjson-c +android-hardware +libeloop_mini +rklog +PACKAGE_flora:flora +PACKAGE_flora:mutils
endef

define Package/powermanager_service/description
	powermanager service
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${PKG_SOURCE_DIR}/../cmake-modules
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/InstallDev
	echo "Here is powermanager fileware Build/InstallDev"
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/powermanager_service $(1)/usr/bin/
endef

define Package/powermanager_service/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_BUILD_DIR)/src/powermanager_service $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/demo/rokid_reboot $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/demo/pms_test $(1)/usr/bin/
	$(CP) $(PKG_BUILD_DIR)/demo/pms_monitor $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(CP) ./pm_service $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,powermanager_service))


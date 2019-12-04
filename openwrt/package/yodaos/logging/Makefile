
include $(TOPDIR)/rules.mk

PKG_NAME:=logging
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/framework/libs/logging
PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/logging
  SECTION:=libs
  CATEGORY:=yodaos
  TITLE:=logging
  DEPENDS:=+libstdcpp +android-system-core +cJSON +flora +mutils +ubox +ubus +libubox
endef

define Package/logging/config
	source "$(SOURCE)/Config.in"
endef

define Package/logging/description
	logging lib
endef

TARGET_CFLAGS += $(FPIC)
CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

ifdef CONFIG_YODAOS_LOG_LEVEL
CMAKE_OPTIONS += -DBOARDCONFIG_LOG_LEVEL=${YODAOS_LOG_LEVEL}
endif

CMAKE_OPTIONS += \
	-DBUILD_SHARED_LIBS=ON \
	-DSTAGING_DIR=

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/rklog/
	$(CP) $(PKG_BUILD_DIR)/include/*.h $(1)/usr/include/rklog/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so $(1)/usr/lib/
endef

define Package/logging/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/set_log_level $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/rklogger $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/init.d/
endef

$(eval $(call BuildPackage,logging))

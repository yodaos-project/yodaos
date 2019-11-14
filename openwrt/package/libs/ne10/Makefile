
include $(TOPDIR)/rules.mk

PKG_NAME:=ne10
PKG_VERSION:=1.2.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/projectNe10/Ne10.git
PKG_SOURCE_VERSION:=v$(PKG_VERSION)

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/ne10
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=ne10
  DEPENDS:=+libstdcpp
endef

define Package/ne10/description
	ne10
endef

TARGET_CFLAGS += $(FPIC)

CMAKE_OPTIONS += \
	-DGNULINUX_PLATFORM=ON \
	-DNE10_LINUX_TARGET_ARCH=armv7a

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.a* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,ne10))

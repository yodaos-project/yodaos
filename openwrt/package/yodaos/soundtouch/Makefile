include $(TOPDIR)/rules.mk

PKG_NAME:=soundtouch
PKG_VERSION:=010a91a59071c7fefd316fca62c0d980ec85b4b1
PKG_RELEASE:=1.0

PKG_SOURCE_PROTO:=git
PKG_SOURCE_VERSION:=-b $(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_RELEASE).tar.gz
PKG_SOURCE_URL:=https://freeswitch.org/stash/scm/sd/libsoundtouch.git

PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.1+
PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/autotools.mk

define Package/soundtouch
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=soundtouch
  DEPENDS:=+libstdcpp
  URL:=https://freeswitch.org/stash/scm/sd/libsoundtouch.git
endef

define Package/soundtouch/description
	handle sound process library
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	       --enable-integer-samples

define Build/InstallDev
	echo "Here is soundtouch cpp fileware Build/InstallDev"
	$(INSTALL_DIR) $(1)/usr/include/soundtouch
	$(CP) $(PKG_BUILD_DIR)/include/* $(1)/usr/include/soundtouch
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libSoundTouch.so* $(1)/usr/lib/
endef

define Package/soundtouch/install
	echo "Here is soundtouch cpp fileware Build/install"
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libSoundTouch.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,soundtouch))



#
# Copyright (C) 2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=DirectFB
PKG_VERSION:=1.7.7
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://sources.webos-ports.org/
PKG_MD5SUM:=

PKG_LICENSE:=LGPLv2.1
PKG_LICENSE_FILES:=COPYING

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

define Package/directfb
    TITLE:=DirectFB
    SECTION:=libs
    CATEGORY:=Libraries
    URL:=http://directfb.org
    DEPENDS:=+libstdcpp +tslib +libpng +libjpeg +libpthread +libfreetype +libsysfs +flux
endef

define Package/directfb/description
    DirectFB is a thin library that provides hardware graphics acceleration, input device handling and abstraction, integrated windowing system with support for translucent windows and multiple display layers, not only on top of the Linux Framebuffer Device. It is a complete hardware abstraction layer with software fallbacks for every graphics operation that is not supported by the underlying hardware.
endef

TARGET_CFLAGS+="-Wno-error=format-security"
TARGET_LDFLAGS+="-Wl,-rpath-link=$(STAGING_DIR)/usr/lib"
CONFIGURE_CMD=./autogen.sh
CONFIGURE_VARS += \
  BUILD_CC="$(TARGET_CC)" \
  HOSTCC="$(HOSTCC)" \
  ac_cv_path_LIBPNG_CONFIG=$(STAGING_DIR)/usr/bin/libpng-config

define Feature/DirectFB
  TARGET_NAME:=drawing-backend
  TARGET_TITLE:=Drawing Backend
  NAME:=DirectFB
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--disable-osx \
		--disable-x11 \
		--enable-debug-support \
		--disable-network \
		--disable-mmx \
		--disable-sse \
		--enable-fbdev \
		--disable-sdl \
		--disable-vnc \
		--enable-sysfs \
		--with-inputdrivers=linuxinput \
		--with-gfxdrivers=none \
	)
endef

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/include \
		$(1)/usr/lib \
		$(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/* \
		$(1)/usr/lib/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/* \
		$(1)/usr/include/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig \
		$(1)/usr/lib/
endef

define Package/directfb/install
	$(INSTALL_DIR) \
		$(1)/usr/bin \
		$(1)/usr/lib \
		$(1)/usr/share/directfb-1.7.7 \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBFont \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBImageProvider \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBVideoProvider \
		$(1)/usr/lib/directfb-1.7-7/wm \
		$(1)/usr/lib/directfb-1.7-7/systems \
		$(1)/usr/lib/directfb-1.7-7/inputdrivers
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/bin/* \
		$(1)/usr/bin/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/share/directfb-1.7.7/* \
		$(1)/usr/share/directfb-1.7.7/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/lib{direct,fusion}*.so* \
		$(1)/usr/lib/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/interfaces/IDirectFBFont/*.so* \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBFont/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/interfaces/IDirectFBImageProvider/*.so* \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBImageProvider/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/interfaces/IDirectFBVideoProvider/*.so* \
		$(1)/usr/lib/directfb-1.7-7/interfaces/IDirectFBVideoProvider/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/wm/*.so* \
		$(1)/usr/lib/directfb-1.7-7/wm/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/systems/*.so* \
		$(1)/usr/lib/directfb-1.7-7/systems/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/directfb-1.7-7/inputdrivers/*.so* \
		$(1)/usr/lib/directfb-1.7-7/inputdrivers/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/lib++*.so* $(1)/usr/lib/
endef

$(eval $(call Feature,DirectFB))
$(eval $(call BuildPackage,directfb))

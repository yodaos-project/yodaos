#
# Copyright (C) 2006-2010 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=webrtc-audio-processing
PKG_VERSION:=0.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://freedesktop.org/software/pulseaudio/webrtc-audio-processing/

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/webrtc
	SECTION:=libs
	CATEGORY:=Libraries
	TITLE:=webrtc-audio-processing
	DEPENDS:=+libstdcpp
endef

define Package/webrtc/description
	webrtc-audio-processing
endef

TARGET_CFLAGS += $(FPIC)

define Build/Configure
	$(call Build/Configure/Default, \
		--enable-neon=yes \
		--enable-shared \
		--enable-static \
	)
endef

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		bin_PROGRAMS="" \
		all install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/webrtc_audio_processing/webrtc $(1)/usr/include/
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libwebrtc_audio_processing.{a,so*} $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/webrtc-audio-processing.pc $(1)/usr/lib/pkgconfig/
endef

define Package/webrtc/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libwebrtc_audio_processing.so.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,webrtc))

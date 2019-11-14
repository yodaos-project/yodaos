#
# Copyright (C) 2011-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=pulseaudio
PKG_VERSION:=10.0
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=http://freedesktop.org/software/pulseaudio/releases/
PKG_HASH:=a3186824de9f0d2095ded5d0d0db0405dc73133983c2fbb37291547e37462f57
PKG_LICENSE:=LGPL-2.1+
PKG_LICENSE_FILES:=GPL LICENSE

PKG_BUILD_DEPENDS:=intltool/host

PKG_FIXUP:=autoreconf
PKG_USE_MIPS16:=0

PKG_INSTALL = 1

PKG_CONFIG_DIR:=$(TOPDIR)/../products/$(CONFIG_PRODUCT_PATHNAME)/pulse

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk

define Package/pulseaudio_enable_bluez
  TITLE:=Pulseaudio Use Bluez
  SECTION:=sound
  CATEGORY:=Sound
  DEPENDS:= +bluez-daemon
endef

define Package/pulseaudio/Default
  SECTION:=sound
  CATEGORY:=Sound
  DEPENDS:= +libspeexdsp +libsoxr +libsndfile +libltdl +libpthread +librt +alsa-lib +PACKAGE_bluez-daemon:bluez-daemon\
    +libopenssl +libwrap +libcap +sbc +glib2 +dbus $(ICONV_DEPENDS) $(INTL_DEPENDS)
  TITLE:=Network sound server
  MAINTAINER:=Peter Wagner <tripolar@gmx.at>
  URL:=http://www.pulseaudio.org
  PROVIDES:=pulseaudio
  USERID:=pulse=51:pulse=51
endef

define Package/pulseaudio-daemon
  $(call Package/pulseaudio/Default)
endef

define Package/pulseaudio/Default/description
  PulseAudio (formerly Polypaudio) is a cross-platform, networked sound server.
endef

define Package/pulseaudio-daemon/description
  $(call Package/pulseaudio/Default/description)
endef

define Package/pulseaudio-daemon/conffiles
/etc/pulse/client.conf
/etc/pulse/daemon.conf
/etc/pulse/default.pa
/etc/pulse/system.pa
endef

define Package/pulseaudio-tools
  SECTION:=sound
  CATEGORY:=Sound
  DEPENDS:=+libsndfile +pulseaudio
  TITLE:=Tools for Pulseaudio
  URL:=http://www.pulseaudio.org
endef

define Package/pulseaudio-profiles
  SECTION:=sound
  CATEGORY:=Sound
  DEPENDS:=+pulseaudio
  TITLE:=Profiles for Pulseaudio
  URL:=http://www.pulseaudio.org
endef

ifeq ($(CONFIG_PACKAGE_pulseaudio_enable_bluez),y)
CONFIGURE_ARGS += \
	--with-system-user=pulse \
	--with-system-group=pulse \
	--with-access-group=audio \
	--with-database=simple \
	--enable-alsa \
	--disable-hal \
	--disable-gconf \
	--disable-nls \
	--disable-manpages \
	--enable-oss-output \
	--disable-oss-wrapper \
	--disable-samplerate \
	--disable-per-user-esound-socket \
	--disable-solaris \
	--disable-gtk2 \
	--disable-jack \
	--disable-asyncns \
	--disable-lirc \
	--enable-bluez5 \
	--enable-bluez5-ofono-headset \
	--enable-bluez5-native-headset \
	--disable-udev \
	--without-fftw \
	--disable-avahi \
	--enable-dbus \
	--without-soxr \
	--without-speex
else
CONFIGURE_ARGS += \
	--with-system-user=pulse \
	--with-system-group=pulse \
	--with-access-group=audio \
	--with-database=simple \
	--enable-alsa \
	--disable-hal \
	--disable-gconf \
	--disable-nls \
	--disable-manpages \
	--enable-oss-output \
	--disable-oss-wrapper \
	--disable-samplerate \
	--disable-per-user-esound-socket \
	--disable-solaris \
	--disable-gtk2 \
	--disable-jack \
	--disable-asyncns \
	--disable-lirc \
	--disable-bluez \
	--disable-bluez5 \
	--disable-bluez5-ofono-headset \
	--disable-bluez5-native-headset \
	--disable-udev \
	--without-fftw \
	--disable-avahi \
	--enable-dbus \
	--without-soxr \
	--without-speex
endif

CONFIGURE_VARS += \
	PKG_CONFIG_LIBDIR="$(STAGING_DIR)/usr/lib/pkgconfig"

#SUPP_LIBS:=-L$(INTL_PREFIX)/lib -L$(ICONV_PREFIX)/lib
TARGET_CFLAGS += -std=gnu99

define Build/InstallDev
	$(INSTALL_DIR) \
		$(1)/usr/lib/pkgconfig \
		$(1)/usr/include/pulse \
		$(1)/usr/lib \
		$(1)/usr/lib/pulseaudio
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/pulse/* \
		$(1)/usr/include/pulse
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/*.pc \
		$(1)/usr/lib/pkgconfig
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pulseaudio/* \
		$(1)/usr/lib/
endef

define Package/pulseaudio-daemon/install
	$(INSTALL_DIR) \
		$(1)/etc/pulse \
		$(1)/etc/init.d \
		$(1)/usr/bin \
		$(1)/usr/lib \
		$(1)/usr/lib/pulseaudio \
		$(1)/usr/lib/pulse-$(PKG_VERSION)/modules \
		$(1)/etc/dbus-1/system.d


	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/pulseaudio \
		$(1)/usr/bin/pulseaudio

	$(INSTALL_BIN) \
		./files/pulseaudio.init \
		$(1)/etc/init.d/pulseaudio

	$(INSTALL_DATA) \
		$(PKG_CONFIG_DIR)/* \
		$(1)/etc/pulse

	$(INSTALL_DATA) \
		./files/dbus/* \
		$(1)/etc/dbus-1/system.d

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pulseaudio/* \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pulse-$(PKG_VERSION)/modules/lib*.so \
		$(1)/usr/lib/

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/pulse-$(PKG_VERSION)/modules/module*.so \
		$(1)/usr/lib/pulse-$(PKG_VERSION)/modules/

endef

define Package/pulseaudio-tools/install
	$(INSTALL_DIR) \
		$(1)/usr/bin

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/pa* \
		$(1)/usr/bin/
endef

define Package/pulseaudio-profiles/install
	$(INSTALL_DIR) \
		$(1)/usr/share/pulseaudio/alsa-mixer/paths \
		$(1)/usr/share/pulseaudio/alsa-mixer/profile-sets

	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/pulseaudio/alsa-mixer/paths/* \
		$(1)/usr/share/pulseaudio/alsa-mixer/paths

	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/pulseaudio/alsa-mixer/profile-sets/* \
		$(1)/usr/share/pulseaudio/alsa-mixer/profile-sets
endef

$(eval $(call BuildPackage,pulseaudio-daemon))
$(eval $(call BuildPackage,pulseaudio-tools))
$(eval $(call BuildPackage,pulseaudio_enable_bluez))
$(eval $(call BuildPackage,pulseaudio-profiles))

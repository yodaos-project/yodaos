#
# Copyright (C) 2010-2016 OpenWrt.org
# Copyright (C) 2010 Alfred E. Heggestad
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=baresip
PKG_VERSION:=0.5.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.creytiv.com/pub/
PKG_MD5SUM:=4c364f4bb2cf17f83b6ff2b35962fd94258158202dfa566f68531e8ba77c1436

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=docs/COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

baresip-mods:= \
	alsa \
	avcodec \
	avformat \
	cons \
	evdev \
	g711 \
	g722 \
	g726 \
	opus \
	oss \
	stdio \
	v4l \
	v4l2

PKG_CONFIG_DEPENDS:= \
	$(patsubst %,CONFIG_PACKAGE_baresip-mod-%,$(baresip-mods)) \

include $(INCLUDE_DIR)/package.mk

define Package/baresip/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://www.creytiv.com/
endef

define Package/baresip
$(call Package/baresip/Default)
  TITLE:=Portable and modular SIP User-Agent with A/V support
  DEPENDS:=+libre +librem +libpthread
  MENU:=1
endef

baresip-mod-alsa      := USE_ALSA
baresip-mod-avcodec   := USE_AVCODEC
baresip-mod-avformat  := USE_AVFORMAT
baresip-mod-cons      := USE_CONS
baresip-mod-evdev     := USE_EVDEV
baresip-mod-g711      := USE_G711
baresip-mod-g722      := USE_G722
baresip-mod-g726      := USE_G726
baresip-mod-opus      := USE_OPUS
baresip-mod-oss       := USE_OSS
baresip-mod-stdio     := USE_STDIO
baresip-mod-v4l       := USE_V4L
baresip-mod-v4l2      := USE_V4L2

BARESIP_MOD_OPTIONS:= \
	MOD_AUTODETECT= \
	EXTRA_MODULES="dtmfio" \
	$(foreach m,$(baresip-mods),$(baresip-mod-$(m))=$(if $(CONFIG_PACKAGE_baresip-mod-$(m)),1))

TARGET_CFLAGS += $(FPIC) -D_GNU_SOURCE

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LIBRE_MK="$(STAGING_DIR)/usr/share/re/re.mk" \
		LIBRE_INC="$(STAGING_DIR)/usr/include/re" \
		LIBRE_SO="$(STAGING_DIR)/usr/lib" \
		LIBREM_PATH="../rem" \
		CC="$(TARGET_CC)" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		EXTRA_CFLAGS="$(TARGET_CFLAGS) -DOPENWRT" \
		SYSROOT="$$$$($(FIND) $(TOOLCHAIN_DIR) -path '*/include/pthread.h' | sed -ne '1s#/include/pthread.h##p')" \
		SYSROOT_ALT="$(STAGING_DIR)/usr" \
		RELEASE=1 \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		OS="linux" \
		$(BARESIP_MOD_OPTIONS) \
		all install
endef

define Package/baresip/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/baresip $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/usr/lib/baresip/modules
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/baresip/modules/{account,auloop,contact,ice,menu,stun,turn}.so \
		$(1)/usr/lib/baresip/modules/.
	$(INSTALL_DIR) $(1)/usr/share/baresip
	$(CP) $(PKG_INSTALL_DIR)/usr/share/baresip/* $(1)/usr/share/baresip/.
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/baresip.init $(1)/etc/init.d/baresip
endef

##################
# Baresip modules
# 1. Name
# 2. Title
# 3. Files
# 4. Dependencies
##################

define BuildPlugin

  define Package/baresip-mod-$(1)
    $$(call Package/baresip/Default)
    TITLE:= baresip $(2) module
    DEPENDS:= baresip $(4)
  endef

  define Package/baresip-mod-$(1)/install
	[ -z "$(3)" ] || $(INSTALL_DIR) $$(1)/usr/lib/baresip/modules
	for f in $(3); do \
		$(CP) \
			$(PKG_INSTALL_DIR)/usr/lib/baresip/modules/$$$$$$$${f}.so \
			$$(1)/usr/lib/baresip/modules/ ; \
	done
  endef

  $$(eval $$(call BuildPackage,baresip-mod-$(1)))
endef

$(eval $(call BuildPackage,baresip))

$(eval $(call BuildPlugin,alsa,ALSA audio driver,alsa,+PACKAGE_baresip-mod-alsa:alsa-lib))
$(eval $(call BuildPlugin,avcodec,FFmpeg video codecs,avcodec,+PACKAGE_baresip-mod-avcodec:libffmpeg-full))
$(eval $(call BuildPlugin,avformat,FFmpeg video source,avformat,+PACKAGE_baresip-mod-avformat:libffmpeg-full))
$(eval $(call BuildPlugin,cons,console UI,cons,))
$(eval $(call BuildPlugin,dtmfio,DTMF input/output,dtmfio,))
$(eval $(call BuildPlugin,evdev,input event device UI,evdev,))
$(eval $(call BuildPlugin,g711,G.711 audio codec,g711,))
$(eval $(call BuildPlugin,g722,G.722 audio codec,g722,+PACKAGE_baresip-mod-g722:libspandsp))
$(eval $(call BuildPlugin,g726,G.726 audio codec,g726,+PACKAGE_baresip-mod-g726:libspandsp))
$(eval $(call BuildPlugin,httpd,Webserver UI module,httpd,))
$(eval $(call BuildPlugin,opus,Opus audio codec,opus,+PACKAGE_baresip-mod-opus:libopus))
$(eval $(call BuildPlugin,oss,OSS audio driver,oss,))
$(eval $(call BuildPlugin,stdio,standard I/O UI,stdio,))
$(eval $(call BuildPlugin,v4l,Video4Linux video source,v4l,+PACKAGE_baresip-mod-v4l:libv4l))
$(eval $(call BuildPlugin,v4l2,Video4Linux2 video source,v4l2,+PACKAGE_baresip-mod-v4l2:libv4l))

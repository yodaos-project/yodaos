#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
# we use the $LINUX_DIR variable, which is defined in kernel.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=lirc
PKG_VERSION:=0.9.2a
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@SF/${PKG_NAME}
PKG_MD5SUM:=639a14ed0b3b34ae227a047b952ea368

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define Package/lirc/Default
    SECTION:=utils
    CATEGORY:=Utilities
    URL:=http://lirc.org/
endef

define Package/lirc
    $(call Package/lirc/Default)
    TITLE:=LIRC - Linux Infrared Remote Control
    DEPENDS:=+libpthread @!LINUX_2_6
    DEPENDS:=+libusb
    DEPENDS:=+libftdi
    MENU:=1
endef

define Package/lirc/description
  LIRC is a package that allows you to decode and send
  infra-red signals of many (but not all) commonly used
  remote controls.
  This package contains only lircd and lircd.conf!

tested drivers
--------------
ok:
lirc_gpioblaster

nok:
todo

unknown:
todo

endef

define  Package/lirctools
    $(call Package/lirc/Default)
    DEPENDS:=lirc
    TITLE:=LIRC tools
endef

define Package/lirctools/description
  This package contains those lirc-tools:
   -ircat:   prints config strings to standard output, can be used to provide remote control input to scripts
   -irexec:  execute programs according to the pressed remote control buttons
   -irpty:   pseudo tty driver. Connects to lircd via socket to receive infra-red codes and converts them to key strokes
   -irsend:  application for sending IR-codes via lirc
   -irw:     watch the codes as lircd recognize them
   -lircrcd: daemon that manages current mode for all applications
   -mode2:   shows the pulse/space length of a remote button
endef

define  Package/lirc-audioalsa
    $(call Package/lirc/Default)
    DEPENDS:=lirc +alsa-lib
    TITLE:=plugin audio_alsa
endef

define Package/lirc-audioalsa/description
  Contains this additional plugin:
   -plugin audio_alsa:
       Implemented routines for using a IR receiver connected to soundcard ADC.
       Uses ALSA sound interface. It does not require any kernel module. It uses
       alsa-lib for connecting to the audio card and detect codes coming from a
       sound line input.
endef

define  Package/lirc-devinput
    $(call Package/lirc/Default)
    DEPENDS:=lirc +kmod-hid
    TITLE:=plugin devinput
endef

define Package/lirc-devinput/description
  Contains this additional plugin:
   -plugin devinput:
       receive keycodes input via /dev/input/...
endef

define  Package/lirc-ftdi
    $(call Package/lirc/Default)
    DEPENDS:=lirc +libftdi +libusb
    TITLE:=plugin ftdi
endef

define Package/lirc-ftdi/description
  Contains this additional plugin:
   -plugin ftdi:
       Mode2 receiver + transmitter using the bitbang mode of an FTDI
       USB-to-serial chip such as the FT232R, with a demodulating IR receiver
       connected to one of the FTDI chips data pins -- by default, D1 (RXD).
endef

CONFIGURE_ARGS += \
	--disable-nls \
	--disable-static \
	--with-kerneldir="$(LINUX_DIR)" \
	--without-x \
	--with-pic \
	--with-gnu-ld  \
	--enable-sandboxed \
	--enable-dependency-tracking

MAKE_FLAGS += \
	DESTDIR="$(PKG_INSTALL_DIR)" \
	ARCH="$(LINUX_KARCH)"

define  Package/lirc/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/daemons/.libs/lircd $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/lirc
	$(INSTALL_CONF) ./files/lircd.conf $(1)/etc/lirc/
	$(INSTALL_DIR) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc_driver.so.0 $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc_client.so.0 $(1)/usr/lib/
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc_client.so.0.3.0 $(1)/usr/lib/
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc.so $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc.so.0 $(1)/usr/lib/
	#$(INSTALL_BIN) $(PKG_BUILD_DIR)/lib/.libs/liblirc.so.0.0.0 $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/lirc $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/lib/lirc/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugins/.libs/default.so $(1)/usr/lib/lirc/plugins
endef

define  Package/lirctools/install
	$(INSTALL_DIR) $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/irsend $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/irexec $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/irw $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/mode2 $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/ircat $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/irpty $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tools/.libs/irrecord $(1)/usr/sbin/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/daemons/.libs/lircrcd $(1)/usr/sbin/

endef

define  Package/lirc-audioalsa/install
	$(INSTALL_DIR) $(1)/usr/lib/lirc/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugins/.libs/audio_alsa.so $(1)/usr/lib/lirc/plugins
endef

define  Package/lirc-devinput/install
	$(INSTALL_DIR) $(1)/usr/lib/lirc/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugins/.libs/devinput.so $(1)/usr/lib/lirc/plugins
endef

define  Package/lirc-ftdi/install
	$(INSTALL_DIR) $(1)/usr/lib/lirc/plugins
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/plugins/.libs/ftdi.so $(1)/usr/lib/lirc/plugins
endef

$(eval $(call BuildPackage,lirc))
$(eval $(call BuildPackage,lirctools))
$(eval $(call BuildPackage,lirc-audioalsa))
$(eval $(call BuildPackage,lirc-devinput))
$(eval $(call BuildPackage,lirc-ftdi))

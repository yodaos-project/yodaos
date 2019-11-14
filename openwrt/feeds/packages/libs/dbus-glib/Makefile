#
# Copyright (C) 2007-2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
# $Id: Makefile 13844 2009-01-04 00:06:33Z lars $

include $(TOPDIR)/rules.mk

PKG_NAME:=dbus-glib
PKG_VERSION:=0.108
PKG_RELEASE:=3

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://dbus.freedesktop.org/releases/dbus-glib/
#PKG_MD5SUM:=d3b716a7e798faa1c6a867675f00306a
PKG_FIXUP:=libtool

include $(INCLUDE_DIR)/package.mk

PKG_INSTALL=1

define Package/libdbus-glib
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=D-Bus Glib bindings
  URL:=http://dbus.freedesktop.org/doc/dbus-glib/
  DEPENDS:=+dbus +glib2 +libiconv +libintl
endef

TARGET_CFLAGS += $(FPIC)

CONFIGURE_ARGS += \
	--enable-shared \
	--enable-static \
	--disable-tests \
	--disable-xml-docs \
	--disable-doxygen-docs \

CONFIGURE_VARS+= \
	ac_cv_func_posix_getpwnam_r="yes" \
	ac_cv_have_abstract_sockets="yes" \
	CPPFLAGS="$$$$CPPFLAGS -I$(STAGING_DIR)/usr/lib/libiconv/include" \
	CPPFLAGS="$$$$CPPFLAGS -I$(STAGING_DIR)/usr/lib/libintl/include" \
	LDFLAGS="$$$$LDFLAGS -L$(STAGING_DIR)/usr/lib/libiconv/lib" \
	LDFLAGS="$$$$LDFLAGS -L$(STAGING_DIR)/usr/lib/libintl/lib" \

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/dbus-1.0/dbus
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/dbus-1.0/dbus/*.h \
		$(1)/usr/include/dbus-1.0/dbus/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/libdbus-glib-1.{la,a,so*} \
		$(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/dbus-glib-1.pc \
		$(1)/usr/lib/pkgconfig/
endef

define Package/libdbus-glib/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libdbus-glib-1.so.* $(1)/usr/lib/
endef


$(eval $(call BuildPackage,libdbus-glib))


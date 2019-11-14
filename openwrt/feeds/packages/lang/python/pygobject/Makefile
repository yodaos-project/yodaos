#
# Copyright (C) 2008 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=pygobject
PKG_VERSION:=2.16.0
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_URL:=@GNOME/$(PKG_NAME)/2.16
PKG_MD5SUM:=431e7d4632163b93d1ee43cd071a389c
PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk
$(call include_mk, python-package.mk)

EXTRA_LDFLAGS+= \
	-L$(ICONV_PREFIX) -L$(INTL_PREFIX)

TARGET_CFLAGS += $(FPIC)
CONFIGURE_ARGS += --enable-static

define Package/python-gobject
  SUBMENU:=Python
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=GLib bindings for python
  URL:=http://www.pygtk.org/
  DEPENDS:=+python +python-expat +glib2 +libffi
endef

define Package/python-gobject/description
  GLib bindings for python
endef

define Build/Configure
	$(call Build/Configure/Default, \
		--with-libffi \
	)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/pygtk-2.0/
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/include/pygtk-2.0/* \
		$(1)/usr/include/pygtk-2.0/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/lib/pkgconfig/* \
		$(1)/usr/lib/pkgconfig/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig/
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.{so,a,la} \
		$(1)/usr/lib/

	$(SED) \
		"s,^\(prefix\|exec_prefix\|datadir\)=,\1=$(STAGING_DIR),g" \
		$(1)/usr/lib/pkgconfig/pygobject-2.0.pc

	$(INSTALL_DIR) $(1)/usr/share/pygobject/2.0/codegen
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/pygobject/2.0/codegen/*.py \
		$(1)/usr/share/pygobject/2.0/codegen

	$(INSTALL_DIR) $(1)/usr/share/pygobject/2.0/defs
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/share/pygobject/2.0/defs/* \
		$(1)/usr/share/pygobject/2.0/defs

	$(INSTALL_DIR) $(2)/bin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/* \
		$(2)/bin

	$(SED) \
		"s,^\(prefix\|datadir\)=,\1=$(STAGING_DIR),g" \
		$(2)/bin/pygobject-codegen-2.0
endef

define Package/python-gobject/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/*.so* \
		$(1)/usr/lib

	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/*.{pth,py} \
		$(1)$(PYTHON_PKG_DIR)/

	$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)/gtk-2.0
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/gtk-2.0/*.py \
		$(1)$(PYTHON_PKG_DIR)/gtk-2.0/

	$(foreach d,gobject gio glib, \
		$(INSTALL_DIR) $(1)$(PYTHON_PKG_DIR)/gtk-2.0/$(d); \
		$(INSTALL_DATA) \
			$(PKG_INSTALL_DIR)$(PYTHON_PKG_DIR)/gtk-2.0/$(d)/*.{py,so} \
			$(1)$(PYTHON_PKG_DIR)/gtk-2.0/$(d); \
	)
endef

$(eval $(call BuildPackage,python-gobject))

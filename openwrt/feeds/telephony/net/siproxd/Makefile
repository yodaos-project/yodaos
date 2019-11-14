#
# Copyright (C) 2014-2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=siproxd
PKG_VERSION:=0.8.2
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=@SF/siproxd
PKG_HASH:=526ce491b0cc189e2766c62432aff3ebb995e551d7261ea32c02a90c7bf7ccd0

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING

include $(INCLUDE_DIR)/package.mk

define Package/siproxd/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://siproxd.sourceforge.net/
endef

define Package/siproxd
  $(call Package/siproxd/Default)
  DEPENDS:=+libltdl +libpthread +libosip2
  TITLE:=SIP (Session Initiation Protocol) proxy
  MENU:=1
endef

define Package/siproxd/description
 Siproxd is a proxy/masquerading daemon for the SIP protocol.
endef

define Package/siproxd/conffiles
/etc/config/siproxd
endef

CONFIGURE_ARGS+= \
	--with-libosip-prefix="$(STAGING_DIR)/usr" \
	--disable-doc

MAKE_FLAGS+= \
	SUBDIRS="src scripts contrib" \
	LIBLTDL="$(STAGING_DIR)/usr/lib/libltdl.la" \

define Package/siproxd/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/siproxd $(1)/usr/sbin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/siproxd.config $(1)/etc/config/siproxd
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/siproxd.init $(1)/etc/init.d/siproxd
endef

define BuildPlugin
  define Package/siproxd-mod-$(1)
    $$(call Package/siproxd/Default)
    TITLE:= siproxd $(1) plugin
    DEPENDS:=siproxd
  endef

  define Package/siproxd-mod-$(1)/install
	$(INSTALL_DIR) $$(1)/usr/lib/siproxd
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/lib/siproxd/plugin_$(1)*.so* $$(1)/usr/lib/siproxd
  endef

  $$(eval $$(call BuildPackage,siproxd-mod-$(1)))
endef

$(eval $(call BuildPackage,siproxd))
$(eval $(call BuildPlugin,codecfilter))
$(eval $(call BuildPlugin,defaulttarget))
$(eval $(call BuildPlugin,demo))
$(eval $(call BuildPlugin,fix_bogus_via))
$(eval $(call BuildPlugin,fix_DTAG))
$(eval $(call BuildPlugin,fix_fbox_anoncall))
$(eval $(call BuildPlugin,logcall))
$(eval $(call BuildPlugin,prefix))
$(eval $(call BuildPlugin,regex))
$(eval $(call BuildPlugin,shortdial))
$(eval $(call BuildPlugin,stripheader))
$(eval $(call BuildPlugin,stun))
$(eval $(call BuildPlugin,siptrunk))

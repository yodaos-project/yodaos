#
# Copyright (C) 2010-2016 OpenWrt.org
# Copyright (C) 2010 Alfred E. Heggestad
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=restund
PKG_VERSION:=0.4.12
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.creytiv.com/pub/
PKG_MD5SUM:=8354dcdb92da406756520b7a2783c52a

PKG_LICENSE:=BSD-3-Clause
PKG_LICENSE_FILES:=docs/COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

restund-mods:= mysql

PKG_CONFIG_DEPENDS:= $(patsubst %,CONFIG_PACKAGE_restund-mod-%,$(restund-mods))

include $(INCLUDE_DIR)/package.mk

define Package/restund/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://www.creytiv.com/
endef

define Package/restund
$(call Package/restund/Default)
  TITLE:=Modular STUN/TURN server
  DEPENDS:=+libre +libpthread
endef

restund-mod-mysql:= USE_MYSQL

RESTUND_MOD_OPTIONS:= \
	$(foreach m,$(restund-mods),$(restund-mod-$(m))=$(if $(CONFIG_PACKAGE_restund-mod-$(m)),1))

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		LIBRE_MK="$(STAGING_DIR)/usr/share/re/re.mk" \
		LIBRE_INC="$(STAGING_DIR)/usr/include/re" \
		LIBRE_SO="$(STAGING_DIR)/usr/lib" \
		CC="$(TARGET_CC)" \
		EXTRA_CFLAGS="$(TARGET_CFLAGS)" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		SYSROOT="$$$$($(FIND) $(TOOLCHAIN_DIR) -path '*/include/pthread.h' | sed -ne '1s#/include/pthread.h##p')" \
		SYSROOT_ALT="$(STAGING_DIR)/usr" \
		RELEASE=1 \
		CROSS_COMPILE="$(TARGET_CROSS)" \
		OS="linux" \
		$(RESTUND_MOD_OPTIONS) \
		all install
endef

define Package/restund/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(CP) $(PKG_INSTALL_DIR)/usr/sbin/restund $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/lib/restund/modules
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/auth.so \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/binding.so \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/stat.so \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/status.so \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/syslog.so \
		$(PKG_INSTALL_DIR)/usr/lib/restund/modules/turn.so \
		$(1)/usr/lib/restund/modules/
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/etc/restund.conf $(1)/etc/
	$(SED) \
		's|^\(module_path\)\([ \t]\+\).*$$$$|\1\2/usr/lib/restund/modules|g' \
		$(1)/etc/restund.conf
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/restund.init $(1)/etc/init.d/restund
endef

define Package/restund/conffiles
/etc/restund.conf
endef

#
# 1. Name
# 2. Title
# 3. Files
# 4. Depends
#
define BuildPlugin

  define Package/restund-mod-$(1)
    $$(call Package/restund/Default)
    TITLE:= restund $(2) module
    DEPENDS:= restund $(4)
  endef

  define Package/restund-mod-$(1)/install
	[ -z "$(3)" ] || $(INSTALL_DIR) $$(1)/usr/lib/restund/modules
	for f in $(3); do \
		$(CP) \
			$(PKG_INSTALL_DIR)/usr/lib/restund/modules/$$$$$$$${f}.so \
			$$(1)/usr/lib/restund/modules/ ; \
	done
  endef

  $$(eval $$(call BuildPackage,restund-mod-$(1)))
endef

$(eval $(call BuildPackage,restund))

$(eval $(call BuildPlugin,mysql,MySQL database backend,mysql_ser,+PACKAGE_restund-mod-mysql:libmysqlclient))

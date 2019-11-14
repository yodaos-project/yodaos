#
# Copyright (C) 2016 - 2017 OpenWrt.org
# Copyright (C) 2016 Cesnet, z.s.p.o.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=chan-sccp-b
PKG_VERSION:=v4.3.0-20170814
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=git://github.com/chan-sccp/chan-sccp.git
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE_VERSION:=016fee3a96b09d5404757f7b178af2e349e55bcc
PKG_SOURCE_PROTO:=git

PKG_FIXUP:=autoreconf

PKG_LICENSE:=GPL-1.0
PKG_LICENSE_FILES:=COPYING LICENSE
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

PKG_INSTALL:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(BUILD_VARIANT)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/chan-sccp-b/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  TITLE:=SCCP channel provider support
  URL:=http://chan-sccp-b.sourceforge.net/
  DEPENDS:=+libltdl
  PKG_BUILD_DEPENDS:=libiconv
endef

define Package/asterisk13-chan-sccp-b
$(call Package/chan-sccp-b/Default)
  DEPENDS+= asterisk13
  VARIANT:=asterisk13
  CONFLICTS:=asterisk13-chan-skinny
endef

define Package/asterisk11-chan-sccp-b
$(call Package/chan-sccp-b/Default)
  DEPENDS+= asterisk11
  VARIANT:=asterisk11
  CONFLICTS:=asterisk11-chan-skinny
endef

define Package/description/Default
 SCCP channel provider for asterisk. It delivers extended functionality for SCCP phones over chan_skinny delivered
 by asterisk by default.
endef

Package/asterisk11-chan-sccp-b/description = $(Package/description/Default)
Package/asterisk13-chan-sccp-b/description = $(Package/description/Default)

CONFIGURE_ARGS += \
	--disable-debug \
	--enable-advanced-functions \
	--enable-conference \
	--enable-video

ifeq ($(BUILD_VARIANT),asterisk13)
  CONFIGURE_ARGS += --with-asterisk=$(STAGING_DIR)/usr/include/asterisk-13
endif

ifeq ($(BUILD_VARIANT),asterisk11)
  CONFIGURE_ARGS += --with-asterisk=$(STAGING_DIR)/usr/include/asterisk-11
endif

define Package/conffiles/Default
/etc/asterisk/sccp.conf
/etc/asterisk/extconfig.conf.sccp_sample
/etc/asterisk/res_config_sqlite3.conf.sccp_sample
/etc/asterisk/sccp_sqlite3.sql
/etc/asterisk/sccp.conf.realtime_sample
endef

Package/asterisk11-chan-sccp-b/conffiles = $(Package/conffiles/Default)
Package/asterisk13-chan-sccp-b/conffiles = $(Package/conffiles/Default)

define Package/Install/Default
	$(INSTALL_DIR) $(1)/etc/asterisk
	$(CP) ./files/sccp.conf $(1)/etc/asterisk/sccp.conf
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/asterisk/modules/chan_sccp.so $(1)/usr/lib/asterisk/modules/
	$(INSTALL_DATA) ./files/extconfig.conf.sccp_sample $(1)/etc/asterisk/extconfig.conf.sccp_sample
	$(INSTALL_DATA) ./files/res_config_sqlite3.conf.sccp_sample $(1)/etc/asterisk/res_config_sqlite3.conf.sccp_sample
	$(INSTALL_DATA) ./files/sccp_sqlite3.sql $(1)/etc/asterisk/sccp_sqlite3.sql
	$(INSTALL_DATA) ./files/sccp.conf.realtime_sample $(1)/etc/asterisk/sccp.conf.realtime_sample
endef

Package/asterisk11-chan-sccp-b/install = $(Package/Install/Default)
Package/asterisk13-chan-sccp-b/install = $(Package/Install/Default)

$(eval $(call BuildPackage,asterisk11-chan-sccp-b))
$(eval $(call BuildPackage,asterisk13-chan-sccp-b))

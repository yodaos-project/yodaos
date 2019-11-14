#
# Copyright (C) 2013-2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=sipp
PKG_VERSION:=3.5.1
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/SIPp/sipp/releases/download/v$(PKG_VERSION)
PKG_HASH:=56421ba7b43b67e9b04e21894b726502a82a6149fc86ba06df33dfc7252a1891

PKG_LICENSE:=GPL-2.0+ BSD-3-Clause Zlib
PKG_LICENSE_FILES:=LICENSE.txt

PKG_FIXUP:=autoreconf

include $(INCLUDE_DIR)/package.mk

define Package/sipp
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  DEPENDS:=+libncurses +libpthread +libstdcpp
  TITLE:=test tool / traffic generator for the SIP protocol
  URL:=http://sipp.sourceforge.net/
endef

define Package/sipp/description
 SIPp is a free Open Source test tool / traffic generator for the SIP
 protocol. It includes a few basic SipStone user agent scenarios (UAC and
 UAS) and establishes and releases multiple calls with the INVITE and BYE
 methods.
endef

CONFIGURE_ARGS+= \
	--enable-epoll \
	--without-gsl \
	--without-pcap \
	--with-rtpstream \
	--without-sctp

define Package/sipp/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/sipp $(1)/usr/bin
endef

$(eval $(call BuildPackage,sipp))

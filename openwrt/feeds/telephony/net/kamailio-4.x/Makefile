#
# Copyright (C) 2006-2017 OpenWrt.org
# Copyright (C) 2013-2016 CESNET,z.s.p.o.
# Copyright (C) 2017 Jiri Slachta
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=kamailio4
PKG_VERSION:=4.4.6
PKG_RELEASE:=1

PKG_SOURCE_URL:=http://www.kamailio.org/pub/kamailio/$(PKG_VERSION)/src/
PKG_SOURCE:=kamailio-$(PKG_VERSION)$(PKG_VARIANT)_src.tar.gz
PKG_HASH:=
PKG_USE_MIPS16:=0

PKG_LICENSE:=GPL-2.0+
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/nls.mk
include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += $(FPIC)
PREBUILT_STAMP=$(STAGING_DIR)/stamp/.$(PKG_NAME)_prebuilt
TAR_CMD:=$(HOST_TAR) -C $(PKG_BUILD_DIR) --strip-components 1 $(TAR_OPTIONS)

define Package/kamailio4/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://www.kamailio.org/
  DEPENDS:=$(ICONV_DEPENDS) +libncurses +libpthread +libreadline +libxml2
endef

define Package/kamailio4
$(call Package/kamailio4/Default)
  TITLE:=Mature and flexible open source SIP server, v$(PKG_VERSION)
  MENU:=1
endef

define Package/kamailio4/conffiles
/etc/default/kamailio
/etc/kamailio/kamailio.cfg
/etc/kamailio/dictionary.kamailio
/etc/kamailio/kamctlrc
endef

define Package/kamailio4/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/kam{ailio,cmd,ctl,dbctl} \
		$(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/usr/lib/kamailio/modules
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/lib/kamailio/lib*.so* \
		$(1)/usr/lib/kamailio/
	$(INSTALL_DIR) $(1)/etc/kamailio
	$(CP) \
		$(PKG_BUILD_DIR)/etc/{dictionary.kamailio,kamailio.cfg,kamailio-basic.cfg} \
		$(1)/etc/kamailio/
	$(CP) \
		$(PKG_BUILD_DIR)/utils/kamctl/kamctlrc \
		$(1)/etc/kamailio/
	$(INSTALL_DIR) $(1)/etc/default
	$(CP) \
		./files/kamailio.default \
		$(1)/etc/default/kamailio
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) \
		./files/kamailio.init \
		$(1)/etc/init.d/kamailio
	$(INSTALL_DIR) $(1)/usr/lib/kamailio
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/lib/kamailio/kamctl \
		$(1)/usr/lib/kamailio/
endef

define Package/kamailio4/install/dbfiles
	$(INSTALL_DIR) $(1)/usr/share/kamailio/$(2)
	$(CP) $(PKG_INSTALL_DIR)/usr/share/kamailio/$(2)/* $(1)/usr/share/kamailio/$(2)/
endef

define Package/kamailio4/install/module
	$(INSTALL_DIR) $(1)/usr/lib/kamailio/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/kamailio/modules/$(2).so* $(1)/usr/lib/kamailio/modules/
endef

define BuildKamailio4Module
  define Package/kamailio4-mod-$(subst _,-,$(1))
    $$(call Package/kamailio4/Default)
    TITLE:= kamailio4 $(2) module
    DEPENDS:=kamailio4 $(4)
  endef

  define Package/kamailio4-mod-$(subst _,-,$(1))/install
$(call Package/kamailio4/install/module,$$(1),$(1))
$(foreach d,$(5),$(call Package/kamailio4/install/dbfiles,$$(1),$(d));)
  endef

  INCL_MODULES+=$(1)
  KAM_MODULES+=kamailio4-mod-$(subst _,-,$(1))
endef

################################
# Kamailio module parameters
# Params:
# 1 - Module name
# 2 - Module title
# 3 - Module description
# 4 - Dependencies
# 5 - Kamailio DB files
################################

$(eval $(call BuildKamailio4Module,acc,Accounting,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,alias_db,Database-backend aliases,,+kamailio4-mod-db-sqlite))
$(eval $(call BuildKamailio4Module,auth,Authentication Framework,,))
$(eval $(call BuildKamailio4Module,auth_db,Database-backend authentication,,+kamailio4-mod-auth +kamailio4-mod-db-sqlite))
$(eval $(call BuildKamailio4Module,auth_diameter,Diameter-backend authentication,,+kamailio4-mod-sl))
$(eval $(call BuildKamailio4Module,auth_xkeys,Shared-key authentication,,+kamailio4-mod-auth))
$(eval $(call BuildKamailio4Module,avpops,AVP operation,,))
$(eval $(call BuildKamailio4Module,benchmark,Config benchmark,,))
$(eval $(call BuildKamailio4Module,cfgutils,Config utilities,,))
$(eval $(call BuildKamailio4Module,cfg_db,Load core and module parameters from database,,+kamailio4-mod-db-sqlite))
$(eval $(call BuildKamailio4Module,cfg_rpc,Update core and module parameters at runtime via RPC interface,,))
$(eval $(call BuildKamailio4Module,cnxcc,Limit call duration,,+kamailio4-mod-dialog +libhiredis +libevent2))
$(eval $(call BuildKamailio4Module,corex,Legacy functions,,))
$(eval $(call BuildKamailio4Module,ctl,BINRPC transport interface,,))
$(eval $(call BuildKamailio4Module,db_flatstore,Fast writing-only text database-backed,,))
$(eval $(call BuildKamailio4Module,db_mysql,MySQL database-backend,,+libmysqlclient,mysql))
$(eval $(call BuildKamailio4Module,db_postgres,PostgreSQL Database-backend,,+libpq,postgres))
$(eval $(call BuildKamailio4Module,db_sqlite,Sqlite DB support,,+libsqlite3,db_sqlite))
$(eval $(call BuildKamailio4Module,db_text,Text database-backend,,,dbtext/kamailio))
$(eval $(call BuildKamailio4Module,db_unixodbc,UnixODBC Database-backend,,+unixodbc))
$(eval $(call BuildKamailio4Module,debugger,Interactive config file debugger,,))
$(eval $(call BuildKamailio4Module,dialog,Dialog support,,+kamailio4-mod-rr +kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,dialog_ng,Dialog support,,+kamailio4-mod-rr +kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,dialplan,Dialplan management,,))
$(eval $(call BuildKamailio4Module,dispatcher,Dispatcher,,))
$(eval $(call BuildKamailio4Module,diversion,Diversion header insertion,,))
$(eval $(call BuildKamailio4Module,domain,Multi-domain support,,))
$(eval $(call BuildKamailio4Module,domainpolicy,Domain policy,,))
$(eval $(call BuildKamailio4Module,drouting,Dynamic routing module,,))
$(eval $(call BuildKamailio4Module,enum,ENUM lookup,,))
$(eval $(call BuildKamailio4Module,evapi,push event details via tcp,,+libev))
$(eval $(call BuildKamailio4Module,exec,External exec,,))
$(eval $(call BuildKamailio4Module,group,Database-backend user-groups,,))
$(eval $(call BuildKamailio4Module,h350,H.350,,+kamailio4-mod-ldap +libopenldap))
$(eval $(call BuildKamailio4Module,htable,Hash Table,,))
$(eval $(call BuildKamailio4Module,imc,IM conferencing,,+kamailio4-mod-db-mysql +kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,ipops,IP and IPv6 operations,,))
$(eval $(call BuildKamailio4Module,jansson,Alternative access to JSON document attributes,,+jansson))
$(eval $(call BuildKamailio4Module,janssonrpc-c,Alternative JSONRPC server,,+kamailio4-mod-jansson +libevent2))
$(eval $(call BuildKamailio4Module,json,Access to JSON document attributes,,+libjson-c))
$(eval $(call BuildKamailio4Module,jsonrpc-s,JSONRPC server over HTTP,,+kamailio4-mod-json +libevent2))
$(eval $(call BuildKamailio4Module,kex,Core extensions,,))
$(eval $(call BuildKamailio4Module,lcr,Least Cost Routing,,+kamailio4-mod-tm +libpcre))
$(eval $(call BuildKamailio4Module,ldap,LDAP connector,,+libopenldap))
$(eval $(call BuildKamailio4Module,maxfwd,Max-Forward processor,,))
$(eval $(call BuildKamailio4Module,mediaproxy,Automatic NAT traversal,,+kamailio4-mod-dialog))
$(eval $(call BuildKamailio4Module,mi_datagram,Datagram support for Management Interface,,))
$(eval $(call BuildKamailio4Module,mi_fifo,FIFO support for Management Interface,,))
$(eval $(call BuildKamailio4Module,mi_rpc,RPC support for Management Interface,,))
$(eval $(call BuildKamailio4Module,msilo,SIP message silo,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,msrp,MSRP routing engine,,+kamailio4-mod-tls))
$(eval $(call BuildKamailio4Module,nathelper,NAT helper,,+kamailio4-mod-usrloc))
$(eval $(call BuildKamailio4Module,nat_traversal,NAT traversal,,+kamailio4-mod-dialog +kamailio4-mod-sl +kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,nosip,non-sip package handling,,+kamailio4-mod-rr))
$(eval $(call BuildKamailio4Module,path,SIP path insertion,,+kamailio4-mod-rr))
$(eval $(call BuildKamailio4Module,pdt,Prefix-to-Domain translator,,))
$(eval $(call BuildKamailio4Module,permissions,Permissions control,,))
$(eval $(call BuildKamailio4Module,pike,Flood detector,,))
$(eval $(call BuildKamailio4Module,presence,Presence server,,+kamailio4-mod-sl +kamailio4-mod-tm +libxml2))
$(eval $(call BuildKamailio4Module,presence_dialoginfo,Dialog Event presence,,+kamailio4-mod-presence))
$(eval $(call BuildKamailio4Module,presence_mwi,Message Waiting Indication presence,,+kamailio4-mod-presence))
$(eval $(call BuildKamailio4Module,presence_xml,XCAP presence,,+kamailio4-mod-presence +kamailio4-mod-xcap-client))
$(eval $(call BuildKamailio4Module,pua,Presence User Agent,,+kamailio4-mod-tm +libxml2))
$(eval $(call BuildKamailio4Module,pua_bla,Bridged Line Appearence PUA,,+kamailio4-mod-presence +kamailio4-mod-pua +kamailio4-mod-usrloc))
$(eval $(call BuildKamailio4Module,pua_dialoginfo,Dialog Event PUA,,+kamailio4-mod-dialog +kamailio4-mod-pua))
$(eval $(call BuildKamailio4Module,pua_mi,PUA Management Interface,,+kamailio4-mod-pua))
$(eval $(call BuildKamailio4Module,pua_usrloc,PUA User Location,,+kamailio4-mod-pua +kamailio4-mod-usrloc))
$(eval $(call BuildKamailio4Module,pua_xmpp,PUA XMPP,,+kamailio4-mod-presence +kamailio4-mod-pua +kamailio4-mod-xmpp))
$(eval $(call BuildKamailio4Module,pv,Pseudo-Variables,,))
$(eval $(call BuildKamailio4Module,qos,QoS control,,+kamailio4-mod-dialog))
$(eval $(call BuildKamailio4Module,ratelimit,Traffic shapping,,))
$(eval $(call BuildKamailio4Module,regex,Regular Expression,,+libpcre))
$(eval $(call BuildKamailio4Module,registrar,SIP Registrar,,+kamailio4-mod-usrloc))
$(eval $(call BuildKamailio4Module,rls,Resource List Server,,+kamailio4-mod-presence +kamailio4-mod-pua +kamailio4-mod-tm +libxml2))
$(eval $(call BuildKamailio4Module,rr,Record-Route and Route,,))
$(eval $(call BuildKamailio4Module,rtimer,Routing Timer,,))
$(eval $(call BuildKamailio4Module,rtpengine,RTP engine,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,rtpproxy,RTP proxy,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,sanity,SIP sanity checks,,+kamailio4-mod-sl))
$(eval $(call BuildKamailio4Module,sctp,SCTP support,,+libsctp))
$(eval $(call BuildKamailio4Module,sipcapture,SIP capture,,))
$(eval $(call BuildKamailio4Module,siptrace,SIP trace,,))
$(eval $(call BuildKamailio4Module,siputils,SIP utilities,,+kamailio4-mod-sl))
$(eval $(call BuildKamailio4Module,sl,Stateless replier,,))
$(eval $(call BuildKamailio4Module,sms,SIP-to-SMS IM gateway,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,speeddial,Per-user speed-dial controller,,))
$(eval $(call BuildKamailio4Module,sqlops,SQL operations,,))
$(eval $(call BuildKamailio4Module,statistics,Script statistics,,))
$(eval $(call BuildKamailio4Module,stun,STUN server support,,))
$(eval $(call BuildKamailio4Module,sst,SIP Session Timer,,+kamailio4-mod-dialog +kamailio4-mod-sl))
$(eval $(call BuildKamailio4Module,tcpops,TCP options tweaking operations,,))
$(eval $(call BuildKamailio4Module,textops,Text operations,,))
$(eval $(call BuildKamailio4Module,tls,TLS operations,,+libopenssl))
$(eval $(call BuildKamailio4Module,topoh,Topology hiding,,+kamailio4-mod-rr))
$(eval $(call BuildKamailio4Module,tm,Transaction,,))
$(eval $(call BuildKamailio4Module,tmx,Transaction module extensions,,))
$(eval $(call BuildKamailio4Module,uac,User Agent Client,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,uac_redirect,User Agent Client redirection,,+kamailio4-mod-tm))
$(eval $(call BuildKamailio4Module,uri_db,Database-backend SIP URI checking,,))
$(eval $(call BuildKamailio4Module,userblacklist,User blacklists,,))
$(eval $(call BuildKamailio4Module,usrloc,User location,,))
$(eval $(call BuildKamailio4Module,utils,Misc utilities,,+libcurl +libxml2))
$(eval $(call BuildKamailio4Module,uuid,UUID utilities,,+libuuid))
$(eval $(call BuildKamailio4Module,xcap_client,XCAP Client,,+libcurl))
$(eval $(call BuildKamailio4Module,xlog,Advanced logger,,))
$(eval $(call BuildKamailio4Module,xmlrpc,XML RPC module,,+libxml2))
$(eval $(call BuildKamailio4Module,xmpp,SIP-to-XMPP Gateway,,+kamailio4-mod-tm +libexpat))

PKG_MAKE_ARGS:= \
	prefix=/ \
	cfg_dir=/etc/kamailio/ \
	bin_dir=/usr/sbin/ \
	data_dir=/usr/share/kamailio/ \
	lib_dir=/usr/lib/kamailio/ \
	modules_dir=/usr/lib/kamailio/ \
	include_modules="$(INCL_MODULES)" \
	cfg_target:=/etc/kamailio/ \
	TLS_HOOKS=1 \
	extra_defs="-DUSE_PTHREAD_MUTEX " \
	CFLAGS="$(TARGET_CFLAGS)" \
	LOCALBASE="$(STAGING_DIR)/usr" \
	SYSBASE="$(STAGING_DIR)/usr" \
	PCREDEFS:="$(TARGET_CPPFLAGS)" \
	PCRELIBS:="$(TARGET_LDFLAGS)" \
	CROSS_COMPILE=$(TARGET_CROSS) \
	CC="$(TARGET_CC)" \
	ARCH="$(ARCH)"  \
	DESTDIR=$(PKG_INSTALL_DIR) \
	EXTRA_LIBS="-L$(STAGING_DIR)/usr/lib/" \
	quiet=verbose

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) $(PKG_MAKE_ARGS) all
	$(MAKE) -C $(PKG_BUILD_DIR) $(PKG_MAKE_ARGS) install
endef

$(eval $(call BuildPackage,kamailio4))
$(foreach m,$(KAM_MODULES),$(eval $(call BuildPackage,$(m))))

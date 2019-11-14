#
# Copyright (C) 2017 OpenWrt.org
# Copyright (C) 2017 Jiri Slachta
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=kamailio5
PKG_VERSION:=5.0.2
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

define Package/kamailio5/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://www.kamailio.org/
  DEPENDS:=$(ICONV_DEPENDS) +libncurses +libpthread +libreadline +libxml2
endef

define Package/kamailio5
$(call Package/kamailio5/Default)
  TITLE:=Mature and flexible open source SIP server, v$(PKG_VERSION)
  MENU:=1
endef

define Package/kamailio5/conffiles
/etc/default/kamailio
/etc/kamailio/kamailio.cfg
/etc/kamailio/dictionary.kamailio
/etc/kamailio/kamctlrc
endef

define Package/kamailio5/install
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

define Package/kamailio5/install/dbfiles
	$(INSTALL_DIR) $(1)/usr/share/kamailio/$(2)
	$(CP) $(PKG_INSTALL_DIR)/usr/share/kamailio/$(2)/* $(1)/usr/share/kamailio/$(2)/
endef

define Package/kamailio5/install/module
	$(INSTALL_DIR) $(1)/usr/lib/kamailio/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/kamailio/modules/$(2).so* $(1)/usr/lib/kamailio/modules/
endef

define BuildKamailio5Module
  define Package/kamailio5-mod-$(subst _,-,$(1))
    $$(call Package/kamailio5/Default)
    TITLE:=$(2) for Kamailio5
    DEPENDS:=kamailio5 $(4)
  endef

  define Package/kamailio5-mod-$(subst _,-,$(1))/install
$(call Package/kamailio5/install/module,$$(1),$(1))
$(foreach d,$(5),$(call Package/kamailio5/install/dbfiles,$$(1),$(d));)
  endef

  INCL_MODULES+=$(1)
  KAM_MODULES+=kamailio5-mod-$(subst _,-,$(1))
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

$(eval $(call BuildKamailio5Module,acc,Accounting,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,alias_db,Database-backend aliases,,+kamailio5-mod-db-sqlite))
$(eval $(call BuildKamailio5Module,app_lua,Execute embedded Lua scripts,,+liblua))
$(eval $(call BuildKamailio5Module,async,Asynchronous SIP handling functions,,+kamailio5-mod-tm +kamailio5-mod-tmx))
$(eval $(call BuildKamailio5Module,auth,Authentication Framework,,))
$(eval $(call BuildKamailio5Module,auth_db,Database-backend authentication,,+kamailio5-mod-auth +kamailio5-mod-db-sqlite))
$(eval $(call BuildKamailio5Module,auth_diameter,Diameter authentication,,+kamailio5-mod-sl))
$(eval $(call BuildKamailio5Module,auth_identity,Identity authentication,,+libopenssl +libcurl))
$(eval $(call BuildKamailio5Module,auth_xkeys,Shared-key authentication,,+kamailio5-mod-auth))
$(eval $(call BuildKamailio5Module,avp,Functions for handling AVPs,,))
$(eval $(call BuildKamailio5Module,avpops,AVP operation,,))
$(eval $(call BuildKamailio5Module,benchmark,Config benchmark,,))
$(eval $(call BuildKamailio5Module,blst,Blacklisting API for config,,))
$(eval $(call BuildKamailio5Module,cfgutils,Config utilities,,))
$(eval $(call BuildKamailio5Module,cfg_db,Load parameters from database,,+kamailio5-mod-db-sqlite))
$(eval $(call BuildKamailio5Module,cfg_rpc,Update parameters via RPC,,))
$(eval $(call BuildKamailio5Module,cnxcc,Limit call duration,,+kamailio5-mod-dialog +libhiredis +libevent2))
$(eval $(call BuildKamailio5Module,corex,Legacy functions,,))
$(eval $(call BuildKamailio5Module,ctl,BINRPC transport interface,,))
$(eval $(call BuildKamailio5Module,db_flatstore,fast write-only text DB-backend,,))
$(eval $(call BuildKamailio5Module,db_mysql,MySQL DB-backend,,+libmysqlclient,mysql))
$(eval $(call BuildKamailio5Module,db_postgres,PostgreSQL DB-backend,,+libpq,postgres))
$(eval $(call BuildKamailio5Module,db_sqlite,SQLite DB-backend,,+libsqlite3,db_sqlite))
$(eval $(call BuildKamailio5Module,db_text,Text DB-backend,,,dbtext/kamailio))
$(eval $(call BuildKamailio5Module,db_unixodbc,UnixODBC DB-backend,,+unixodbc))
$(eval $(call BuildKamailio5Module,debugger,Interactive config file debugger,,))
$(eval $(call BuildKamailio5Module,dialog,Dialog support,,+kamailio5-mod-rr +kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,dialplan,Dialplan management,,))
$(eval $(call BuildKamailio5Module,dispatcher,Dispatcher,,))
$(eval $(call BuildKamailio5Module,diversion,Diversion header insertion,,))
$(eval $(call BuildKamailio5Module,domain,Multi-domain support,,))
$(eval $(call BuildKamailio5Module,domainpolicy,Domain policy,,))
$(eval $(call BuildKamailio5Module,drouting,Dynamic routing module,,))
$(eval $(call BuildKamailio5Module,enum,ENUM lookup,,))
$(eval $(call BuildKamailio5Module,evapi,push event details via tcp,,+libev))
$(eval $(call BuildKamailio5Module,exec,External exec,,))
$(eval $(call BuildKamailio5Module,group,Database-backend user-groups,,))
$(eval $(call BuildKamailio5Module,h350,H.350,,+kamailio5-mod-ldap +libopenldap))
$(eval $(call BuildKamailio5Module,htable,Hash Table,,))
$(eval $(call BuildKamailio5Module,imc,IM conferencing,,+kamailio5-mod-db-mysql +kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,ipops,IP and IPv6 operations,,))
$(eval $(call BuildKamailio5Module,jansson,Access to JSON attributes,,+jansson))
$(eval $(call BuildKamailio5Module,janssonrpcc,Alternative JSONRPC server,,+kamailio5-mod-jansson +libevent2))
$(eval $(call BuildKamailio5Module,json,Access to JSON document attributes,,+libjson-c))
$(eval $(call BuildKamailio5Module,jsonrpcs,JSONRPC server over HTTP,,+kamailio5-mod-json +libevent2))
$(eval $(call BuildKamailio5Module,kex,Core extensions,,))
$(eval $(call BuildKamailio5Module,lcr,Least Cost Routing,,+kamailio5-mod-tm +libpcre))
$(eval $(call BuildKamailio5Module,ldap,LDAP connector,,+libopenldap))
$(eval $(call BuildKamailio5Module,maxfwd,Max-Forward processor,,))
$(eval $(call BuildKamailio5Module,mediaproxy,Automatic NAT traversal,,+kamailio5-mod-dialog))
$(eval $(call BuildKamailio5Module,msilo,SIP message silo,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,msrp,MSRP routing engine,,+kamailio5-mod-tls))
$(eval $(call BuildKamailio5Module,nathelper,NAT helper,,+kamailio5-mod-usrloc))
$(eval $(call BuildKamailio5Module,nat_traversal,NAT traversal,,+kamailio5-mod-dialog +kamailio5-mod-sl +kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,nosip,non-sip package handling,,+kamailio5-mod-rr))
$(eval $(call BuildKamailio5Module,path,SIP path insertion,,+kamailio5-mod-rr))
$(eval $(call BuildKamailio5Module,pdt,Prefix-to-Domain translator,,))
$(eval $(call BuildKamailio5Module,permissions,Permissions control,,))
$(eval $(call BuildKamailio5Module,pike,Flood detector,,))
$(eval $(call BuildKamailio5Module,presence,Presence server,,+kamailio5-mod-sl +kamailio5-mod-tm +libxml2))
$(eval $(call BuildKamailio5Module,presence_dialoginfo,Dialog Event presence,,+kamailio5-mod-presence))
$(eval $(call BuildKamailio5Module,presence_mwi,MWI presence,,+kamailio5-mod-presence))
$(eval $(call BuildKamailio5Module,presence_xml,XCAP presence,,+kamailio5-mod-presence +kamailio5-mod-xcap-client))
$(eval $(call BuildKamailio5Module,pua,Presence User Agent,,+kamailio5-mod-tm +libxml2))
$(eval $(call BuildKamailio5Module,pua_bla,Bridged Line Appearence PUA,,+kamailio5-mod-presence +kamailio5-mod-pua +kamailio5-mod-usrloc))
$(eval $(call BuildKamailio5Module,pua_dialoginfo,Dialog Event PUA,,+kamailio5-mod-dialog +kamailio5-mod-pua))
$(eval $(call BuildKamailio5Module,pua_usrloc,PUA User Location,,+kamailio5-mod-pua +kamailio5-mod-usrloc))
$(eval $(call BuildKamailio5Module,pua_xmpp,PUA XMPP,,+kamailio5-mod-presence +kamailio5-mod-pua +kamailio5-mod-xmpp))
$(eval $(call BuildKamailio5Module,pv,Pseudo-Variables,,))
$(eval $(call BuildKamailio5Module,qos,QoS control,,+kamailio5-mod-dialog))
$(eval $(call BuildKamailio5Module,ratelimit,Traffic shapping,,))
$(eval $(call BuildKamailio5Module,regex,Regular Expression,,+libpcre))
$(eval $(call BuildKamailio5Module,registrar,SIP Registrar,,+kamailio5-mod-usrloc))
$(eval $(call BuildKamailio5Module,rls,Resource List Server,,+kamailio5-mod-presence +kamailio5-mod-pua +kamailio5-mod-tm +libxml2))
$(eval $(call BuildKamailio5Module,rr,Record-Route and Route,,))
$(eval $(call BuildKamailio5Module,rtimer,Routing Timer,,))
$(eval $(call BuildKamailio5Module,rtpengine,RTP engine,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,rtpproxy,RTP proxy,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,sanity,SIP sanity checks,,+kamailio5-mod-sl))
$(eval $(call BuildKamailio5Module,sctp,SCTP support,,+libsctp))
$(eval $(call BuildKamailio5Module,sipcapture,SIP capture,,))
$(eval $(call BuildKamailio5Module,siptrace,SIP trace,,))
$(eval $(call BuildKamailio5Module,siputils,SIP utilities,,+kamailio5-mod-sl))
$(eval $(call BuildKamailio5Module,sl,Stateless replier,,))
$(eval $(call BuildKamailio5Module,sms,SIP-to-SMS IM gateway,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,speeddial,Per-user speed-dial controller,,))
$(eval $(call BuildKamailio5Module,sqlops,SQL operations,,))
$(eval $(call BuildKamailio5Module,statistics,Script statistics,,))
$(eval $(call BuildKamailio5Module,stun,STUN server support,,))
$(eval $(call BuildKamailio5Module,sst,SIP Session Timer,,+kamailio5-mod-dialog +kamailio5-mod-sl))
$(eval $(call BuildKamailio5Module,tcpops,TCP options tweaking operations,,))
$(eval $(call BuildKamailio5Module,textops,Text operations,,))
$(eval $(call BuildKamailio5Module,tls,TLS operations,,+libopenssl))
$(eval $(call BuildKamailio5Module,topoh,Topology hiding,,+kamailio5-mod-rr))
$(eval $(call BuildKamailio5Module,tm,Transaction,,))
$(eval $(call BuildKamailio5Module,tmx,Transaction module extensions,,))
$(eval $(call BuildKamailio5Module,uac,User Agent Client,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,uac_redirect,User Agent Client redirection,,+kamailio5-mod-tm))
$(eval $(call BuildKamailio5Module,uri_db,Database-backend SIP URI checking,,))
$(eval $(call BuildKamailio5Module,userblacklist,User blacklists,,))
$(eval $(call BuildKamailio5Module,usrloc,User location,,))
$(eval $(call BuildKamailio5Module,utils,Misc utilities,,+libcurl +libxml2))
$(eval $(call BuildKamailio5Module,uuid,UUID utilities,,+libuuid))
$(eval $(call BuildKamailio5Module,xcap_client,XCAP Client,,+libcurl))
$(eval $(call BuildKamailio5Module,xlog,Advanced logger,,))
$(eval $(call BuildKamailio5Module,xmlrpc,XML RPC module,,+libxml2))
$(eval $(call BuildKamailio5Module,xmpp,SIP-to-XMPP Gateway,,+kamailio5-mod-tm +libexpat))

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

$(eval $(call BuildPackage,kamailio5))
$(foreach m,$(KAM_MODULES),$(eval $(call BuildPackage,$(m))))

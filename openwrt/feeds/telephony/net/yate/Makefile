#
# Copyright (C) 2006-2014 OpenWrt.org
# Copyright (C) 2006-2011 SMBPhone
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

RELEASEVER:=5.5.0

PKG_NAME:=yate
PKG_VERSION:=$(RELEASEVER)-1
PKG_RELEASE:=2

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://yate.null.ro/tarballs/yate5/
PKG_MD5SUM:=c433b3df08a5993e16fadaac279e39a0
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

PKG_FIXUP:=autoreconf

PKG_INSTALL:=1

include $(INCLUDE_DIR)/uclibc++.mk
include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  URL:=http://yate.null.ro/
endef

define Package/$(PKG_NAME)
  $(call Package/yate/Default)
  DEPENDS:=+libpthread $(CXX_DEPENDS)
  TITLE:=Yet Another Telephony Engine
  MENU:=1
endef

define Package/$(PKG_NAME)/conffiles
/etc/yate/yate.conf
/etc/init.d/yate
endef

define Package/$(PKG_NAME)/Default/description
  Is a next-generation telephony engine focused on the VoIP and PSTN. It does
  SIP, H.323, IAX, PSTN, and more.
endef

define Package/$(PKG_NAME)-scripts-perl
  $(call Package/yate/Default)
  DEPENDS += $(PKG_NAME) $(PKG_NAME)-mod-extmodule +perlbase-data
  TITLE:= Perl module for Yate
endef

define Package/$(PKG_NAME)-sounds
  $(call Package/yate/Default)
  DEPENDS += $(PKG_NAME)
  TITLE := Sounds for Yate
endef

define Package/$(PKG_NAME)-collection-basic
  $(call Package/yate/Default)
  DEPENDS += $(PKG_NAME) \
	+$(PKG_NAME)-sounds \
	+$(PKG_NAME)-mod-dumbchan \
	+$(PKG_NAME)-mod-regfile \
	+$(PKG_NAME)-mod-regexroute \
	+$(PKG_NAME)-mod-rmanager \
	+$(PKG_NAME)-mod-tonegen \
	+$(PKG_NAME)-mod-yrtpchan \
	+$(PKG_NAME)-mod-ysipchan
   TITLE := Basic Yate Server
endef

ifneq ($(CONFIG_USE_MUSL),)
  LIBNSL:=
else
  LIBNSL:=-lnsl
endif

TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include $(if $(CONFIG_BIG_ENDIAN),-DBIGENDIAN,)

CONFIGURE_ARGS+= \
	--enable-strings \
	--enable-poll \
	--enable-inline \
	--enable-atomics \
	--disable-sse2 \
	--disable-sctp \
	--enable-resolver \
	--enable-internalregex \
	--disable-dahdi \
	--disable-zaptel \
	--disable-wpcard \
	--disable-tdmcard \
	--disable-wanpipe \
	--enable-ilbc \
	--enable-ilbc-webrtc \
	--enable-isac-float \
	--disable-isac-fixed \
	--enable-rtti \
	--with-libpq="$(STAGING_DIR)/usr" \
	--with-mysql="$(STAGING_DIR)/usr" \
	--without-wphwec \
	--without-libgsm \
	--without-amrnb \
	--with-spandsp="$(STAGING_DIR)/usr/include" \
	--without-pwlib \
	--without-openh323 \
	--with-openssl \
	--with-zlib="$(STAGING_DIR)/usr" \
	--without-libqt4 \
	--without-qtstatic \
	--without-coredumper \
	--without-doxygen \
	--without-kdoc

CONFIGURE_VARS+= \
	LDFLAGS="$$$$LDFLAGS -nodefaultlibs" \
	LIBS="-lc -luClibc++ -lpthread -lresolv -ldl -lm -lc $(LIBGCC_S) $(LIBNSL)"

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/lib
	
	for yatelib in "" asn sig mgcp jabber script; do \
		$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/libyate$$$${yatelib}.so.$(RELEASEVER) $(1)/usr/lib ;\
		$(LN) ./libyate$$$${yatelib}.so.$(RELEASEVER) $(1)/usr/lib/libyate$$$${yatelib}.so ;\
	done
	
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/yate $(1)/usr/bin/
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/etc/yate/yate.conf $(1)/etc/$(PKG_NAME)/yate.conf
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/yate.init $(1)/etc/init.d/yate
endef

define Package/$(PKG_NAME)-scripts-perl/install
	$(INSTALL_DIR) $(1)/usr/share/yate/scripts
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)/usr/share/yate/scripts/Yate.pm $(1)/usr/share/yate/scripts/
	$(INSTALL_BIN) ./files/banbrutes.pl $(1)/usr/share/yate/scripts/
endef

define Package/$(PKG_NAME)-sounds/install
	$(INSTALL_DIR) $(1)/usr/share/yate/sounds
	$(CP) $(PKG_INSTALL_DIR)/usr/share/yate/sounds/ $(1)/usr/share/yate/
endef

define Package/$(PKG_NAME)-collection-basic/install
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(STAGING_DIR)/usr/include/
	$(INSTALL_DIR) $(STAGING_DIR)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(STAGING_DIR)/usr/lib/
endef

define BuildPlugin
  define Package/$(PKG_NAME)-mod-$(1)
    $(call Package/yate/Default)
    DEPENDS:= $(PKG_NAME) $(4)
    TITLE:=$(3)
  endef

  $(if $(PKG_BUILD_DIR)/conf.d/$(1).conf.sample,
  define Package/$(PKG_NAME)-mod-$(1)/conffiles
    /etc/yate/$(1).conf
  endef
  ,)
  
  define Package/$(PKG_NAME)-mod-$(1)/description
    $(3) module for $(PKG_NAME)
  endef
  
  define Package/$(PKG_NAME)-mod-$(1)/install
	$$(INSTALL_DIR) $$(1)/usr/lib/yate/$(2)
	$$(INSTALL_BIN) $$(PKG_INSTALL_DIR)/usr/lib/yate/$(2)/$(1).yate $$(1)/usr/lib/yate/$(2)/$(1).yate
	if [ -f $$(PKG_INSTALL_DIR)/etc/yate/$(1).conf ]; then \
		$$(INSTALL_DIR) $$(1)/etc/yate ;\
		$$(INSTALL_DATA) $$(PKG_INSTALL_DIR)/etc/yate/$(1).conf $$(1)/etc/yate/ ;\
	fi
	$(if $(5),$(foreach extra_file,$(5),\
		$(INSTALL_DIR) $$(1)$(dir $(extra_file)) ; \
		$(CP) $(PKG_INSTALL_DIR)$(extra_file) $$(1)$(extra_file) ; \
	),)
  endef

  $$(eval $$(call BuildPackage,$(PKG_NAME)-mod-$(1)))
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-collection-basic))
$(eval $(call BuildPackage,$(PKG_NAME)-scripts-perl))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds))
$(eval $(call BuildPlugin,accfile,server,SIP or H.323 client (from file),))
$(eval $(call BuildPlugin,alsachan,client,ALSA Sound Channel,+alsa-lib))
$(eval $(call BuildPlugin,amrnbcodec,,AMR-NB Codec,@BROKEN)) # Missing amr-nb library
$(eval $(call BuildPlugin,analog,server,Analog (e.g. POTS) Channel,))
$(eval $(call BuildPlugin,analogdetect,server,Analog Data Detector,))
$(eval $(call BuildPlugin,analyzer,,Test Call Generator and AQ Analyzer,))
$(eval $(call BuildPlugin,cache,server,CNAM and LNP memory caches,))
$(eval $(call BuildPlugin,callcounters,server,Count Active Call Legs,))
$(eval $(call BuildPlugin,callfork,,Call Forker,))
$(eval $(call BuildPlugin,callgen,,Call Generator,))
$(eval $(call BuildPlugin,ccongestion,server,Accept Status from Installed Engine Monitors,))
$(eval $(call BuildPlugin,cdrbuild,,Call Detail Record Builder,))
$(eval $(call BuildPlugin,cdrcombine,,Call Detail Records per call instead of per call leg,))
$(eval $(call BuildPlugin,cdrfile,,Call Detail Record to File,))
$(eval $(call BuildPlugin,ciscosm,server,SS7 Support,))
$(eval $(call BuildPlugin,clustering,server,Clustering Server Support,))
$(eval $(call BuildPlugin,conference,,Conference Room Mixer,))
$(eval $(call BuildPlugin,cpuload,server,Monitor CPU load and Inform Yate,))
$(eval $(call BuildPlugin,dbpbx,server,PBX IVR and Multi-routing from Database,))
$(eval $(call BuildPlugin,dbwave,server,Wav Media for DB Storage,+$(PKG_NAME)-mod-wavefile))
$(eval $(call BuildPlugin,dumbchan,,Dummy Channel,))
$(eval $(call BuildPlugin,enumroute,,ENUM Routing,))
$(eval $(call BuildPlugin,eventlogs,server,Write events and alarms to log files,))
$(eval $(call BuildPlugin,extmodule,,External Module Handler,,/usr/share/yate/scripts/echo.sh))
$(eval $(call BuildPlugin,faxchan,,Spandsp Fax Channel,+libspandsp))
$(eval $(call BuildPlugin,filetransfer,,File Transfer Driver,))
$(eval $(call BuildPlugin,gvoice,,Google Voice support,))
$(eval $(call BuildPlugin,heartbeat,server,Linux-HA compatible heartbeat,))
$(eval $(call BuildPlugin,ilbccodec,,iLBC Codec,))
$(eval $(call BuildPlugin,ilbcwebrtc,,iLBC Codec from the WebRTC project,))
$(eval $(call BuildPlugin,isaccodec,,internet Speech Audio Codec,))
$(eval $(call BuildPlugin,isupmangler,sig,ISUP parameter mangling in a STP,))
$(eval $(call BuildPlugin,jabberclient,client,Jabber Client,))
$(eval $(call BuildPlugin,jabberserver,jabber,Jabber Server,))
$(eval $(call BuildPlugin,javascript,,Routing using the embedded Javascript language,))
$(eval $(call BuildPlugin,jbfeatures,jabber,Jabber Server Features,))
$(eval $(call BuildPlugin,lateroute,server,Last Chance Routing,))
$(eval $(call BuildPlugin,mgcpca,server,Media Gateway Control Protocol Agent,))
$(eval $(call BuildPlugin,mgcpgw,server,Media Gateway Control Protocol Gateway,))
$(eval $(call BuildPlugin,moh,,On Hold (music) Generator,))
$(eval $(call BuildPlugin,monitoring,server,Monitoring/gathering Information,))
$(eval $(call BuildPlugin,mrcpspeech,server,MRCP v2 Voice/Tone Detector and Synthesizer,))
$(eval $(call BuildPlugin,msgsniff,,Sample Message Sniffer,))
$(eval $(call BuildPlugin,mux,,Data Multiplexor,))
$(eval $(call BuildPlugin,mysqldb,server,MySQL Backend DB,+libmysqlclient-r))
$(eval $(call BuildPlugin,openssl,,Encrypted transport (OpenSSL),+libopenssl))
$(eval $(call BuildPlugin,osschan,client,OSS Sound Channel,))
$(eval $(call BuildPlugin,park,server,Call Parking,))
$(eval $(call BuildPlugin,pbx,,PBX Message Handlers,))
$(eval $(call BuildPlugin,pbxassist,server,Full featured PBX and IVR,))
$(eval $(call BuildPlugin,pgsqldb,server,PostgrestSQL Backend DB,+libpq))
$(eval $(call BuildPlugin,presence,server,Presence,))
$(eval $(call BuildPlugin,queues,server,Call Distribution and Queues from Database,))
$(eval $(call BuildPlugin,queuesnotify,server,Notify when queued call status changes,))
$(eval $(call BuildPlugin,regexroute,,Regular Expression Based Routing,))
$(eval $(call BuildPlugin,regfile,server,Registration based on users in file,))
$(eval $(call BuildPlugin,register,server,Call Detail Record to a database,))
$(eval $(call BuildPlugin,rmanager,,Yate Remote Management,))
$(eval $(call BuildPlugin,sigtransport,server,SIGTRAN (SS7 over IP) connection provider,))
$(eval $(call BuildPlugin,sip_cnam_lnp,sip,Query CNAM and LNP databases using SIP INVITE,))
$(eval $(call BuildPlugin,sipfeatures,server,SIP Features (SUBSCRIBE/NOTIFY),))
$(eval $(call BuildPlugin,subscription,server,Subcription handler and presence notifier,))
$(eval $(call BuildPlugin,tdmcard,server,TDM Cards Signalling and Data Driver,@BROKEN)) # Missing TDM libraries
$(eval $(call BuildPlugin,tonedetect,,Detectors for Various Tones,))
$(eval $(call BuildPlugin,tonegen,,Tones Generator,))
$(eval $(call BuildPlugin,users,server,Users,))
$(eval $(call BuildPlugin,wavefile,,Wav file Record and Playback,))
$(eval $(call BuildPlugin,wpcard,server,Wanpipe PRI cards Signalling and Data Driver,@BROKEN)) # Mising wanpipe and PRI libraries
$(eval $(call BuildPlugin,yiaxchan,,IAX Channel,))
$(eval $(call BuildPlugin,yjinglechan,,Jingle Channel,))
$(eval $(call BuildPlugin,yradius,server,RADIUS Client,))
$(eval $(call BuildPlugin,yrtpchan,,RTP Channel and Other Data Helper,))
$(eval $(call BuildPlugin,ysigchan,server,SS7/ISDN Protocols - Yate Signalling Library,))
$(eval $(call BuildPlugin,ysipchan,,SIP Channel,))
$(eval $(call BuildPlugin,ysnmpagent,server,SNMP Protocol Agent,,/usr/share/yate/data/NULL-TEAM-MIB.txt /usr/share/yate/data/snmp_mib.conf /usr/share/yate/data/YATE-MIB.txt))
$(eval $(call BuildPlugin,ysockschan,,SOCKS Channel,))
$(eval $(call BuildPlugin,ystunchan,,STUN Support,))
$(eval $(call BuildPlugin,zlibcompress,,Zlib Compression,+zlib))


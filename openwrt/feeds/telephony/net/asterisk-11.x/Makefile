#
# Copyright (C) 2016 OpenWrt.org
# Copyright (C) 2016 Cesnet, z.s.p.o.
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=asterisk11
PKG_VERSION:=11.25.1
PKG_RELEASE:=2

PKG_SOURCE:=asterisk-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://downloads.asterisk.org/pub/telephony/asterisk/releases/
PKG_HASH:=dc9c282ca1142b286e5a5c391647d6be73f35524c8def37718de866e8384a57c

PKG_BUILD_DIR:=$(BUILD_DIR)/asterisk-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=libxml2/host

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING LICENSE
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/asterisk11/install/module
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/asterisk/modules/*$(2).so* $(1)/usr/lib/asterisk/modules/
endef

define Package/asterisk11/install/conffile
	$(INSTALL_DIR) $(1)/etc/asterisk
	$(CP) $(PKG_INSTALL_DIR)/etc/asterisk/*$(2)* $(1)/etc/asterisk/
endef

define Package/asterisk11/install/lib
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/$(2).so* $(1)/usr/lib/
endef

define Package/asterisk11/install/sbin
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/$(2) $(1)/usr/sbin/
endef

define Package/asterisk11/install/sounds
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/sounds/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/asterisk/sounds/en/$(2) $(1)/usr/lib/asterisk/sounds/
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef

define BuildAsterisk11Module
  define Package/asterisk11-$(1)
  $$(call Package/asterisk11/Default)
    TITLE:=$(2) support
    DEPENDS:= asterisk11 $(patsubst +%,+PACKAGE_asterisk11-$(1):%,$(4))
  endef

  define Package/asterisk11-$(1)/conffiles
$(5)
  endef

  define Package/asterisk11-$(1)/description
This package provides support for '$(3)' in Asterisk11.
  endef

  define Package/asterisk11-$(1)/install
$(foreach c,$(6),$(call Package/asterisk11/install/conffile,$$(1),$(c));)
$(foreach m,$(7),$(call Package/asterisk11/install/module,$$(1),$(m));)
$(foreach s,$(8),$(call Package/asterisk11/install/sounds,$$(1),$(s));)
$(foreach b,$(9),$(call Package/asterisk11/install/sbin,$$(1),$(b));)
  endef

  $$(eval $$(call BuildPackage,asterisk11-$(1)))
endef

define Package/asterisk11/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  URL:=http://www.asterisk.org/
  MAINTAINER:=Jiri Slachta <jiri@slachta.eu>
endef

define Package/asterisk11/Default/description
 Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
endef

define Package/asterisk11
$(call Package/asterisk11/Default)
  TITLE:=Complete open source PBX, v$(PKG_VERSION)
  MENU:=1
  DEPENDS:=+libopenssl +libncurses +libpopt +libpthread +libsqlite3 +librt +libuuid +zlib @!TARGET_avr32
  CONFLICTS:=asterisk13
endef

define Package/asterisk11/description
$(call Package/asterisk11/Default/description)
endef

define Package/asterisk11/conffiles
/etc/asterisk/asterisk.conf
/etc/asterisk/acl.conf
/etc/asterisk/ccss.conf
/etc/asterisk/cel.conf
/etc/asterisk/modules.conf
/etc/asterisk/extconfig.conf
/etc/asterisk/extensions.conf
/etc/asterisk/features.conf
/etc/asterisk/http.conf
/etc/asterisk/indications.conf
/etc/asterisk/logger.conf
/etc/asterisk/manager.conf
/etc/asterisk/modules.conf
/etc/asterisk/res_config_sqlite3.conf
/etc/asterisk/rtp.conf
/etc/asterisk/sip.conf
/etc/asterisk/sip_notify.conf
/etc/asterisk/udptl.conf
/etc/asterisk/users.conf
/etc/init.d/asterisk
endef

AST_CFG_FILES:= \
	asterisk.conf acl.conf cel.conf ccss.conf extconfig.conf \
	extensions.conf features.conf http.conf indications.conf \
	logger.conf manager.conf modules.conf res_config_sqlite3.conf \
	rtp.conf sip_notify.conf sip.conf udptl.conf users.conf
AST_EMB_MODULES:=\
	app_dial app_echo app_macro app_playback codec_ulaw format_pcm func_callerid \
	func_logic func_strings func_timeout chan_sip chan_local pbx_config \
	res_crypto res_http_websocket res_rtp_asterisk res_rtp_multicast

define Package/asterisk11/install
$(call Package/asterisk11/install/lib,$(1),libasteriskssl)
$(call Package/asterisk11/install/sbin,$(1),asterisk)
$(call Package/asterisk11/install/sbin,$(1),safe_asterisk)
$(call Package/asterisk11/install/sbin,$(1),astgenkey)
$(foreach m,$(AST_CFG_FILES),$(call Package/asterisk11/install/conffile,$(1),$(m));)
$(foreach m,$(AST_EMB_MODULES),$(call Package/asterisk11/install/module,$(1),$(m));)
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/asterisk.init $(1)/etc/init.d/asterisk
endef

define Package/asterisk11-sounds
$(call Package/asterisk11/Default)
  TITLE:=Sounds support
  DEPENDS:=asterisk11
endef

define Package/asterisk11-sounds/description
This package provides sounds for Asterisk11.
endef

define Package/asterisk11-sounds/install
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/sounds/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/asterisk/sounds/en/* $(1)/usr/lib/asterisk/sounds/
	rm -f $(1)/usr/lib/asterisk/sounds/vm-*
	rm -f $(1)/usr/lib/asterisk/sounds/conf-*
endef

ifneq ($(CONFIG_PACKAGE_asterisk11-chan-dahdi),)
  CONFIGURE_ARGS+= \
	--with-dahdi="$(STAGING_DIR)/usr" \
	--with-pri="$(STAGING_DIR)/usr" \
	--with-tonezone="$(STAGING_DIR)/usr"
else
  CONFIGURE_ARGS+= \
	--without-dahdi \
	--without-pri \
	--without-tonezone
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-curl),)
  CONFIGURE_ARGS+= \
	--with-libcurl="$(STAGING_DIR)/usr"
else
  CONFIGURE_ARGS+= \
	--without-libcurl
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-mysql),)
  CONFIGURE_VARS+= \
	ac_cv_path_ac_pt_CONFIG_MYSQLCLIENT=$(STAGING_DIR)/usr/bin/mysql_config
  CONFIGURE_ARGS+= \
	--with-mysqlclient
else
  CONFIGURE_ARGS+= \
	--without-mysqlclient
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-pbx-lua),)
  CONFIGURE_ARGS+= \
	--with-lua="$(STAGING_DIR)/usr"
  TARGET_LDFLAGS+=-ldl -lcrypt
else
  CONFIGURE_ARGS+= \
	--without-lua
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-pgsql),)
  CONFIGURE_ARGS+= \
	--with-postgres="$(STAGING_DIR)/usr"
else
  CONFIGURE_ARGS+= \
	--without-postgres
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-res-fax-spandsp),)
  CONFIGURE_ARGS+= \
	--with-spandsp="$(STAGING_DIR)/usr"
else
  CONFIGURE_ARGS+= \
	--without-spandsp
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-res-srtp),)
  CONFIGURE_ARGS+= \
	--with-srtp="$(STAGING_DIR)/usr"
else
  CONFIGURE_ARGS+= \
	--without-srtp
endif

ifneq ($(CONFIG_PACKAGE_asterisk11-res-xmpp),)
  CONFIGURE_ARGS+= \
	--with-iksemel="$(STAGING_DIR)/usr"
  SITE_VARS+= \
	ac_cv_lib_iksemel_iks_start_sasl=yes
else
  CONFIGURE_ARGS+= \
	--without-iksemel
endif

EXTRA_CFLAGS+= $(TARGET_CPPFLAGS)
EXTRA_LDFLAGS+= $(TARGET_LDFLAGS) -Wl,-rpath-link,$(STAGING_DIR)/usr/lib
CONFIGURE_ARGS+= \
	--build=$(GNU_HOST_NAME) \
	--host=$(GNU_TARGET_NAME) \
	--target=$(GNU_TARGET_NAME) \
	--without-bluetooth \
	--without-cap \
	--without-curses \
	--with-gsm=internal \
	--without-gtk2 \
	--with-ilbc=internal \
	--without-isdnnet \
	--without-libxml2 \
	--without-misdn \
	--without-nbs \
	--with-ncurses="$(STAGING_DIR)/usr" \
	--without-netsnmp \
	--without-neon \
	--without-neon29 \
	--without-newt \
	--without-ogg \
	--without-osptk \
	--with-popt="$(STAGING_DIR)/usr" \
	--without-pwlib \
	--without-radius \
	--without-sdl \
	--without-sqlite \
	--with-sqlite3="$(STAGING_DIR)/usr" \
	--without-suppserv \
	--without-tds \
	--without-termcap \
	--without-tinfo \
	--with-uuid="$(STAGING_DIR)/usr" \
	--without-vorbis \
	--without-vpb \
	--with-z="$(STAGING_DIR)/usr" \
	--with-sounds-cache="$(DL_DIR)" \
	--disable-xmldoc

CONFIGURE_VARS += \
	ac_cv_path_ac_pt_CONFIG_LIBXML2=$(STAGING_DIR)/host/bin/xml2-config

AST_MENUSELECT_OPTS = \
	--without-newt \
	--without-curses \
	--with-libxml2="$(STAGING_DIR_HOSTPKG)/usr"

define Build/Prepare
	$(call Build/Prepare/Default)
ifneq ($(CONFIG_TARGET_lantiq),)
	$(CP) ./src-lantiq/* $(PKG_BUILD_DIR)/
endif
endef

define Build/Configure
	(cd $(PKG_BUILD_DIR); \
		./bootstrap.sh; \
	);
	echo "export CFLAGS += $(FPIC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS)"\
		" $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS)"\
		> $(PKG_BUILD_DIR)/res/pjproject/user.mak;
	echo "export LDLAGS += $(TARGET_LDFLAGS) $(EXTRA_LDFLAGS)"\
		" -lc $(LIBGCC_S) -lm"\
		>> $(PKG_BUILD_DIR)/res/pjproject/user.mak;
	echo "export CXXFLAGS += $(FPIC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS)"\
		" $(TARGET_CPPFLAGS) $(EXTRA_CPPFLAGS)"\
		>> $(PKG_BUILD_DIR)/res/pjproject/user.mak;
	$(call Build/Configure/Default,,$(SITE_VARS))
	(cd $(PKG_BUILD_DIR)/menuselect; \
		CC="$(HOSTCC)" \
		CFLAGS="$(HOST_CFLAGS) -I$(STAGING_DIR_HOSTPKG)/include/libxml2" \
		CONFIG_SITE= \
		LDFLAGS="$(HOST_LDFLAGS) -Wl,-rpath,$(STAGING_DIR_HOSTPKG)/lib" \
		ac_cv_path_ac_pt_CONFIG_LIBXML2=$(STAGING_DIR_HOSTPKG)/bin/xml2-config \
		./configure \
		$(HOST_CONFIGURE_ARGS) \
		$(AST_MENUSELECT_OPTS) \
	);
endef

define Build/Compile
	CC="$(HOSTCC)" \
	CFLAGS="$(HOST_CFLAGS) -I$(STAGING_DIR_HOSTPKG)/include/libxml2" \
	LDFLAGS="$(HOST_LDFLAGS) -Wl,-rpath,$(STAGING_DIR_HOSTPKG)/lib" \
	$(MAKE) -C "$(PKG_BUILD_DIR)/menuselect"
	$(MAKE) -C "$(PKG_BUILD_DIR)" menuselect-tree
	cd "$(PKG_BUILD_DIR)" && \
		./menuselect/menuselect \
			--disable BUILD_NATIVE \
			$(if $(CONFIG_ASTERISK11_LOW_MEMORY),--enable LOW_MEMORY) \
			menuselect.makeopts
	$(MAKE) -C "$(PKG_BUILD_DIR)" include/asterisk/version.h \
		include/asterisk/buildopts.h defaults.h \
		makeopts.embed_rules
	ASTCFLAGS="$(EXTRA_CFLAGS)" \
	ASTLDFLAGS="$(EXTRA_LDFLAGS)" \
	$(MAKE) -C "$(PKG_BUILD_DIR)" \
		ASTVARLIBDIR="/usr/lib/asterisk" \
		ASTDATADIR="/usr/lib/asterisk" \
		ASTKEYDIR="/usr/lib/asterisk" \
		ASTDBDIR="/usr/lib/asterisk" \
		NOISY_BUILD="yes" \
		DEBUG="" \
		OPTIMIZE="" \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all install samples
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/asterisk-11/include/asterisk/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/asterisk/*.h $(1)/usr/include/asterisk-11/include/asterisk/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/asterisk.h $(1)/usr/include/asterisk-11/include/
endef

$(eval $(call BuildPackage,asterisk11))
$(eval $(call BuildPackage,asterisk11-sounds))

################################
# AST modules
# Params:
# 1 - Package subname
# 2 - Package title
# 3 - Module description
# 4 - Module dependencies
# 5 - Full path to multiple conffiles
# 6 - conf files
# 7 - module files
# 8 - sound files
# 9 - binary files
################################

$(eval $(call BuildAsterisk11Module,app-alarmreceiver,Alarm receiver,Central Station Alarm receiver for Ademco Contact ID,,,,app_alarmreceiver,))
$(eval $(call BuildAsterisk11Module,app-authenticate,Authenticate commands,Execute arbitrary authenticate commands,,,,app_authenticate,))
$(eval $(call BuildAsterisk11Module,app-confbridge,ConfBridge,Software bridge for multi-party audio conferencing,,/etc/asterisk/confbridge.conf,confbridge.conf,app_confbridge bridge_builtin_features bridge_multiplexed bridge_simple bridge_softmix chan_bridge,))
$(eval $(call BuildAsterisk11Module,app-dahdiras,Execute an ISDN RAS,support for executing an ISDN RAS using DAHDI,+asterisk11-chan-dahdi,,,app_dahdiras,))
$(eval $(call BuildAsterisk11Module,app-directory,Provide a directory of extensions,provides a directory of extensions,,,,app_directory,))
$(eval $(call BuildAsterisk11Module,app-directed_pickup,Directed call pickup,support for directed call pickup,,,,app_directed_pickup,))
$(eval $(call BuildAsterisk11Module,app-disa,Direct Inward System Access,Direct Inward System Access,,,,app_disa,))
$(eval $(call BuildAsterisk11Module,app-exec,Exec application,support for application execution,,,,app_exec,))
$(eval $(call BuildAsterisk11Module,app-chanisavail,Channel availability check,support for checking if a channel is available,,,,app_chanisavail,))
$(eval $(call BuildAsterisk11Module,app-chanspy,Channel listen in,support for listening in on any channel,,,,app_chanspy,))
$(eval $(call BuildAsterisk11Module,app-minivm,Minimal voicemail system,a voicemail system in small building blocks working together based on the Comedian Mail voicemail,,/etc/asterisk/extensions_minivm.conf /etc/asterisk/minivm.conf,extensions_minivm.conf minivm.conf,app_minivm,))
$(eval $(call BuildAsterisk11Module,app-mixmonitor,Record a call and mix the audio,record a call and mix the audio during the recording,,,,app_mixmonitor,))
$(eval $(call BuildAsterisk11Module,app-originate,Originate a call,originating an outbound call and connecting it to a specified extension or application,,,,app_originate,))
$(eval $(call BuildAsterisk11Module,app-playtones,Playtones application,play a tone list,,,,app_playtones,))
$(eval $(call BuildAsterisk11Module,app-queue,True Call Queueing,support for ACD,,/etc/asterisk/queues.conf /etc/asterisk/queuerules.conf,queues.conf queuerules.conf,app_queue,))
$(eval $(call BuildAsterisk11Module,app-read,Variable read,a trivial application to read a variable,,,,app_read,))
$(eval $(call BuildAsterisk11Module,app-readexten,Extension to variable,a trivial application to read an extension into a variable,,,,app_readexten,))
$(eval $(call BuildAsterisk11Module,app-record,Record sound file,to record a sound file,,,,app_record,))
$(eval $(call BuildAsterisk11Module,app-sayunixtime,Say Unix time,an application to say Unix time,,,,app_sayunixtime,))
$(eval $(call BuildAsterisk11Module,app-senddtmf,Send DTMF digits,Sends arbitrary DTMF digits,,,,app_senddtmf,))
$(eval $(call BuildAsterisk11Module,app-sms,SMS,SMS support (ETSI ES 201 912 protocol 1),+libpopt +libstdcpp,,,app_sms,,smsq))
$(eval $(call BuildAsterisk11Module,app-speech,Dialplan Speech,Dialplan Speech Applications,+asterisk11-res-speech,,,app_speech_utils,))
$(eval $(call BuildAsterisk11Module,app-stack,Stack applications,Stack applications Gosub Return etc.,+asterisk11-res-agi,,,app_stack,))
$(eval $(call BuildAsterisk11Module,app-system,System exec,support for executing system commands,,,,app_system,))
$(eval $(call BuildAsterisk11Module,app-talkdetect,File playback with audio detect,for file playback with audio detect,,,,app_talkdetect,))
$(eval $(call BuildAsterisk11Module,app-verbose,Verbose logging,Verbose logging application,,,,app_verbose,))
$(eval $(call BuildAsterisk11Module,app-waituntil,Sleep,support sleeping until the given epoch,,,,app_waituntil,))
$(eval $(call BuildAsterisk11Module,app-while,While loop,a while loop implementation,,,,app_while,))
$(eval $(call BuildAsterisk11Module,cdr-csv,Provides CDR CSV,Call Detail Record with CSV support,,,,cdr_csv,))
$(eval $(call BuildAsterisk11Module,cdr-sqlite3,Provides CDR SQLITE3,Call Detail Record with SQLITE3 support,libsqlite3,,,cdr_sqlite3_custom,))
$(eval $(call BuildAsterisk11Module,cdr,Provides CDR,Call Detail Record,,/etc/asterisk/cdr.conf /etc/asterisk/cdr_custom.conf /etc/asterisk/cdr_manager.conf /etc/asterisk/cdr_syslog.conf,cdr cdr_custom cdr_manager cdr_syslog,app_cdr app_forkcdr cdr_custom cdr_manager cdr_syslog func_cdr,))
$(eval $(call BuildAsterisk11Module,chan-alsa,ALSA channel,the channel chan_alsa,+alsa-lib,/etc/asterisk/alsa.conf,alsa.conf,chan_alsa,,))
$(eval $(call BuildAsterisk11Module,chan-agent,Agents proxy channel,an implementation of agents proxy channel,,,,chan_agent,))
$(eval $(call BuildAsterisk11Module,chan-dahdi,DAHDI channel,DAHDI channel support,+dahdi-tools-libtonezone +kmod-dahdi +libpri @!aarch64,/etc/asterisk/chan_dahdi.conf,chan_dahdi.conf,chan_dahdi,))
$(eval $(call BuildAsterisk11Module,chan-iax2,IAX2 channel,IAX support,+asterisk11-res-timing-timerfd,/etc/asterisk/iax.conf /etc/asterisk/iaxprov.conf,iax.conf iaxprov.conf,chan_iax2,))
$(eval $(call BuildAsterisk11Module,chan-lantiq,Lantiq TAPI channel,An implementation of Lantiq TAPI channel,@TARGET_lantiq +kmod-ltq-vmmc,/etc/asterisk/lantiq.conf,lantiq.conf,chan_lantiq,))
$(eval $(call BuildAsterisk11Module,chan-mgcp,MGCP channel,the channel chan_mgcp,,/etc/asterisk/mgcp.conf,mgcp.conf,chan_mgcp,))
$(eval $(call BuildAsterisk11Module,chan-motif,Jingle channel,Motif Jingle Channel Driver,+asterisk11-res-xmpp,/etc/asterisk/motif.conf /etc/asterisk/jingle.conf,motif.conf jingle.conf,chan_motif,))
$(eval $(call BuildAsterisk11Module,chan-ooh323,ooH323 channel,the channel chan_ooh323,,/etc/asterisk/ooh323.conf,ooh323.conf,chan_ooh323,))
$(eval $(call BuildAsterisk11Module,chan-oss,OSS channel,the channel chan_oss,,/etc/asterisk/oss.conf,oss.conf,chan_oss,,))
$(eval $(call BuildAsterisk11Module,chan-skinny,Skinny channel,the channel chan_skinny,,/etc/asterisk/skinny.conf,skinny.conf,chan_skinny,))
$(eval $(call BuildAsterisk11Module,chan-unistim,Unistim channel,channel driver for the UNISTIM (Unified Networks IP Stimulus) protocol,,/etc/asterisk/unistim.conf,unistim.conf,chan_unistim,))
$(eval $(call BuildAsterisk11Module,codec-a-mu,Alaw to ulaw translation,translation between alaw and ulaw codecs,,,,codec_a_mu,))
$(eval $(call BuildAsterisk11Module,codec-adpcm,ADPCM text,ADPCM text ,,,,codec_adpcm,))
$(eval $(call BuildAsterisk11Module,codec-alaw,Signed linear to alaw translation,translation between signed linear and alaw codecs,,,,codec_alaw,))
$(eval $(call BuildAsterisk11Module,codec-dahdi,DAHDI codec,DAHDI native transcoding support,+asterisk11-chan-dahdi,,,codec_dahdi,))
$(eval $(call BuildAsterisk11Module,codec-g722,G.722,a high bit rate 48/56/64Kbps ITU standard codec,,,,codec_g722,))
$(eval $(call BuildAsterisk11Module,codec-g726,Signed linear to G.726 translation,translation between signed linear and ITU G.726-32kbps codecs,,,,codec_g726,))
$(eval $(call BuildAsterisk11Module,codec-gsm,linear to GSM translation,translate between signed linear and GSM,,,,codec_gsm,))
$(eval $(call BuildAsterisk11Module,codec-ilbc,linear to ILBC translation,translate between signed linear and ILBC,,,,codec_ilbc,))
$(eval $(call BuildAsterisk11Module,codec-lpc10,Linear to LPC10 translation,translate between signed linear and LPC10,,,,codec_lpc10,))
$(eval $(call BuildAsterisk11Module,codec-resample,resample sLinear audio,resample sLinear audio,,,,codec_resample,))
$(eval $(call BuildAsterisk11Module,curl,CURL,CURL support,+libcurl,,,func_curl res_curl,))
$(eval $(call BuildAsterisk11Module,format-g726,G.726,support for headerless G.726 16/24/32/40kbps data format,,,,format_g726,))
$(eval $(call BuildAsterisk11Module,format-g729,G.729,support for raw headerless G729 data,,,,format_g729,))
$(eval $(call BuildAsterisk11Module,format-gsm,GSM format,support for GSM format,,,,format_gsm,))
$(eval $(call BuildAsterisk11Module,format-h263,H263 format,support for H264 format,,,,format_h263,))
$(eval $(call BuildAsterisk11Module,format-h264,H264 format,support for H264 format,,,,format_h264,))
$(eval $(call BuildAsterisk11Module,format-ilbc,ILBC format,support for ILBC format,,,,format_ilbc,))
$(eval $(call BuildAsterisk11Module,format-sln,Raw slinear format,support for raw slinear format,,,,format_sln,))
$(eval $(call BuildAsterisk11Module,format-vox,VOX format,support for ADPCM vox format,,,,format_vox,))
$(eval $(call BuildAsterisk11Module,format-wav-gsm,WAV format (Proprietary GSM),support for proprietary Microsoft WAV format (Proprietary GSM),,,,format_wav_gsm,))
$(eval $(call BuildAsterisk11Module,format-wav,WAV format (8000hz Signed Linear),support for proprietary Microsoft WAV format (8000hz Signed Linear),,,,format_wav,))
$(eval $(call BuildAsterisk11Module,func-base64,base64 support,support of base64 function,,,,func_base64,))
$(eval $(call BuildAsterisk11Module,func-blacklist,Blacklist on callerid,looking up the callerid number and see if it is blacklisted,,,,func_blacklist,))
$(eval $(call BuildAsterisk11Module,func-cut,CUT function,CUT function,,,,func_cut,))
$(eval $(call BuildAsterisk11Module,func-db,Database interaction,functions for interaction with the database,,,,func_db app_db,))
$(eval $(call BuildAsterisk11Module,func-devstate,Blinky lights control,functions for manually controlled blinky lights,,,,func_devstate,))
$(eval $(call BuildAsterisk11Module,func-enum,ENUM,ENUM,,/etc/asterisk/enum.conf,enum.conf,func_enum,))
$(eval $(call BuildAsterisk11Module,func-env,Environment functions,Environment dialplan functions,,,,func_env,))
$(eval $(call BuildAsterisk11Module,func-extstate,Hinted extension state,retrieving the state of a hinted extension for dialplan control,,,,func_extstate,))
$(eval $(call BuildAsterisk11Module,func-global,Global variable,global variable dialplan functions,,,,func_global,))
$(eval $(call BuildAsterisk11Module,func-groupcount,Group count,for counting number of channels in the specified group,,,,func_groupcount,))
$(eval $(call BuildAsterisk11Module,func-channel,Channel info,Channel info dialplan function,,,,func_channel,))
$(eval $(call BuildAsterisk11Module,func-math,Math functions,Math functions,,,,func_math,))
$(eval $(call BuildAsterisk11Module,func-module,Simple module check function,Simple module check function,,,,func_module,))
$(eval $(call BuildAsterisk11Module,func-presencestate,Hinted presence state,Gets or sets a presence state in the dialplan,,,,func_presencestate,))
$(eval $(call BuildAsterisk11Module,func-shell,Shell,support for shell execution,,,,func_shell,))
$(eval $(call BuildAsterisk11Module,func-uri,URI encoding and decoding,Encodes and decodes URI-safe strings,,,,func_uri,))
$(eval $(call BuildAsterisk11Module,func-vmcount,vmcount dialplan,a vmcount dialplan function,,,,func_vmcount,))
$(eval $(call BuildAsterisk11Module,func-realtime,realtime dialplan,the realtime dialplan function,,,,func_realtime,))
$(eval $(call BuildAsterisk11Module,mysql,MySQL,MySQL support,+libmysqlclient,/etc/asterisk/res_config_mysql.conf,res_config_mysql.conf,res_config_mysql,))
$(eval $(call BuildAsterisk11Module,odbc,ODBC,ODBC support,+libpthread +libc +unixodbc,/etc/asterisk/cdr_adaptive_odbc.conf /etc/asterisk/cdr_odbc.conf /etc/asterisk/cel_odbc.conf /etc/asterisk/func_odbc.conf /etc/asterisk/res_odbc.conf,cdr_adaptive_odbc cdr_odbc cel_odbc func_odbc res_odbc,cdr_adaptive_odbc cdr_odbc cel_odbc func_odbc res_config_odbc res_odbc,))
$(eval $(call BuildAsterisk11Module,pbx-ael,Asterisk Extension Logic,support for symbolic Asterisk Extension Logic,,,extensions.ael,pbx_ael,))
$(eval $(call BuildAsterisk11Module,pbx-dundi,Dundi,provides Dundi Lookup service for Asterisk,,/etc/asterisk/dundi.conf,dundi.conf,pbx_dundi,))
$(eval $(call BuildAsterisk11Module,pbx-lua,Lua,provides Lua resources for Asterisk,+libpthread +libc +liblua,/etc/asterisk/extensions.lua,extensions.lua,pbx_lua,))
$(eval $(call BuildAsterisk11Module,pbx-spool,Call Spool,outgoing call spool support,,,,pbx_spool,))
$(eval $(call BuildAsterisk11Module,pbx-realtime,Realtime Switch,realtime switch support,,,,pbx_realtime,))
$(eval $(call BuildAsterisk11Module,pgsql,PostgreSQL,PostgreSQL support,+libpq @!arc,/etc/asterisk/cel_pgsql.conf /etc/asterisk/cdr_pgsql.conf /etc/asterisk/res_pgsql.conf,cel_pgsql.conf cdr_pgsql.conf res_pgsql.conf,cel_pgsql cdr_pgsql res_config_pgsql,))
$(eval $(call BuildAsterisk11Module,res-adsi,Provide ADSI,Analog Display Services Interface capability,,,,res_adsi,))
$(eval $(call BuildAsterisk11Module,res-ael-share,Shareable AEL code,support for shareable AEL code mainly between internal and external modules,,,,res_ael_share,))
$(eval $(call BuildAsterisk11Module,res-agi,Asterisk Gateway Interface,Support for the Asterisk Gateway Interface extension,+asterisk11-res-speech,,,res_agi,))
$(eval $(call BuildAsterisk11Module,res-calendar,Calendaring API,Calendaring support (ICal and Google Calendar),,/etc/asterisk/calendar.conf,calendar.conf,res_calendar,,))
$(eval $(call BuildAsterisk11Module,res-clioriginate,Calls via CLI,Originate calls via the CLI,,,,res_clioriginate,))
$(eval $(call BuildAsterisk11Module,res-fax-spandsp,Spandsp T.38 and G.711,Spandsp T.38 and G.711 FAX Resource,+asterisk11-res-fax +libspandsp +libtiff,,,res_fax_spandsp,))
$(eval $(call BuildAsterisk11Module,res-fax,FAX modules,Generic FAX resource for FAX technology resource modules,+asterisk11-res-timing-pthread,/etc/asterisk/res_fax.conf,res_fax.conf,res_fax,))
$(eval $(call BuildAsterisk11Module,res-monitor,Provide Monitor,Cryptographic Signature capability,,,,res_monitor,))
$(eval $(call BuildAsterisk11Module,res-musiconhold,MOH,Music On Hold support,,/etc/asterisk/musiconhold.conf,musiconhold.conf,res_musiconhold,))
$(eval $(call BuildAsterisk11Module,res-phoneprov,Phone Provisioning,Phone provisioning application for the asterisk internal http server,,/etc/asterisk/phoneprov.conf,phoneprov.conf,res_phoneprov,))
$(eval $(call BuildAsterisk11Module,res-pktccops,Provide PacketCable COPS,simple client/server model for supporting policy control over QoS signaling protocols,,/etc/asterisk/res_pktccops.conf,res_pktccops.conf,res_pktccops,))
$(eval $(call BuildAsterisk11Module,res-realtime,Realtime CLI,Realtime CLI,,,,res_realtime,))
$(eval $(call BuildAsterisk11Module,res-smdi,Provide SMDI,Simple Message Desk Interface capability,,/etc/asterisk/smdi.conf,smdi.conf,res_smdi,))
$(eval $(call BuildAsterisk11Module,res-speech,Speech Recognition API,Support for the Asterisk Generic Speech Recognition API,,,,res_speech,))
$(eval $(call BuildAsterisk11Module,res-srtp,SRTP,Secure RTP,+libsrtp,,,res_srtp,))
$(eval $(call BuildAsterisk11Module,res-stun-monitor,STUN monitoring,resource STUN Monitor,,/etc/asterisk/res_stun_monitor.conf,res_stun_monitor.conf,res_stun_monitor,))
$(eval $(call BuildAsterisk11Module,res-timing-dahdi,DAHDI Timing Interface,DAHDI timing interface,+asterisk11-chan-dahdi,,,res_timing_dahdi,))
$(eval $(call BuildAsterisk11Module,res-timing-pthread,pthread Timing Interface,POSIX pthreads Timing Interface,,,,res_timing_pthread,))
$(eval $(call BuildAsterisk11Module,res-timing-timerfd,Timerfd Timing Interface,Timing interface provided by Linux kernel,,,,res_timing_timerfd,))
$(eval $(call BuildAsterisk11Module,res-xmpp,XMPP client and component module,reference module for interfacting Asterisk directly as a client or component with XMPP server,+libiksemel +libopenssl,/etc/asterisk/xmpp.conf,xmpp.conf,res_xmpp,))
$(eval $(call BuildAsterisk11Module,voicemail,Voicemail,voicemail related modules,+asterisk11-res-adsi +asterisk11-res-smdi,/etc/asterisk/voicemail.conf,voicemail.conf,*voicemail,vm-*)) 

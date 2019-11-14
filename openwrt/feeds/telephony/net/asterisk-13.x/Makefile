#
# Copyright (C) 2016 OpenWrt.org
# Copyright (C) 2016 Cesnet, z.s.p.o.
# Copyright (C) 2017 Jiri Slachta <jiri@slachta.eu>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=asterisk13
PKG_VERSION:=13.17.0
PKG_RELEASE:=5

PKG_SOURCE:=asterisk-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://downloads.asterisk.org/pub/telephony/asterisk/releases/
PKG_HASH:=c4a89386d7db58520391fcbd3202902584b4434289094ca2e4a6b8dada1e90a4

PKG_BUILD_DIR:=$(BUILD_DIR)/asterisk-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=libxml2/host

PKG_LICENSE:=GPL-2.0
PKG_LICENSE_FILES:=COPYING LICENSE
PKG_MAINTAINER:=Jiri Slachta <jiri@slachta.eu>

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/host-build.mk

define Package/asterisk13/install/module
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/modules
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/asterisk/modules/*$(2).so* $(1)/usr/lib/asterisk/modules/
endef

define Package/asterisk13/install/conffile
	$(INSTALL_DIR) $(1)/etc/asterisk
	$(CP) $(PKG_INSTALL_DIR)/etc/asterisk/$(2) $(1)/etc/asterisk/
endef

define Package/asterisk13/install/lib
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/$(2).so* $(1)/usr/lib/
endef

define Package/asterisk13/install/sbin
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/sbin/$(2) $(1)/usr/sbin/
endef

define Package/asterisk13/install/sounds
	$(INSTALL_DIR) $(1)/usr/share/asterisk/sounds/
	$(CP) $(PKG_INSTALL_DIR)/usr/share/asterisk/sounds/en/$(2) $(1)/usr/share/asterisk/sounds/
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef

define BuildAsterisk13Module
  define Package/asterisk13-$(1)
  $$(call Package/asterisk13/Default)
    TITLE:=$(2) support
    DEPENDS:= asterisk13 $(patsubst +%,+PACKAGE_asterisk13-$(1):%,$(4))
  endef

  define Package/asterisk13-$(1)/conffiles
$(subst $(space),$(newline),$(foreach c,$(5),/etc/asterisk/$(c)))
  endef

  define Package/asterisk13-$(1)/description
This package provides support for '$(3)' in Asterisk.
  endef

  define Package/asterisk13-$(1)/install
$(foreach c,$(5),$(call Package/asterisk13/install/conffile,$$(1),$(c));)
$(foreach m,$(6),$(call Package/asterisk13/install/module,$$(1),$(m));)
$(foreach s,$(7),$(call Package/asterisk13/install/sounds,$$(1),$(s));)
$(foreach b,$(8),$(call Package/asterisk13/install/sbin,$$(1),$(b));)
  endef

  $$(eval $$(call BuildPackage,asterisk13-$(1)))
endef

define Package/asterisk13/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  URL:=http://www.asterisk.org/
endef

define Package/asterisk13/Default/description
 Asterisk is a complete PBX in software. It provides all of the features
 you would expect from a PBX and more. Asterisk does voice over IP in three
 protocols, and can interoperate with almost all standards-based telephony
 equipment using relatively inexpensive hardware.
endef

define Package/asterisk13
$(call Package/asterisk13/Default)
  TITLE:=Complete open source PBX, v$(PKG_VERSION)
  MENU:=1
  DEPENDS:=+jansson +libncurses +libopenssl +libpopt +libsqlite3 +libstdcpp +libuuid +libxml2 +libxslt +zlib
endef

define Package/asterisk13/description
$(call Package/asterisk13/Default/description)
endef

define Package/asterisk13/conffiles
/etc/asterisk/asterisk.conf
/etc/asterisk/acl.conf
/etc/asterisk/cel.conf
/etc/asterisk/ccss.conf
/etc/asterisk/extconfig.conf
/etc/asterisk/extensions.conf
/etc/asterisk/features.conf
/etc/asterisk/http.conf
/etc/asterisk/indications.conf
/etc/asterisk/logger.conf
/etc/asterisk/manager.conf
/etc/asterisk/modules.conf
/etc/asterisk/res_config_sqlite3.conf
/etc/asterisk/stasis.conf
/etc/asterisk/udptl.conf
/etc/asterisk/users.conf
/etc/default/asterisk
/etc/init.d/asterisk
endef

AST_CFG_FILES:= \
	asterisk.conf acl.conf cel.conf ccss.conf extconfig.conf \
	extensions.conf features.conf http.conf indications.conf \
	logger.conf manager.conf modules.conf stasis.conf udptl.conf \
	users.conf res_config_sqlite3.conf

AST_EMB_MODULES:=\
	app_dial app_echo app_macro app_playback \
	func_callerid func_logic func_strings func_timeout \
	pbx_config res_crypto

define Package/asterisk13/install
$(call Package/asterisk13/install/lib,$(1),libasteriskssl)
$(call Package/asterisk13/install/sbin,$(1),asterisk)
$(call Package/asterisk13/install/sbin,$(1),safe_asterisk)
$(call Package/asterisk13/install/sbin,$(1),astgenkey)
$(foreach m,$(AST_CFG_FILES),$(call Package/asterisk13/install/conffile,$(1),$(m));)
$(foreach m,$(AST_EMB_MODULES),$(call Package/asterisk13/install/module,$(1),$(m));)
	$(INSTALL_DIR) $(1)/usr/share/asterisk/sounds/
	$(INSTALL_DIR) $(1)/etc/default
	$(INSTALL_DATA) ./files/asterisk.default $(1)/etc/default/asterisk
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/asterisk.init $(1)/etc/init.d/asterisk
endef

define Package/asterisk13-sounds
$(call Package/asterisk13/Default)
  TITLE:=Sounds support
  DEPENDS:=asterisk13
endef

define Package/asterisk13-sounds/description
This package provides the sound-files for Asterisk-13.
endef

define Package/asterisk13-sounds/install
	$(INSTALL_DIR) $(1)/usr/share/asterisk/sounds/
	$(CP) $(PKG_INSTALL_DIR)/usr/share/asterisk/sounds/en/* $(1)/usr/share/asterisk/sounds/
	rm -f $(1)/usr/share/asterisk/sounds/vm-*
endef

ifneq ($(CONFIG_PACKAGE_asterisk13-chan-dahdi),)
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

# Pass CPPFLAGS in the CFLAGS as otherwise the build system will
# ignore them.
TARGET_CFLAGS+=$(TARGET_CPPFLAGS)

CONFIGURE_ARGS+= \
	--without-execinfo \
	--without-bluetooth \
	--without-cap \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-curl),--with-libcurl="$(STAGING_DIR)/usr") \
	--without-curses \
	--with-gsm=internal \
	--without-gtk2 \
	--with-ilbc=internal \
	--without-isdnnet \
	--without-misdn \
	--without-nbs \
	--without-neon \
	--without-neon29 \
	--with-libxml2 \
	--with-ncurses="$(STAGING_DIR)/usr" \
	--without-netsnmp \
	--without-newt \
	--without-ogg \
	--without-osptk \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-pbx-lua),--with-lua="$(STAGING_DIR)/usr",--without-lua) \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-pgsql),--with-postgres="$(STAGING_DIR)/usr",--without-postgres) \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-pjsip),--with-pjproject="$(STAGING_DIR)/usr",--without-pjproject) \
	--with-popt="$(STAGING_DIR)/usr" \
	--without-pwlib \
	--without-radius \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-res-fax-spandsp),--with-spandsp="$(STAGING_DIR)/usr",--without-spandsp) \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-res-xmpp),--with-iksemel="$(STAGING_DIR)/usr",--without-iksemel) \
	--without-sdl \
	--without-sqlite \
	--with-sqlite3="$(STAGING_DIR)/usr" \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-res-srtp),--with-srtp="$(STAGING_DIR)/usr",--without-srtp) \
	--without-suppserv \
	--without-tds \
	--without-termcap \
	--without-tinfo \
	--without-vorbis \
	--without-vpb \
	--with-z="$(STAGING_DIR)/usr" \
	--with-sounds-cache="$(DL_DIR)" \
	--enable-xmldoc

CONFIGURE_VARS += \
	ac_cv_lib_srtp2_srtp_init=no \
	ac_cv_path_ac_pt_CONFIG_LIBXML2=$(STAGING_DIR)/host/bin/xml2-config

MAKE_FLAGS+= \
	ASTDATADIR="/usr/share/asterisk" \
	DESTDIR="$(PKG_INSTALL_DIR)"

# show full gcc arguments instead of [CC] and [LD]
MAKE_FLAGS+= \
	NOISY_BUILD="yes"

# don't let asterisk mess with build flags
MAKE_FLAGS+= \
	AST_FORTIFY_SOURCE="" \
	DEBUG="" \
	OPTIMIZE=""

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
	cd $(PKG_BUILD_DIR); \
		./bootstrap.sh
	$(call Build/Configure/Default)
	cd $(PKG_BUILD_DIR)/menuselect; \
		./bootstrap.sh; \
		CC="$(HOSTCC)" \
		CFLAGS="$(HOST_CFLAGS) -I$(STAGING_DIR_HOSTPKG)/include/libxml2" \
		CONFIG_SITE= \
		LDFLAGS="$(HOST_LDFLAGS) -Wl,-rpath,$(STAGING_DIR_HOSTPKG)/lib" \
		ac_cv_path_ac_pt_CONFIG_LIBXML2=$(STAGING_DIR_HOSTPKG)/bin/xml2-config \
		./configure \
		$(HOST_CONFIGURE_ARGS) \
		$(AST_MENUSELECT_OPTS)
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
			$(if $(CONFIG_ASTERISK13_LOW_MEMORY),--enable LOW_MEMORY) \
			menuselect.makeopts
	$(call Build/Compile/Default,all install samples)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/asterisk-13/include/asterisk/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/asterisk/*.h $(1)/usr/include/asterisk-13/include/asterisk/
	$(CP) $(PKG_INSTALL_DIR)/usr/include/asterisk.h $(1)/usr/include/asterisk-13/include/
endef

$(eval $(call BuildPackage,asterisk13))
$(eval $(call BuildPackage,asterisk13-sounds))

################################
# AST modules
# Params:
# 1 - Package subname
# 2 - Package title
# 3 - Module description
# 4 - Module dependencies
# 5 - conf files
# 6 - module files
# 7 - sound files
# 8 - binary files
################################
#$(eval $(call BuildAsterisk13Module,subname,title,module description,module dependencies,conf files,module files,sound files,binary files))

$(eval $(call BuildAsterisk13Module,app-alarmreceiver,Alarm receiver,Central Station Alarm receiver for Ademco Contact ID,,,app_alarmreceiver,,))
$(eval $(call BuildAsterisk13Module,app-authenticate,Authenticate commands,Execute arbitrary authenticate commands,,,app_authenticate,,))
$(eval $(call BuildAsterisk13Module,app-chanisavail,Channel availability check,support for checking if a channel is available,,,app_chanisavail,,))
$(eval $(call BuildAsterisk13Module,app-chanspy,Channel listen in,support for listening in on any channel,,,app_chanspy,,))
$(eval $(call BuildAsterisk13Module,app-confbridge,ConfBridge,Software bridge for multi-party audio conferencing,+asterisk13-bridge-builtin-features +asterisk13-bridge-simple +asterisk13-bridge-softmix,confbridge.conf,app_confbridge,,))
$(eval $(call BuildAsterisk13Module,app-dahdiras,Execute an ISDN RAS,support for executing an ISDN RAS using DAHDI,+asterisk13-chan-dahdi,,app_dahdiras,,))
$(eval $(call BuildAsterisk13Module,app-directed_pickup,Directed call pickup,support for directed call pickup,,,app_directed_pickup,,))
$(eval $(call BuildAsterisk13Module,app-disa,Direct Inward System Access,Direct Inward System Access,,,app_disa,,))
$(eval $(call BuildAsterisk13Module,app-exec,Exec application,support for application execution,,,app_exec,,))
$(eval $(call BuildAsterisk13Module,app-minivm,Minimal voicemail system,a voicemail system in small building blocks working together based on the Comedian Mail voicemail,,extensions_minivm.conf minivm.conf,app_minivm,,))
$(eval $(call BuildAsterisk13Module,app-mixmonitor,Record a call and mix the audio,record a call and mix the audio during the recording,,,app_mixmonitor,,))
$(eval $(call BuildAsterisk13Module,app-originate,Originate a call,originating an outbound call and connecting it to a specified extension or application,,,app_originate,,))
$(eval $(call BuildAsterisk13Module,app-playtones,Playtones application,play a tone list,,,app_playtones,,))
$(eval $(call BuildAsterisk13Module,app-queue,True Call Queueing,support for ACD,,queues.conf queuerules.conf,app_queue,))
$(eval $(call BuildAsterisk13Module,app-read,Variable read,a trivial application to read a variable,,,app_read,,))
$(eval $(call BuildAsterisk13Module,app-readexten,Extension to variable,a trivial application to read an extension into a variable,,,app_readexten,,))
$(eval $(call BuildAsterisk13Module,app-record,Record sound file,to record a sound file,,,app_record,,))
$(eval $(call BuildAsterisk13Module,app-sayunixtime,Say Unix time,an application to say Unix time,,,app_sayunixtime,,))
$(eval $(call BuildAsterisk13Module,app-senddtmf,Send DTMF digits,Sends arbitrary DTMF digits,,,app_senddtmf,,))
$(eval $(call BuildAsterisk13Module,app-sms,SMS,SMS support (ETSI ES 201 912 protocol 1),+libpopt +libstdcpp,,app_sms,,))
$(eval $(call BuildAsterisk13Module,app-speech,Dialplan Speech,Dialplan Speech Applications,+asterisk13-res-speech,,app_speech_utils,,))
$(eval $(call BuildAsterisk13Module,app-stack,Stack applications,Stack applications Gosub Return etc.,+asterisk13-res-agi,,app_stack,,))
$(eval $(call BuildAsterisk13Module,app-system,System exec,support for executing system commands,,,app_system,,))
$(eval $(call BuildAsterisk13Module,app-talkdetect,File playback with audio detect,for file playback with audio detect,,,app_talkdetect,,))
$(eval $(call BuildAsterisk13Module,app-verbose,Verbose logging,Verbose logging application,,,app_verbose,,))
$(eval $(call BuildAsterisk13Module,app-waituntil,Sleep,support sleeping until the given epoch,,,app_waituntil,,))
$(eval $(call BuildAsterisk13Module,app-while,While loop,a while loop implementation,,,app_while,,))
$(eval $(call BuildAsterisk13Module,bridge-builtin-features,Bridging features,built in bridging features,,,bridge_builtin_features))
$(eval $(call BuildAsterisk13Module,bridge-builtin-interval-features,Built in bridging interval features,built in bridging interval features,,,bridge_builtin_interval_features))
$(eval $(call BuildAsterisk13Module,bridge-holding,Bridging for storing channels in a bridge,bridging technology for storing channels in a bridge,,,bridge_holding))
$(eval $(call BuildAsterisk13Module,bridge-native-rtp,Native RTP bridging technology module,native RTP bridging technology module,,,bridge_native_rtp))
$(eval $(call BuildAsterisk13Module,bridge-simple,Simple two channel bridging module,simple two channel bridging module,,,bridge_simple))
$(eval $(call BuildAsterisk13Module,bridge-softmix,Multi-party software based channel mixing,multi-party software based channel mixing,,,bridge_softmix))
$(eval $(call BuildAsterisk13Module,cdr,Provides CDR,Call Detail Record,,cdr.conf cdr_custom.conf cdr_manager.conf cdr_syslog.conf,app_cdr app_forkcdr cdr_custom cdr_manager cdr_syslog func_cdr,,))
$(eval $(call BuildAsterisk13Module,cdr-csv,Provides CDR CSV,Call Detail Record with CSV support,,,cdr_csv,,))
$(eval $(call BuildAsterisk13Module,cdr-sqlite3,Provides CDR SQLITE3,Call Detail Record with SQLITE3 support,libsqlite3,,cdr_sqlite3_custom,,))
$(eval $(call BuildAsterisk13Module,chan-alsa,ALSA channel,the channel chan_alsa,+alsa-lib,alsa.conf,chan_alsa,,))
$(eval $(call BuildAsterisk13Module,chan-dahdi,DAHDI channel,DAHDI channel support,+dahdi-tools-libtonezone +kmod-dahdi +libpri @!aarch64,chan_dahdi.conf,chan_dahdi,,))
$(eval $(call BuildAsterisk13Module,chan-iax2,IAX2 channel,IAX support,+asterisk13-res-timing-timerfd,iax.conf iaxprov.conf,chan_iax2,,))
$(eval $(call BuildAsterisk13Module,chan-lantiq,Lantiq TAPI channel,An implementation of Lantiq TAPI channel,@TARGET_lantiq +kmod-ltq-vmmc,lantiq.conf,chan_lantiq,,))
$(eval $(call BuildAsterisk13Module,chan-motif,Jingle channel,Motif Jingle Channel Driver,+asterisk13-res-xmpp,motif.conf,chan_motif,,))
$(eval $(call BuildAsterisk13Module,chan-oss,OSS channel,the channel chan_oss,,oss.conf,chan_oss,,))
$(eval $(call BuildAsterisk13Module,chan-sip,SIP channel,the channel chan_sip,+asterisk13-app-confbridge,sip.conf sip_notify.conf,chan_sip,,))
$(eval $(call BuildAsterisk13Module,chan-skinny,Skinny channel,the channel chan_skinny,,skinny.conf,chan_skinny,,))
$(eval $(call BuildAsterisk13Module,chan-unistim,Unistim channel,channel driver for the UNISTIM (Unified Networks IP Stimulus) protocol,,unistim.conf,chan_unistim,,))
$(eval $(call BuildAsterisk13Module,codec-a-mu,Alaw to ulaw translation,translation between alaw and ulaw codecs,,,codec_a_mu,,))
$(eval $(call BuildAsterisk13Module,codec-adpcm,ADPCM text,ADPCM text ,,,codec_adpcm,,))
$(eval $(call BuildAsterisk13Module,codec-alaw,Signed linear to alaw translation,translation between signed linear and alaw codecs,,,codec_alaw,,))
$(eval $(call BuildAsterisk13Module,codec-dahdi,DAHDI codec,DAHDI native transcoding support,+asterisk13-chan-dahdi,,codec_dahdi,,))
$(eval $(call BuildAsterisk13Module,codec-g722,G.722,a high bit rate 48/56/64Kbps ITU standard codec,,,codec_g722,,))
$(eval $(call BuildAsterisk13Module,codec-g726,Signed linear to G.726 translation,translation between signed linear and ITU G.726-32kbps codecs,,,codec_g726,,))
$(eval $(call BuildAsterisk13Module,codec-gsm,linear to GSM translation,translate between signed linear and GSM,,,codec_gsm,,))
$(eval $(call BuildAsterisk13Module,codec-ilbc,linear to ILBC translation,translate between signed linear and ILBC,,,codec_ilbc,,))
$(eval $(call BuildAsterisk13Module,codec-lpc10,Linear to LPC10 translation,translate between signed linear and LPC10,,,codec_lpc10,,))
$(eval $(call BuildAsterisk13Module,codec-resample,resample sLinear audio,resample sLinear audio,,,codec_resample,,))
$(eval $(call BuildAsterisk13Module,codec-ulaw,Signed linear to ulaw translation,translation between signed linear and ulaw codecs,,,codec_ulaw,,))
$(eval $(call BuildAsterisk13Module,curl,CURL,CURL support,+libcurl,,func_curl res_config_curl res_curl,,))
$(eval $(call BuildAsterisk13Module,format-g726,G.726,support for headerless G.726 16/24/32/40kbps data format,,,format_g726,,))
$(eval $(call BuildAsterisk13Module,format-g729,G.729,support for raw headerless G729 data,,,format_g729,,))
$(eval $(call BuildAsterisk13Module,format-gsm,GSM format,support for GSM format,,,format_gsm,,))
$(eval $(call BuildAsterisk13Module,format-h263,H263 format,support for H264 format,,,format_h263,,))
$(eval $(call BuildAsterisk13Module,format-h264,H264 format,support for H264 format,,,format_h264,,))
$(eval $(call BuildAsterisk13Module,format-ilbc,ILBC format,support for ILBC format,,,format_ilbc,,))
$(eval $(call BuildAsterisk13Module,format-pcm,PCM format,support for PCM format,,,format_pcm,,))
$(eval $(call BuildAsterisk13Module,format-sln,Raw slinear format,support for raw slinear format,,,format_sln,,))
$(eval $(call BuildAsterisk13Module,format-vox,VOX format,support for ADPCM vox format,,,format_vox,,))
$(eval $(call BuildAsterisk13Module,format-wav,WAV format (8000hz Signed Linear),support for proprietary Microsoft WAV format (8000hz Signed Linear),,,format_wav,,))
$(eval $(call BuildAsterisk13Module,format-wav-gsm,WAV format (Proprietary GSM),support for proprietary Microsoft WAV format (Proprietary GSM),,,format_wav_gsm,,))
$(eval $(call BuildAsterisk13Module,func-base64,base64 support,support of base64 function,,,func_base64,,))
$(eval $(call BuildAsterisk13Module,func-blacklist,Blacklist on callerid,looking up the callerid number and see if it is blacklisted,,,func_blacklist,,))
$(eval $(call BuildAsterisk13Module,func-channel,Channel info,Channel info dialplan function,,,func_channel,,))
$(eval $(call BuildAsterisk13Module,func-cut,CUT function,CUT function,,,func_cut,,))
$(eval $(call BuildAsterisk13Module,func-db,Database interaction,functions for interaction with the database,,,func_db app_db,))
$(eval $(call BuildAsterisk13Module,func-devstate,Blinky lights control,functions for manually controlled blinky lights,,,func_devstate,,))
$(eval $(call BuildAsterisk13Module,func-enum,ENUM,ENUM,,enum.conf,func_enum,))
$(eval $(call BuildAsterisk13Module,func-env,Environment functions,Environment dialplan functions,,,func_env,))
$(eval $(call BuildAsterisk13Module,func-extstate,Hinted extension state,retrieving the state of a hinted extension for dialplan control,,,func_extstate,,))
$(eval $(call BuildAsterisk13Module,func-global,Global variable,global variable dialplan functions,,,func_global,,))
$(eval $(call BuildAsterisk13Module,func-groupcount,Group count,for counting number of channels in the specified group,,,func_groupcount,,))
$(eval $(call BuildAsterisk13Module,func-math,Math functions,Math functions,,,func_math,))
$(eval $(call BuildAsterisk13Module,func-module,Simple module check function,Simple module check function,,,func_module,))
$(eval $(call BuildAsterisk13Module,func-periodic-hook,Periodic dialplan hooks,Execute a periodic dialplan hook into the audio of a call,,,func_periodic_hook,,))
$(eval $(call BuildAsterisk13Module,func-presencestate,Hinted presence state,Gets or sets a presence state in the dialplan,,,func_presencestate,,))
$(eval $(call BuildAsterisk13Module,func-rand,RAND dialplan function,RAND dialplan function,,,func_rand,,))
$(eval $(call BuildAsterisk13Module,func-realtime,REALTIME dialplan function,REALTIME dialplan function,,,func_realtime,,))
$(eval $(call BuildAsterisk13Module,func-shell,Shell,support for shell execution,,,func_shell,,))
$(eval $(call BuildAsterisk13Module,func-uri,URI encoding and decoding,Encodes and decodes URI-safe strings,,,func_uri,,))
$(eval $(call BuildAsterisk13Module,func-vmcount,vmcount dialplan,a vmcount dialplan function,,,func_vmcount,,))
$(eval $(call BuildAsterisk13Module,odbc,ODBC,ODBC support,+libpthread +libc +unixodbc,cdr_adaptive_odbc.conf cdr_odbc.conf cel_odbc.conf func_odbc.conf res_odbc.conf,cdr_adaptive_odbc cdr_odbc cel_odbc func_odbc res_config_odbc res_odbc res_odbc_transaction,,))
$(eval $(call BuildAsterisk13Module,pbx-ael,Asterisk Extension Logic,support for symbolic Asterisk Extension Logic,,extensions.ael,pbx_ael,,))
$(eval $(call BuildAsterisk13Module,pbx-dundi,Dundi,provides Dundi Lookup service for Asterisk,,dundi.conf,pbx_dundi,,))
$(eval $(call BuildAsterisk13Module,pbx-lua,Lua,provides Lua resources for Asterisk,+liblua,extensions.lua,pbx_lua,,))
$(eval $(call BuildAsterisk13Module,pbx-realtime,Realtime Switch,realtime switch support,,,pbx_realtime,,))
$(eval $(call BuildAsterisk13Module,pbx-spool,Call Spool,outgoing call spool support,,,pbx_spool,,))
$(eval $(call BuildAsterisk13Module,pgsql,PostgreSQL,PostgreSQL support,+libpq @!arc,cel_pgsql.conf cdr_pgsql.conf res_pgsql.conf,cel_pgsql cdr_pgsql res_config_pgsql,,))
$(eval $(call BuildAsterisk13Module,pjsip,pjsip channel,the channel pjsip,+asterisk13-res-sorcery +asterisk13-res-pjproject +libpjsip +libpjmedia +libpjnath +libpjsip-simple +libpjsip-ua +libpjsua +libpjsua2,pjsip.conf pjsip_notify.conf pjsip_wizard.conf,chan_pjsip func_pjsip_aor func_pjsip_contact func_pjsip_endpoint res_pjsip res_pjsip_acl res_pjsip_authenticator_digest res_pjsip_caller_id res_pjsip_config_wizard res_pjsip_dialog_info_body_generator res_pjsip_diversion res_pjsip_dlg_options res_pjsip_dtmf_info res_pjsip_empty_info res_pjsip_endpoint_identifier_anonymous res_pjsip_endpoint_identifier_ip res_pjsip_endpoint_identifier_user res_pjsip_exten_state res_pjsip_header_funcs res_pjsip_history res_pjsip_logger res_pjsip_messaging res_pjsip_mwi res_pjsip_mwi_body_generator res_pjsip_nat res_pjsip_notify res_pjsip_one_touch_record_info res_pjsip_outbound_authenticator_digest res_pjsip_outbound_publish res_pjsip_outbound_registration res_pjsip_path res_pjsip_pidf_body_generator res_pjsip_pidf_digium_body_supplement res_pjsip_pidf_eyebeam_body_supplement res_pjsip_publish_asterisk res_pjsip_pubsub res_pjsip_refer res_pjsip_registrar res_pjsip_registrar_expire res_pjsip_rfc3326 res_pjsip_sdp_rtp res_pjsip_send_to_voicemail res_pjsip_session res_pjsip_sips_contact res_pjsip_t38 res_pjsip_transport_management res_pjsip_transport_websocket res_pjsip_xpidf_body_generator,,))
$(eval $(call BuildAsterisk13Module,res-adsi,Provide ADSI,Analog Display Services Interface capability,,,res_adsi,,))
$(eval $(call BuildAsterisk13Module,res-ael-share,Shareable AEL code,support for shareable AEL code mainly between internal and external modules,,,res_ael_share,,))
$(eval $(call BuildAsterisk13Module,res-agi,Asterisk Gateway Interface,Support for the Asterisk Gateway Interface extension,+asterisk13-res-speech,,res_agi,,))
$(eval $(call BuildAsterisk13Module,res-calendar,Calendaring API,Calendaring support (ICal and Google Calendar),,calendar.conf,res_calendar,,))
$(eval $(call BuildAsterisk13Module,res-clioriginate,Calls via CLI,Originate calls via the CLI,,,res_clioriginate,,))
$(eval $(call BuildAsterisk13Module,res-hep,HEPv3 API,Routines for integration with Homer using HEPv3,,,res_hep,,))
$(eval $(call BuildAsterisk13Module,res-hep-pjsip,PJSIP HEPv3 Logger,PJSIP logging with Homer,+asterisk13-res-hep +asterisk13-pjsip,,res_hep_pjsip,,))
$(eval $(call BuildAsterisk13Module,res-hep-rtcp,RTCP HEPv3 Logger,RTCP logging with Homer,+asterisk13-res-hep,,res_hep_rtcp,,))
$(eval $(call BuildAsterisk13Module,res-fax-spandsp,Spandsp T.38 and G.711,Spandsp T.38 and G.711 FAX Resource,+asterisk13-res-fax +libspandsp +libtiff,,res_fax_spandsp,,))
$(eval $(call BuildAsterisk13Module,res-fax,FAX modules,Generic FAX resource for FAX technology resource modules,+asterisk13-res-timing-pthread,res_fax.conf,res_fax,,))
$(eval $(call BuildAsterisk13Module,res-http-websocket,HTTP websocket support,WebSocket support for the Asterisk internal HTTP server,,,res_http_websocket,,))
$(eval $(call BuildAsterisk13Module,res-monitor,Provide Monitor,Cryptographic Signature capability,,,res_monitor,,))
$(eval $(call BuildAsterisk13Module,res-musiconhold,MOH,Music On Hold support,,musiconhold.conf,res_musiconhold,,))
$(eval $(call BuildAsterisk13Module,res-parking,Phone Parking,Phone Parking application,,res_parking.conf,res_parking,,))
$(eval $(call BuildAsterisk13Module,res-phoneprov,Phone Provisioning,Phone provisioning application for the asterisk internal http server,,phoneprov.conf,res_phoneprov,,))
$(eval $(call BuildAsterisk13Module,res-pjsip-phoneprov,PJSIP Phone Provisioning,PJSIP Phone Provisioning,+asterisk13-pjsip +asterisk13-res-phoneprov,,res_pjsip_phoneprov_provider,,))
$(eval $(call BuildAsterisk13Module,res-pjproject,Bridge PJPROJECT to Asterisk logging,,+libpj +libpjlib-util +libpjmedia +libpjmedia +libpjnath +libpjsip-simple +libpjsip-ua +libpjsip +libpjsua +libpjsua2 +libsrtp,pjproject.conf,res_pjproject,,))
$(eval $(call BuildAsterisk13Module,res-realtime,RealTime CLI,RealTime CLI,,,res_realtime,,))
$(eval $(call BuildAsterisk13Module,res-rtp-asterisk,RTP stack,Supports RTP and RTCP with Symmetric RTP support for NAT traversal,+libpjsip +libpjmedia +libpjnath +libpjsip-simple +libpjsip-ua +libpjsua +libpjsua2,rtp.conf,res_rtp_asterisk,,))
$(eval $(call BuildAsterisk13Module,res-rtp-multicast,RTP multicast engine,Multicast RTP Engine,,,res_rtp_multicast,,))
$(eval $(call BuildAsterisk13Module,res-smdi,Provide SMDI,Simple Message Desk Interface capability,,smdi.conf,res_smdi,,))
$(eval $(call BuildAsterisk13Module,res-sorcery,Sorcery data layer,Sorcery backend modules for data access intended for using realtime as backend ,,,res_sorcery_astdb res_sorcery_config res_sorcery_memory res_sorcery_realtime,,))
$(eval $(call BuildAsterisk13Module,res-speech,Speech Recognition API,Support for the Asterisk Generic Speech Recognition API,,,res_speech,,))
$(eval $(call BuildAsterisk13Module,res-srtp,SRTP Support,Secure RTP connection,+libsrtp,,res_srtp,,))
$(eval $(call BuildAsterisk13Module,res-stun-monitor,STUN monitoring,resource STUN Monitor,,res_stun_monitor.conf,res_stun_monitor,,))
$(eval $(call BuildAsterisk13Module,res-timing-dahdi,DAHDI Timing Interface,DAHDI timing interface,+asterisk13-chan-dahdi,,res_timing_dahdi,,))
$(eval $(call BuildAsterisk13Module,res-timing-pthread,pthread Timing Interface,POSIX pthreads Timing Interface,,,res_timing_pthread,,))
$(eval $(call BuildAsterisk13Module,res-timing-timerfd,Timerfd Timing Interface,Timing interface provided by Linux kernel,,,res_timing_timerfd,,))
$(eval $(call BuildAsterisk13Module,res-xmpp,XMPP client and component module,reference module for interfacting Asterisk directly as a client or component with XMPP server,+libiksemel +libopenssl,xmpp.conf,res_xmpp,,))
$(eval $(call BuildAsterisk13Module,voicemail,Voicemail,voicemail related modules,+asterisk13-res-adsi +asterisk13-res-smdi,voicemail.conf,app_voicemail,vm-*,))

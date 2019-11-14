#
# Copyright (C) 2009-2016 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
#
include $(TOPDIR)/rules.mk


ifneq ($(PACKAGE_$(PKG_NAME)-mod-perl),)
include $(TOPDIR)/feeds/packages/lang/perl/perlmod.mk
endif


PKG_NAME:=freeswitch
PKG_SOURCE_PROTO:=git
PKG_DISTNAME:=$(PKG_NAME)_$(PKG_SOURCE_PROTO)
PKG_VERSION:=1.7.0


#
# The latest FS git hash in PKG_SOURCE_VERSION can be obtained from
# http://fisheye.freeswitch.org
#
PKG_SOURCE_URL:=https://$(PKG_NAME).org/stash/scm/fs/$(PKG_NAME).git
PATCH_DIR:=./patches/$(CONFIG_LIBC)


FS_WITH_DEFAULT_HEAD:=cc0a062204cb51311619cd37f51b32b2f266729f
FS_WITH_DEFAULT_HEAD_SHORT:=$(shell echo $(FS_WITH_DEFAULT_HEAD)|cut -b -7)
PKG_SOURCE_VERSION:=$(if $(CONFIG_FS_WITH_LATEST_HEAD),$(shell git ls-remote $(PKG_SOURCE_URL) HEAD | cut -f1),$(FS_WITH_DEFAULT_HEAD))
PKG_SOURCE_VERSION_SHORT:=$(shell echo $(PKG_SOURCE_VERSION)|cut -b -7)
PKG_RELEASE:=$(PKG_SOURCE_VERSION_SHORT)
PKG_SOURCE:=$(PKG_DISTNAME)-$(FS_WITH_DEFAULT_HEAD_SHORT).tar.xz
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_SOURCE_SUBDIR)


TAR_OPTIONS+= --strip-components=1 -C $(PKG_BUILD_DIR)


PKG_FIXUP:=autoreconf
PKG_BUILD_PARALLEL:=0


PKG_LICENSE:=MPL-1.1
PKG_LICENSE_FILES:=COPYING


ifeq ($(CONFIG_DEVEL),y)
	export QUILT=1
endif


FS_MOD_AVAILABLE:= \
	abstraction \
	alsa \
	amr \
	amrwb \
	av \
	avmd \
	basic \
	bert \
	blacklist \
	bv \
	callcenter \
	cdr-csv \
	cdr-mongodb \
	cdr-pg-csv \
	cdr-sqlite \
	cepstral \
	cidlookup \
	cluechoo \
	codec2 \
	commands \
	conference \
	console \
	curl \
	cv \
	dahdi-codec \
	db \
	dialplan-asterisk \
	dialplan-directory \
	dialplan-xml \
	dingaling \
	directory \
	distributor \
	dptools \
	easyroute \
	enum \
	erlang-event \
	esf \
	esl \
	event-multicast \
	event-socket \
	event-zmq \
	expr \
	fifo \
	file-string \
	flite \
	format-cdr \
	freetdm \
	fsk \
	fsv \
	g723-1 \
	g729 \
	graylog2 \
	gsmopen \
	h26x \
	h323 \
	hash \
	hiredis \
	html5 \
	httapi \
	http-cache \
	ilbc \
	isac \
	java \
	json-cdr \
	kazoo \
	lcr \
	ldap \
	local-stream \
	logfile \
	loopback \
	lua \
	managed \
	memcache \
	mongo \
	mp4 \
	mp4v \
	native-file \
	nibblebill \
	opus \
	oreka \
	perl \
	png \
	portaudio \
	portaudio-stream \
	posix-timer \
	prefix \
	python \
	radius-cdr \
	rayo \
	redis \
	rss \
	rtc \
	rtmp \
	sangoma-codec \
	say-de \
	say-en \
	say-es \
	say-fa \
	say-fr \
	say-hu \
	say-it \
	say-nl \
	say-pt \
	say-ru \
	say-th \
	say-zh \
	shell-stream \
	silk \
	siren \
	skel \
	skinny \
	skypopen \
	smpp \
	sms \
	snapshot \
	sndfile \
	snmp \
	snom \
	sofia \
	sonar \
	spandsp \
	spidermonkey \
	spidermonkey-core-db \
	spidermonkey-curl \
	spidermonkey-odbc \
	spidermonkey-socket \
	spidermonkey-teletone \
	spy \
	ssml \
	stress \
	syslog \
	timerfd \
	tone-stream \
	translate \
	tts-commandline \
	unimrcp \
	valet-parking \
	verto \
	vmd \
	voicemail \
	voicemail-ivr \
	xml-cdr \
	xml-curl \
	xml-ldap \
	xml-radius \
	xml-rpc \
	xml-scgi \
	yaml \


PKG_CONFIG_DEPENDS:= \
	CONFIG_FS_WITH_LATEST_HEAD \
	CONFIG_FS_WITH_ALSA \
	CONFIG_FS_WITH_APR_CONFIG_SWITCHES \
	CONFIG_FS_WITH_APR_IPV6 \
	CONFIG_FS_WITH_APR_SCTP \
	CONFIG_FS_WITH_APR_THREADS \
	CONFIG_FS_WITH_BUILTIN_ZRTP \
	CONFIG_FS_WITH_CORE_LIBEDIT_SUPPORT \
	CONFIG_FS_WITH_DEFAULT_HEAD \
	CONFIG_FS_WITH_ERLANG \
	CONFIG_FS_WITH_FHS \
	CONFIG_FS_WITH_FIXED_POINT \
	CONFIG_FS_WITH_LOCAL_SOURCE \
	CONFIG_FS_WITH_LIBVPX \
	CONFIG_FS_WITH_LIBYUV \
	CONFIG_FS_WITH_LZMA \
	CONFIG_FS_WITH_MYSQL \
	CONFIG_FS_WITH_ODBC \
	CONFIG_FS_WITH_OGG \
	CONFIG_FS_WITH_OPENSSL \
	CONFIG_FS_WITH_OPENWRT_SOURCE \
	CONFIG_FS_WITH_OPT \
	CONFIG_FS_WITH_POSTGRESQL \
	CONFIG_FS_WITH_PYTHON \
	CONFIG_FS_WITH_PYTHON_PATH \
	CONFIG_FS_WITH_SILENT_RULES \
	CONFIG_FS_WITH_SQLITE2 \
	CONFIG_FS_WITH_SQLITE3 \
	CONFIG_FS_WITH_SRTP \
	CONFIG_FS_WITH_SRTP_GENERIC_AESICM \
	CONFIG_FS_WITH_SRTP_KERNEL_LINUX \
	CONFIG_FS_WITH_SRTP_OPENSSL \
	CONFIG_FS_WITH_STUN \
	CONFIG_FS_WITH_VISIBILITY \
	$(patsubst %,CONFIG_PACKAGE_$(PKG_NAME)-mod-%,$(FS_MOD_AVAILABLE)) \


include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/nls.mk
ifneq ($(CONFIG_FS_WITH_PYTHON),)
$(call include_mk, python3-package.mk)
endif


ifneq ($(CONFIG_CCACHE),)
TARGET_CC=$(TARGET_CC_NOCACHE)
TARGET_CXX=$(TARGET_CXX_NOCACHE)
endif


define Package/$(PKG_NAME)/Default
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=Telephony
  URL:=http://www.$(PKG_NAME).org/
  MAINTAINER:=Mazi Lo <openwrt.mazilo@recursor.net>
endef


define Package/$(PKG_NAME)
  $(call Package/$(PKG_NAME)/Default)
  MENU:=1
  TITLE:=FreeSWITCH open source telephony platform ($(PKG_SOURCE_VERSION_SHORT))
  DEPENDS:=+FS_WITH_CORE_LIBEDIT_SUPPORT:libedit +FS_WITH_OPENSSL:libopenssl +libcurl +libdb47 +libfreetype +libgdbm $(ICONV_DEPENDS) $(INTL_DEPENDS) +libjpeg +libncurses +libopenldap +libpcre +libpng +libpthread +librt +libspeex +libspeexdsp +FS_WITH_SQLITE2:libsqlite2 +FS_WITH_SQLITE3:libsqlite3 +libsrtp +SSP_SUPPORT:libssp +libstdcpp +libuuid +PACKAGE_$(PKG_NAME)-mod-perl:perl
endef


define Package/$(PKG_NAME)/description
  FreeSWITCH is an open source telephony platform designed to
  facilitate the creation of voice and chat driven products 
  scaling from a soft-phone up to a soft-switch.
endef


define Package/$(PKG_NAME)/config
  source "$(SOURCE)/Config.in"
endef


define Package/$(PKG_NAME)-mod-spandsp/config
    config FS_WITH_FIXED_POINT
    depends on PACKAGE_$(PKG_NAME)-mod-spandsp
    bool "Enable fixed point support for spandsp (may degrade performance)"
    default n
    help
	Compile spandsp package with a fixed point support. ONLY enabled this
	option when necessary, i.e. CPU doesn't support floating computation.
endef


define Package/$(PKG_NAME)-example-config
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH example config (commented)
  DEPENDS:= $(PKG_NAME)
endef


define Package/$(PKG_NAME)-example-config/description
  The default configuration included with FreeSWITCH. It is not intended to be
  a final product, and is thus moved to /usr/share/$(PKG_NAME)/examples/conf to
  be an example of how FreeSWITCH can be configured.
endef


define Package/$(PKG_NAME)-config-upstream-defaults
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH upstream config
  DEPENDS:= $(PKG_NAME)
endef


define Package/$(PKG_NAME)-config-upstream-defaults/description
  The default configuration included with FreeSWITCH. It is not intended to be
  a final product, and but is included for those who don't want to use the
  OpenWRT configuration and/or want to try things as prepared by upstream.
endef


define Package/$(PKG_NAME)-config-minimal
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH minimal config
  DEPENDS:= $(PKG_NAME)
endef


define Package/$(PKG_NAME)-config-minimal/description
  A minimal configuration of FreeSWITCH for OpenWRT devices. Is what the UCI
  configuration began with.
endef


define Package/$(PKG_NAME)-collection-upstream-defaults
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FS upstream def sample conf
  DEPENDS:= $(PKG_NAME) \
		+$(PKG_NAME)-config-upstream-defaults \
		+$(PKG_NAME)-mod-callcenter \
		+$(PKG_NAME)-mod-codec2 \
		+$(PKG_NAME)-mod-commands \
		+$(PKG_NAME)-mod-conference \
		+$(PKG_NAME)-mod-curl \
		+$(PKG_NAME)-mod-dialplan-xml \
		+$(PKG_NAME)-mod-dingaling \
		+$(PKG_NAME)-mod-dptools \
		+$(PKG_NAME)-mod-event-socket \
		+$(PKG_NAME)-mod-g723-1 \
		+$(PKG_NAME)-mod-g729 \
		+$(PKG_NAME)-mod-hash \
		+$(PKG_NAME)-mod-http-cache \
		+$(PKG_NAME)-mod-ilbc \
		+$(PKG_NAME)-mod-local-stream \
		+$(PKG_NAME)-mod-lua \
		+$(PKG_NAME)-mod-native-file \
		+$(PKG_NAME)-mod-say-en \
		+$(PKG_NAME)-mod-sndfile \
		+$(PKG_NAME)-mod-sofia \
		+$(PKG_NAME)-mod-syslog \
		+$(PKG_NAME)-mod-tone-stream \
		+$(PKG_NAME)-mod-xml-curl \
		+$(PKG_NAME)-mod-xml-rpc \
		+$(PKG_NAME)-tools
endef


define Package/$(PKG_NAME)-collection-minimal
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH minimal package collection
  DEFAULT:=y
  DEPENDS:= $(PKG_NAME) \
		+$(PKG_NAME)-config-minimal \
		+$(PKG_NAME)-mod-codec2 \
		+$(PKG_NAME)-mod-commands \
		+$(PKG_NAME)-mod-dialplan-xml \
		+$(PKG_NAME)-mod-dptools \
		+$(PKG_NAME)-mod-event-socket \
		+$(PKG_NAME)-mod-hash \
		+$(PKG_NAME)-mod-local-stream \
		+$(PKG_NAME)-mod-sndfile \
		+$(PKG_NAME)-mod-sofia \
		+$(PKG_NAME)-mod-syslog \
		+$(PKG_NAME)-mod-tone-stream \
		+$(PKG_NAME)-tools
endef


define Package/$(PKG_NAME)-sounds-en-08KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 8KHz english sounds
  DEPENDS:= $(PKG_NAME)
endef


define Package/$(PKG_NAME)-sounds-en-16KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 16KHz english sounds
  DEPENDS:= +$(PKG_NAME)-sounds-en-08KHz
endef


define Package/$(PKG_NAME)-sounds-en-32KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 32KHz english sounds
  DEPENDS:= +$(PKG_NAME)-sounds-en-16KHz
endef


define Package/$(PKG_NAME)-sounds-en-48KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 48KHz english sounds
  DEPENDS:= +$(PKG_NAME)-sounds-en-32KHz
endef


define Package/$(PKG_NAME)-sounds-moh-08KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 8KHz music-on-hold sounds
  DEPENDS:= $(PKG_NAME)
endef


define Package/$(PKG_NAME)-sounds-moh-16KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 16KHz music-on-hold sounds
  DEPENDS:= +$(PKG_NAME)-sounds-moh-08KHz
endef


define Package/$(PKG_NAME)-sounds-moh-32KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 32KHz music-on-hold sounds
  DEPENDS:= +$(PKG_NAME)-sounds-moh-16KHz
endef


define Package/$(PKG_NAME)-sounds-moh-48KHz
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH 48KHz music-on-hold sounds
  DEPENDS:= +$(PKG_NAME)-sounds-moh-32KHz
endef


define Package/$(PKG_NAME)-tools
  $(call Package/$(PKG_NAME)/Default)
  TITLE:=FreeSWITCH control tools
  DEPENDS:= $(PKG_NAME)
endef


define Download/files
 define Download/$(1)
  FILE:=$(2)
  URL:=$(if $(4),$(4),http://files.$(PKG_NAME).org)/$(5)
  MD5SUM:=$(3)
 endef
 define Prepare/$(1)
  $(LN) $(DL_DIR)/$(2) $(PKG_BUILD_DIR)/$(6)
 endef
 $$(eval $$(call Download,$(1)))
endef


MAKE_INSTALL_TARGETS:=install


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-curl),)
 $(eval $(call Download/files,json-cdr,json-c-0.9.tar.gz,4653b3b9c568bb9c782178abfaac128d,,downloads/libs/,libs/))
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-event-zmq),)
 $(eval $(call Download/files,event-zmq,zeromq-2.1.9.tar.gz,94c5e0262a79c5f82bc0b178c1f8a33d,http://download.zeromq.org http://download.zeromq.org/historic,,libs/))
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-mongo),)
 $(eval $(call Download/files,mongo,mongo-c-driver-0.92.2.tar.gz,afab25a33abd3f1e0ba79c5a9da92651,,downloads/libs/,libs/))
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-08KHz),)
 $(eval $(call Download/files,sounds-en-08KHz,$(PKG_NAME)-sounds-en-us-callie-8000-1.0.50.tar.gz,14cccba7b5c56f027fea391f1e0ee2d6,,,))
	MAKE_INSTALL_TARGETS+= sounds-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-16KHz),)
 $(eval $(call Download/files,sounds-en-16KHz,$(PKG_NAME)-sounds-en-us-callie-16000-1.0.50.tar.gz,261f42e310baa97cc4dcfdc51971aff8,,,))
	MAKE_INSTALL_TARGETS+= hd-sounds-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-32KHz),)
 $(eval $(call Download/files,sounds-en-32KHz,$(PKG_NAME)-sounds-en-us-callie-32000-1.0.50.tar.gz,f204512fadc49a3267ff4a5e899b77a5,,,))
	MAKE_INSTALL_TARGETS+= uhd-sounds-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-48KHz),)
 $(eval $(call Download/files,sounds-en-48KHz,$(PKG_NAME)-sounds-en-us-callie-48000-1.0.50.tar.gz,8622a77a5d6dcf5e47bdf30d6a33e27c,,,))
	MAKE_INSTALL_TARGETS+= cd-sounds-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-08KHz),)
 $(eval $(call Download/files,sounds-moh-08KHz,$(PKG_NAME)-sounds-music-8000-1.0.50.tar.gz,c1f62dc26360f39f8eb442d5efd0a3aa,,,))
	MAKE_INSTALL_TARGETS+= moh-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-16KHz),)
 $(eval $(call Download/files,sounds-moh-16KHz,$(PKG_NAME)-sounds-music-16000-1.0.50.tar.gz,d253e26baa85e0e2e727a1cb1cca898e,,,))
	MAKE_INSTALL_TARGETS+= hd-moh-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-32KHz),)
 $(eval $(call Download/files,sounds-moh-32KHz,$(PKG_NAME)-sounds-music-32000-1.0.50.tar.gz,0e3ee69cb4ba2813ed0d3f7811332b63,,,))
	MAKE_INSTALL_TARGETS+= uhd-moh-install
endif


ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-48KHz),)
 $(eval $(call Download/files,sounds-moh-48KHz,$(PKG_NAME)-sounds-music-48000-1.0.50.tar.gz,873a62df1ab3be4191eb2488156ec2c2,,,))
	MAKE_INSTALL_TARGETS+= cd-moh-install
endif


#
# XXX: -fgnu89-inline is not supported on all compiler versions, so only enable it on avr32 where it solves build issues
#
ifneq ($(CONFIG_avr32),)
  TARGET_CFLAGS += -fgnu89-inline
  TARGET_CXXFLAGS += -fgnu89-inline
endif


ifneq ($(CONFIG_USE_EGLIBC),)
  ifeq ($(CONFIG_EGLIBC_OPTION_EGLIBC_UTMP),)
    TARGET_CFLAGS += -DNO_GETLOGIN
  endif
endif


ifeq ($(CONFIG_LIBC),musl)
	TARGET_CFLAGS += -D_GNU_SOURCE
endif


MAKE_FLAGS += \
	AR="$(TOOLCHAIN_DIR)/bin/$(TARGET_CROSS)ar" \
	CC="$(TOOLCHAIN_DIR)/bin/$(TARGET_CC)" \
	CXX="$(TOOLCHAIN_DIR)/bin/$(TARGET_CXX)" \
	RANLIB="$(TOOLCHAIN_DIR)/bin/$(TARGET_CROSS)ranlib" \
	CC_FOR_BUILD="$(HOSTCC)" \


MAKE_VARS += \
	OPT_FLAGS+="$(TARGET_CFLAGS)"


CONFIGURE_ARGS+= \
	--host=$(GNU_TARGET_NAME) \
	--build=$(GNU_HOST_NAME) \
	--prefix="/usr/share/$(PKG_NAME)" \
	--bindir="/usr/bin" \
	--libdir="/usr/lib" \
	--srcdir="$(PKG_BUILD_DIR)" \
	--sysconfdir="/etc" \
	--with-modinstdir="/usr/lib/$(PKG_NAME)" \
	--with-random="/dev/urandom" \
	$(call autoconf_bool,CONFIG_FS_WITH_BUILTIN_ZRTP,zrtp) \
	$(call autoconf_bool,CONFIG_FS_WITH_CORE_LIBEDIT_SUPPORT,core-libedit-support) \
	$(call autoconf_bool,CONFIG_FS_WITH_FHS,fhs) \
	$(call autoconf_bool,CONFIG_FS_WITH_APR_IPV6,ipv6) \
	$(call autoconf_bool,CONFIG_FS_WITH_LIBVPX,libvpx) \
	$(call autoconf_bool,CONFIG_FS_WITH_LIBYUV,libyuv) \
	$(call autoconf_bool,CONFIG_FS_WITH_LZMA,lzma) \
	$(call autoconf_bool,CONFIG_FS_WITH_ODBC,core-odbc-support) \
	$(call autoconf_bool,CONFIG_FS_WITH_OPT,optimization) \
	$(call autoconf_bool,CONFIG_FS_WITH_POSTGRESQL,core-pgsql-pkgconfig) \
	$(call autoconf_bool,CONFIG_FS_WITH_POSTGRESQL,core-pgsql-support) \
	$(call autoconf_bool,CONFIG_FS_WITH_SILENT_RULES,silent-rules) \
	$(call autoconf_bool,CONFIG_FS_WITH_SRTP,srtp) \
	$(call autoconf_bool,CONFIG_FS_WITH_SRTP_GENERIC_AESICM,generic-aesicm) \
	$(call autoconf_bool,CONFIG_FS_WITH_SRTP_KERNEL_LINUX,kernel-linux) \
	$(call autoconf_bool,CONFIG_FS_WITH_SRTP_OPENSSL,openssl) \
	$(call autoconf_bool,CONFIG_FS_WITH_STUN,stun) \
	$(call autoconf_bool,CONFIG_FS_WITH_APR_THREADS,threads) \
	$(call autoconf_bool,CONFIG_FS_WITH_VISIBILITY,visibility) \
	$(call autoconf_bool,CONFIG_PACKAGE_$(PKG_NAME)-mod-spandsp && CONFIG_FS_WITH_FIXED_POINT,fixed-point) \
	$(call autoconf_bool,CONFIG_PACKAGE_$(PKG_NAME)-mod-spandsp,builtin-tiff) \
	$(call autoconf_bool,CONFIG_PACKAGE_$(PKG_NAME)-mod-timerfd,timerfd-wrapper) \
	$(if $(CONFIG_FS_WITH_ODBC),--with-odbc-lib="$(STAGING_DIR)/usr/lib") \
	$(if $(CONFIG_FS_WITH_ODBC),--with-odbc="$(STAGING_DIR)/usr") \
	$(if $(CONFIG_FS_WITH_OGG),--with-ogg="$(STAGING_DIR)/usr") \
	$(if $(CONFIG_FS_WITH_OPENSSL),--with-openssl="pkg-config") \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-mod-erlang-event),--with-erlang="$(CONFIG_FS_WITH_ERLANG)",--without-erlang) \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-mod-java),--with-java="$(STAGING_DIR)/usr",--without-java) \
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-mod-python),--with-python="$(CONFIG_FS_WITH_PYTHON_PATH)",--without-python) \
	$(if ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-curl)|$(CONFIG_PACKAGE_$(PKG_NAME)-mod-xml-cdr)|$(CONFIG_PACKAGE_$(PKG_NAME)-mod-xml-curl)),--with-libcurl="$(STAGING_DIR)/usr",--without-libcurl) \
	--with$(if $(CONFIG_FS_WITH_MYSQL),,out)-mysql \
	--with$(if $(CONFIG_FS_WITH_POSTGRESQL),,out)-pgsql \
	--with$(if $(CONFIG_FS_WITH_SQLITE2),,out)-sqlite2 \
	--with$(if $(CONFIG_FS_WITH_SQLITE3),,out)-sqlite3 \


ifeq ($(CONFIG_TARGET_ARCH_PACKAGES),"kirkwood")
	CONFIGURE_ARGS+= --enable-arm5e-asm \

endif


#ifeq ($(CONFIG_LIBC),"uClibc")
ifeq ($(CONFIG_ARCH),"mips")
	CONFIGURE_VARS+= \
		ac_cv_file_dbd_apr_dbd_mysql_c="no" \

endif
#endif


define Build/Prepare
	$(call Build/Prepare/Default)
	$(call Prepare/event-zmq)
	$(call Prepare/json-cdr)
	$(call Prepare/mongo)
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-08KHz),$(call Prepare/sounds-en-08KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-16KHz),$(call Prepare/sounds-en-16KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-32KHz),$(call Prepare/sounds-en-32KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-en-48KHz),$(call Prepare/sounds-en-48KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-08KHz),$(call Prepare/sounds-moh-08KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-16KHz),$(call Prepare/sounds-moh-16KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-32KHz),$(call Prepare/sounds-moh-32KHz))
	$(if $(CONFIG_PACKAGE_$(PKG_NAME)-sounds-moh-48KHz),$(call Prepare/sounds-moh-48KHz))
ifeq ($(CONFIG_FS_WITH_LOCAL_SOURCE),y)
	(cd $(PKG_BUILD_DIR); $(if $(CONFIG_FS_WITH_LATEST_HEAD),if $(QUILT_CMD) top > /dev/null 2>&1 && [ -f $(PKG_BUILD_DIR)/patches/series ]; then $(QUILT_CMD) pop -a; fi; git pull;) if $(QUILT_CMD) next > /dev/null 2>&1; then $(QUILT_CMD) push -a; fi)
endif
	(cd $(PKG_BUILD_DIR); $(AM_TOOL_PATHS) ./bootstrap.sh)
endef


define Build/Configure
	$(foreach m,$(FS_MOD_AVAILABLE),
		$(if $(CONFIG_PACKAGE_$(PKG_NAME)-mod-$(m)),
			$(SED) 's|^[ #]*\([^#]*/mod_$(subst -,_,$(m))\)$$$$|\1|g' $(PKG_BUILD_DIR)/modules.conf
		,
			$(SED) 's|^\([^#]*/mod_$(subst -,_,$(m))\)$$$$|#\1|g' $(PKG_BUILD_DIR)/modules.conf
		)
	)
	$(call Build/Configure/Default)
endef


define Build/Compile
	$(call Build/Compile/Default, \
		DESTDIR="$(PKG_INSTALL_DIR)" \
		all $(MAKE_INSTALL_TARGETS) \
	)
	$(SED) 's|^\([ \t]*\)\(.*\)"mod_console"\(.*\)$$$$|\1<!-- \2"mod_console"\3 -->|' $(PKG_INSTALL_DIR)/etc/$(PKG_NAME)/autoload_configs/modules.conf.xml
	$(SED) 's|^\([ \t]*\)<!-- *\(.*\)"mod_syslog"\(.*\) *-->\(.*\)$$$$|\1\2"mod_syslog"\3\4|' $(PKG_INSTALL_DIR)/etc/$(PKG_NAME)/autoload_configs/modules.conf.xml
endef


define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/default
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)/scripts
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(INSTALL_BIN) ./files/$(PKG_NAME).init $(1)/etc/init.d/$(PKG_NAME)
	$(INSTALL_DATA) ./files/$(PKG_NAME).config $(1)/etc/config/$(PKG_NAME)
	$(INSTALL_DATA) ./files/$(PKG_NAME).default $(1)/etc/default/$(PKG_NAME)
	$(INSTALL_DATA) ./files/$(PKG_NAME).hotplug $(1)/etc/hotplug.d/iface/90-$(PKG_NAME)
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/$(PKG_NAME) $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib$(PKG_NAME)* $(1)/usr/lib/
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/$(PKG_NAME).pc $(1)/usr/lib/pkgconfig/
	$(if $(1)/usr/lib/lib$(PKG_NAME).la,$(STAGING_DIR_HOST)/bin/sed -i -re 's|$(STAGING_DIR)||g;s|$(TOOLCHAIN_DIR)||g;s|$(TARGET_CROSS)|usr|g;s|$(REAL_GNU_TARGET_NAME)|usr|g;s|-L$(PKG_BUILD_DIR)(.*)libzrtp ||g;:a;s|((-[IL]/\S+\s).*)\2|\1|;ta' $(1)/usr/lib/lib$(PKG_NAME).la)
endef


define Package/$(PKG_NAME)-collection-upstream-defaults/install
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
endef


define Package/$(PKG_NAME)-example-config/install
	$(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/examples/conf
	$(CP) $(PKG_INSTALL_DIR)/etc/$(PKG_NAME)/* $(1)/usr/share/$(PKG_NAME)/examples/conf/
endef


define Package/$(PKG_NAME)-config-upstream-defaults/install
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
	$(CP) $(PKG_INSTALL_DIR)/etc/$(PKG_NAME) $(1)/etc
endef


define Package/$(PKG_NAME)-collection-minimal/install
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
endef


define Package/$(PKG_NAME)-collection-uci-minimal/install
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)
endef


define Package/$(PKG_NAME)-config-minimal/install
	$(INSTALL_DIR) $(1)/etc/$(PKG_NAME)/autoload_configs

	#
	# Copying dialplan directory jingle_profiles sip_profiles $(PKG_NAME).xml
	# fur_elise.ttml mime.types tetris.ttml tones.conf vars.xml
	#
	$(foreach f,dialplan directory jingle_profiles sip_profiles $(PKG_NAME).xml fur_elise.ttml mime.types tetris.ttml vars.xml,$(if $(PKG_BUILD_DIR)/conf/vanilla/$(f),$(CP) $(PKG_BUILD_DIR)/conf/vanilla/$(f) $(1)/etc/$(PKG_NAME);))

	#
	# Copying acl ivr modules post_load_modules switch timezones
	#
	$(foreach f,acl ivr modules post_load_modules switch timezones,$(if $(PKG_BUILD_DIR)/conf/vanilla/autoload_configs/$(f).conf.xml,$(CP) $(PKG_BUILD_DIR)/conf/vanilla/autoload_configs/$(f).conf.xml $(1)/etc/$(PKG_NAME)/autoload_configs;))
endef


define Package/$(PKG_NAME)-sounds-en-08KHz/install
	#
	# sounds-en-8KHz
	#
	for dir in `(cd $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie;ls)`; \
	do \
		[ -d $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/8000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/8000; \
		$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/8000/* $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/8000; \
	done
endef


define Package/$(PKG_NAME)-sounds-en-16KHz/install
	#
	# sounds-en-16KHz
	#
	for dir in `(cd $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie;ls)`; \
	do \
		[ -d $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/16000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/16000; \
		$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/16000/* $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/16000; \
	done
endef


define Package/$(PKG_NAME)-sounds-en-32KHz/install
	#
	# sounds-en-32KHz
	#
	for dir in `(cd $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie;ls)`; \
	do \
		[ -d $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/32000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/32000; \
		$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/32000/* $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/32000; \
	done
endef


define Package/$(PKG_NAME)-sounds-en-48KHz/install
	#
	# sounds-en-48KHz
	#
	for dir in `(cd $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie;ls)`; \
	do \
		[ -d $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/48000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/48000; \
		$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/48000/* $(1)/usr/share/$(PKG_NAME)/sounds/en/us/callie/$$$${dir}/48000; \
	done
endef


define Package/$(PKG_NAME)-sounds-moh-08KHz/install
	#
	# sounds-moh-8KHz
	#
	[ -d $(1)/usr/share/$(PKG_NAME)/sounds/music/8000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/music/8000;
	$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/music/8000 $(1)/usr/share/$(PKG_NAME)/sounds/music;
endef


define Package/$(PKG_NAME)-sounds-moh-16KHz/install
	#
	# sounds-moh-16KHz
	#
	[ -d $(1)/usr/share/$(PKG_NAME)/sounds/music/16000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/music/16000;
	$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/music/16000 $(1)/usr/share/$(PKG_NAME)/sounds/music;
endef


define Package/$(PKG_NAME)-sounds-moh-32KHz/install
	#
	# sounds-moh-32KHz
	#
	[ -d $(1)/usr/share/$(PKG_NAME)/sounds/music/32000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/music/32000;
	$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/music/32000 $(1)/usr/share/$(PKG_NAME)/sounds/music;
endef


define Package/$(PKG_NAME)-sounds-moh-48KHz/install
	#
	# sounds-moh-48KHz
	#
	[ -d $(1)/usr/share/$(PKG_NAME)/sounds/music/48000 ] || $(INSTALL_DIR) $(1)/usr/share/$(PKG_NAME)/sounds/music/48000;
	$(CP) $(PKG_INSTALL_DIR)/usr/share/$(PKG_NAME)/sounds/music/48000 $(1)/usr/share/$(PKG_NAME)/sounds/music;
endef


define Package/$(PKG_NAME)-tools/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/fs_cli $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/fs_encode $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/fs_ivrd $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/fsxs $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/gentls_cert $(1)/usr/bin/
	$(CP) $(PKG_INSTALL_DIR)/usr/bin/tone2wav $(1)/usr/bin/
	$(STAGING_DIR_HOST)/bin/sed -i -re 's|$(ICONV_LDFLAGS)||g;s|$(INTL_LDFLAGS)||g;s|$(PKG_BUILD_DIR)|/usr|g;s|$(STAGING_DIR)||g;s|$(STAGING_DIR_HOST)|/usr|g;s|$(TOOLCHAIN_DIR)||g;s|$(TARGET_CROSS)|usr|g;s|$(REAL_GNU_TARGET_NAME)|usr|g;:a;s|((-[IL]/\S+\s).*)[ ,\n]\1|\1|;ta' $(1)/usr/bin/fsxs
endef


# 1. Name
# 2. Title
# 3. Selection on module configuration file
# 4. Files (module files include different names)
# 5. Files (supporting files include different names)
# 6. Inter module depends
# 7. Extra depends
#
define BuildPlugin
  define Package/$(PKG_NAME)-mod-$(1)
    $$(call Package/$(PKG_NAME)/Default)
    TITLE:= FS $(2) module
    DEPENDS:= $(PKG_NAME) $(foreach m,$(6),+$(PKG_NAME)-mod-$(m)) $(7)
  endef

  define Package/$(PKG_NAME)-mod-$(1)/install
	[ -z "mod_$(subst -,_,$(1))" ] || $(INSTALL_DIR) $$(1)/usr/lib/$(PKG_NAME)

	for f in $$$$(realpath $$$$(wildcard $(PKG_INSTALL_DIR)/usr/lib/$(PKG_NAME)/mod_$(subst -,_,$(1)).*)) $$$$(wildcard $(PKG_INSTALL_DIR)/usr/lib/$(PKG_NAME)/$(4)_*); \
	do \
		$(CP) $$$$$$$${f} $$(1)/usr/lib/$(PKG_NAME)/; \
	done

	#
	# mod_$(subst -,_,$(1)): $(5)
	#
	$(foreach f,$(5),[ -d $(dir $$(1)/usr/${f}) ] || $(INSTALL_DIR) $(dir $$(1)/usr/${f});$(CP) $$$$(wildcard $(PKG_INSTALL_DIR)/usr/${f}*) $(dir $$(1)/usr/${f});)

	#
	# mod_$(subst -,_,$(1)): Clean installed library control files from referencig to OpenWRT PATH.
	#
	$(STAGING_DIR_HOST)/bin/sed -i -re 's|$(STAGING_DIR)||g;s|$(TOOLCHAIN_DIR)||g;s|$(TARGET_CROSS)|usr|g;s|$(REAL_GNU_TARGET_NAME)|usr|g;s|-L$(PKG_BUILD_DIR)(.*)libzrtp ||g;s|-L$(PKG_BUILD_DIR)(.*)libtiff ||g;:a;s|((-[IL]/\S+\s).*)\2|\1|;ta' $(foreach l,`find $$(1)/usr/lib -name "*.la" -print`,$(l))

	#
	# 1. mod_$(subst -,_,$(1)) XML configuration files from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/conf/$(3)/autoload_configs/$(subst -,_,$(1)).conf.xml)))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/conf/$(3)/autoload_configs/$(subst -,_,$(1)).conf.xml)),[ -d $$(1)/etc/$(PKG_NAME)/autoload_configs ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(CP) $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/conf/$(3)/autoload_configs/$(subst -,_,$(1)).conf.xml)) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(SED) 's|/usr/local/lib|/usr/lib|g;s|/usr/local|/etc|g' $$(1)/etc/$(PKG_NAME)/autoload_configs/$(subst -,_,$(1)).conf.xml)

	#
	# 2. mod_$(subst -,_,$(1)) XML configuration files from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/$(3)/$(1)/*/$(subst -,_,$(1)).conf.xml)))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/$(3)/$(1)/*/$(subst -,_,$(1)).conf.xml)),[ -d $$(1)/etc/$(PKG_NAME)/autoload_configs ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(CP) $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/$(3)/$(1)/*/$(subst -,_,$(1)).conf.xml)) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(SED) 's|/usr/local/lib|/usr/lib|g;s|/usr/local|/etc|g' $$(1)/etc/$(PKG_NAME)/autoload_configs/$(subst -,_,$(1)).conf.xml)

	#
	# 3. mod_$(subst -,_,$(1)) XML configuration files from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf/autoload_configs/$(subst -,_,$(1)).conf.xml)))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf/autoload_configs/$(subst -,_,$(1)).conf.xml)),[ -d $$(1)/etc/$(PKG_NAME)/autoload_configs ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(CP) $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf/autoload_configs/$(subst -,_,$(1)).conf.xml)) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(SED) 's|/usr/local/lib|/usr/lib|g;s|/usr/local|/etc|g' $$(1)/etc/$(PKG_NAME)/autoload_configs/$(subst -,_,$(1)).conf.xml)

	#
	# 4. mod_$(subst -,_,$(1)) XML configuration files from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf*/$(subst -,_,$(1)).conf.xml)))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf*/$(subst -,_,$(1)).conf.xml)),[ -d $$(1)/etc/$(PKG_NAME)/autoload_configs ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(CP) $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/conf*/$(subst -,_,$(1)).conf.xml)) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(SED) 's|/usr/local/lib|/usr/lib|g;s|/usr/local|/etc|g' $$(1)/etc/$(PKG_NAME)/autoload_configs/$(subst -,_,$(1)).conf.xml)

	#
	# 5. mod_$(subst -,_,$(1)) XML configuration files from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/$(subst -,_,$(1)).conf.xml)))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/$(subst -,_,$(1)).conf.xml)),[ -d $$(1)/etc/$(PKG_NAME)/autoload_configs ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(CP) $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/src/$(3)/*/mod_$(subst -,_,$(1))/$(subst -,_,$(1)).conf.xml)) $$(1)/etc/$(PKG_NAME)/autoload_configs;$(SED) 's|/usr/local/lib|/usr/lib|g;s|/usr/local|/etc|g' $$(1)/etc/$(PKG_NAME)/autoload_configs/$(subst -,_,$(1)).conf.xml)

	#
	# 6. Additional configuration files for mod_$(subst -,_,$(1)) from: $$$$(patsubst $(PKG_BUILD_DIR)/%,%,$$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/conf/$(3)/$(1)/*.$(1))))
	#
	$$$$(if $$$$(firstword $$$$(wildcard $(PKG_BUILD_DIR)/conf/$(3)/$(1)/*.$(1))),[ -d $$(1)/etc/$(PKG_NAME)/$(1) ] || $(INSTALL_DIR) $$(1)/etc/$(PKG_NAME)/$(1);$(CP) $(PKG_BUILD_DIR)/conf/$(3)/$(1) $$(1)/etc/$(PKG_NAME))
  endef

  define Package/$(PKG_NAME)-mod-$(1)/postinst
#!/bin/sh
sed -i -e 's|^\([ \t]*\)<!-- *\(.*\)"mod_$(subst -,_,$(1))"\(.*\) *-->\(.*\)$$$$|\1\2"mod_$(subst -,_,$(1))"\3\4|' $$$${IPKG_INSTROOT}/etc/$(PKG_NAME)/autoload_configs/modules.conf.xml
  endef

  define Package/$(PKG_NAME)-mod-$(1)/prerm
#!/bin/sh
sed -i -e 's|^\([ \t]*\)\(.*\)"mod_$(subst -,_,$(1))"\(.*\)$$$$|\1<!-- \2"mod_$(subst -,_,$(1))"\3 -->|' $$$${IPKG_INSTROOT}/etc/$(PKG_NAME)/autoload_configs/modules.conf.xml
  endef

  $$(eval $$(call BuildPackage,$(PKG_NAME)-mod-$(1)))
endef


$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-collection-upstream-defaults))
$(eval $(call BuildPackage,$(PKG_NAME)-collection-minimal))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-en-08KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-en-16KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-en-32KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-en-48KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-moh-08KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-moh-16KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-moh-32KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-sounds-moh-48KHz))
$(eval $(call BuildPackage,$(PKG_NAME)-tools))
$(eval $(call BuildPackage,$(PKG_NAME)-example-config))
$(eval $(call BuildPackage,$(PKG_NAME)-config-upstream-defaults))
$(eval $(call BuildPackage,$(PKG_NAME)-config-minimal))


#
# $(eval $(call BuildPlugin,Name,Title,Selection on module Conf File,Files,Inter Depends,Extra Depends))
#
$(eval $(call BuildPlugin,abstraction,An Abstraction To API Call,vanilla,,,,))
$(eval $(call BuildPlugin,alsa,Alsa Endpoint,vanilla,,,,+FS_WITH_ALSA:alsa-lib @FS_WITH_ALSA))
$(eval $(call BuildPlugin,amr,Adaptive Multi-Rate On Bandwidth Codec,vanilla,,,,))
$(eval $(call BuildPlugin,amrwb,Wide Band AMR,vanilla,,,,))
$(eval $(call BuildPlugin,av,FS Video Codec / File Format,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,avmd,Advanced Voice Mail Detection,vanilla,,,,))
$(eval $(call BuildPlugin,basic,BASIC,vanilla,,,,))
$(eval $(call BuildPlugin,bert,Naive Bit Error Rate Tester,vanilla,,,,))
$(eval $(call BuildPlugin,blacklist,Blacklist,vanilla,,,,))
$(eval $(call BuildPlugin,bv,BroadVoice 16/32-bit Audio Codec,vanilla,,,,@BROKEN)) # Requires unsupported libbroadvoice
$(eval $(call BuildPlugin,callcenter,Call Center,vanilla,,,,))
$(eval $(call BuildPlugin,cdr-csv,CSV-CDR Handler,mod,,,,)) # ~/conf
$(eval $(call BuildPlugin,cdr-mongodb,MongoDB CDR logger,vanilla,,,,))
$(eval $(call BuildPlugin,cdr-pg-csv,PostgreSQL CDR Handler,vanilla,,,,+coreutils +FS_WITH_POSTGRESQL:libpq @FS_WITH_POSTGRESQL))
$(eval $(call BuildPlugin,cdr-sqlite,SQLite3 CDR Handler,vanilla,,,,@FS_WITH_SQLITE3))
$(eval $(call BuildPlugin,cepstral,Cepstral Interface,vanilla,,,,@BROKEN)) # needs <swift.h>
$(eval $(call BuildPlugin,cidlookup,Data Query For CID->NAME Services,mod,,,,+libcurl)) # ~/conf
$(eval $(call BuildPlugin,cluechoo,Framework Demo,vanilla,,,,))
$(eval $(call BuildPlugin,codec2,CoDec 2,,,,,@BROKEN)) # Requires unsupported libcodec2
$(eval $(call BuildPlugin,commands,API Commands,vanilla,,,,))
$(eval $(call BuildPlugin,conference,Conference Room,vanilla,,,,))
$(eval $(call BuildPlugin,console,Console Logger,vanilla,,,,))
$(eval $(call BuildPlugin,curl,HTTP Request,vanilla,,,,+libcurl))
$(eval $(call BuildPlugin,cv,Detect Video motion,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,dahdi-codec,DAHDI Codecs,vanilla,,,,))
$(eval $(call BuildPlugin,db,Database Backend,vanilla,,,,+libdb47))
$(eval $(call BuildPlugin,dialplan-asterisk,Asterisk Dialplan Parser,vanilla,,,,))
$(eval $(call BuildPlugin,dialplan-directory,Dialplan Directory,vanilla,,,,))
$(eval $(call BuildPlugin,dialplan-xml,Dialplan-XML Interface,vanilla,,,,))
$(eval $(call BuildPlugin,dingaling,Jabber Interface,vanilla,,,,+libopenssl))
$(eval $(call BuildPlugin,directory,Search By Name Directory IVR,vanilla,,,,))
$(eval $(call BuildPlugin,distributor,Simple Robin-Round Load,vanilla,,,,))
$(eval $(call BuildPlugin,dptools,Dialplan Tools,vanilla,,,,))
$(eval $(call BuildPlugin,easyroute,Simple DID Routing,vanilla,,,,))
$(eval $(call BuildPlugin,enum,ENUM Routing,,,,,+libldns))
$(eval $(call BuildPlugin,erlang-event,Erlang Event Handler,vanilla,,,,+erlang))
$(eval $(call BuildPlugin,esf,Extra SIP Functionality,vanilla,,,,))
$(eval $(call BuildPlugin,esl,Allow to generate remote ESL commands,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,event-multicast,Multicast Event Handler,vanilla,,,,))
$(eval $(call BuildPlugin,event-socket,Socket Event Handler,vanilla,,,,))
$(eval $(call BuildPlugin,event-zmq,Socket Event Handler By Zero MQ,vanilla,,,,+libuuid))
$(eval $(call BuildPlugin,expr,Expression Evaluation,vanilla,,,,))
$(eval $(call BuildPlugin,fifo,FIFO,vanilla,,,,))
$(eval $(call BuildPlugin,file-string,Streaming Multiple Sound Files Sequentially,vanilla,,,,@OBSOLETE)) # merged into dptools
$(eval $(call BuildPlugin,flite,Festival TTS,vanilla,,,,+flite @BROKEN)) # flite is from old package repo
$(eval $(call BuildPlugin,format-cdr,XML CDR Module to files or curl,vanilla,,,,))
$(eval $(call BuildPlugin,fsk,Bell-202 1200-Baud FSK Decoder,vanilla,,,,))
$(eval $(call BuildPlugin,fsv,Video Player / Recorder,vanilla,,,,+libyuv @BROKEN)) # Requires unsupported libyuv.
$(eval $(call BuildPlugin,g723-1,G.723.1 Codec,vanilla,,,,))
$(eval $(call BuildPlugin,g729,G.729 Codec,vanilla,,,,))
$(eval $(call BuildPlugin,graylog2,Graylog2 GELF logger,vanilla,,,,))
$(eval $(call BuildPlugin,gsmopen,GSM Modem compatible Endpoint,mod,,,alsa,+FS_WITH_ALSA:alsa-lib +FS_WITH_LZMA:liblzma @FS_WITH_ALSA @BROKEN)) # needs gsmlib
$(eval $(call BuildPlugin,h26x,H26X Linear Codec,vanilla,,,,))
$(eval $(call BuildPlugin,h323,H323 Endpoint,mod,,,,@BROKEN)) # Req. H323Plus v1.24.x or newer
$(eval $(call BuildPlugin,hash,API For Manipulating A Hash Table,vanilla,,,,))
$(eval $(call BuildPlugin,hiredis,Redis DB access,vanilla,,,,+hiredis @BROKEN))
#$(eval $(call BuildPlugin,html5,HTML5 Endpoint Module,vanilla,,,,)) # defunct?
$(eval $(call BuildPlugin,httapi,HT Telephony API and HTTP Caching,mod,,,,)) # ~/conf
$(eval $(call BuildPlugin,http-cache,HTTP GET With Caching,mod,,,,+libcurl)) # ~/conf
$(eval $(call BuildPlugin,ilbc,ILBC Codec,vanilla,,,,+libilbc @BROKEN)) # Requires unsupported libilbc.
$(eval $(call BuildPlugin,isac,iSAC Codec,vanilla,,,,))
$(eval $(call BuildPlugin,java,Java Language Interface,vanilla,,,,@BROKEN)) # needs java
$(eval $(call BuildPlugin,json-cdr,JSon-CDR Interface,mod,,,,+libcurl))
$(eval $(call BuildPlugin,kazoo,Socket Controlled Event Handler,vanilla,,,,+erlang))
$(eval $(call BuildPlugin,ldap,LDAP interface,vanilla,,,,+libopenldap))
$(eval $(call BuildPlugin,lcr,Least Cost Routing,vanilla,,,,))
$(eval $(call BuildPlugin,local-stream,Multi-Channel On Same Stream,vanilla,,,,))
$(eval $(call BuildPlugin,logfile,File Logger,vanilla,,,,))
$(eval $(call BuildPlugin,loopback,Loopback to Dialplan Endpoint,vanilla,,,,))
$(eval $(call BuildPlugin,lua,LUA Language Interface,vanilla,,,,+liblua))
$(eval $(call BuildPlugin,managed,Media Switching Software Library,vanilla,,,,+glib2 @BROKEN)) # needs Mono
$(eval $(call BuildPlugin,memcache,MemCached Interface,vanilla,,,,+libmemcached @BROKEN)) # Req host libmemcached
$(eval $(call BuildPlugin,mongo,A Document-Oriented Database,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,mp4,MP4 File Format Support For Video,vanilla,,,,@BROKEN)) # needs host libmp4v2
$(eval $(call BuildPlugin,mp4v,MP4 CoDec Support For Video,vanilla,,,,))
$(eval $(call BuildPlugin,native-file,WAV Format Sound Player,vanilla,,,,))
$(eval $(call BuildPlugin,nibblebill,Credit / Debit Billing,vanilla,,,,))
$(eval $(call BuildPlugin,opus,Opus CoDec,vanilla,,,,+libopus))
$(eval $(call BuildPlugin,oreka,Media Recording with Oreka,vanilla,,,,))
$(eval $(call BuildPlugin,png,play a png as video with audio,vanilla,,,,+libpng))
$(eval $(call BuildPlugin,perl,Perl Language Interface,vanilla,,lib/perl5/$(PERL_VERSION)/$(PKG_NAME).la lib/perl5/$(PERL_VERSION)/$(PKG_NAME).pm lib/perl5/$(PERL_VERSION)/$(PKG_NAME).so,,+perl +libdb47 +libgdbm @BROKEN))
$(eval $(call BuildPlugin,portaudio,Portaudio To Sound Card Interface,vanilla,,,alsa,+FS_WITH_ALSA:alsa-lib @FS_WITH_ALSA @BROKEN)) # needs portaudio
$(eval $(call BuildPlugin,portaudio-stream,Portaudio Streaming Interface,vanilla,,,alsa,+FS_WITH_ALSA:alsa-lib @FS_WITH_ALSA @BROKEN)) # needs portaudio
$(eval $(call BuildPlugin,posix-timer,POSIX Compliant Soft Timer,vanilla,,,,))
$(eval $(call BuildPlugin,prefix,longest-prefix match in store,mod,,,,))
$(eval $(call BuildPlugin,python,Python Language Interface,vanilla,,lib/python$(PYTHON3_VERSION)/site-packages/freeswitch.py,,+FS_WITH_PYTHON:python3 @FS_WITH_PYTHON))
$(eval $(call BuildPlugin,radius-cdr,Radius-CDR interface,vanilla,,,,@BROKEN)) # fails in freeradius-client
$(eval $(call BuildPlugin,rayo,Rayo server & node implementation,vanilla,,,,))
$(eval $(call BuildPlugin,redis,Redis limit backend,vanilla,,,,))
$(eval $(call BuildPlugin,rss,RRS Feeds via TTS,vanilla,,,,))
$(eval $(call BuildPlugin,rtc,RTC endpoint,vanilla,,,,))
$(eval $(call BuildPlugin,rtmp,RTMP Protocol Handler,vanilla,,,,))
$(eval $(call BuildPlugin,sangoma-codec,Sangoma Codec,vanilla,,,,@BROKEN)) # Req. Sangoma CoDec source
$(eval $(call BuildPlugin,say-de,German Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-en,English Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-es,Spanish Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-fa,Persian Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-fr,French Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-hu,Hungarian Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-it,Italian Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-nl,Dutch Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-pt,Portugeese Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-ru,Russian Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-th,Thai Say,vanilla,,,,))
$(eval $(call BuildPlugin,say-zh,Chineese Say,vanilla,,,,))
$(eval $(call BuildPlugin,shell-stream,Streaming Audio Through CLI,vanilla,,,,))
$(eval $(call BuildPlugin,silk,Skype(TM) SILK Codec Module,vanilla,,,,+libsilk @BROKEN)) # Requires unsupported libsilk
$(eval $(call BuildPlugin,siren,G.722.1 Codec,vanilla,,,,+libg7221 @BROKEN)) # Requires unsupported libg7221
$(eval $(call BuildPlugin,skel,Template For New Module,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,skinny,Skinny Call Control Protocol (SCCP),vanilla,,,,))
$(eval $(call BuildPlugin,skypopen,Skype Compatible Endpoint,mod,,,,@FEATURE_drawing-backend_libX11))
$(eval $(call BuildPlugin,sms,SMS,vanilla,,,,))
$(eval $(call BuildPlugin,smpp,smpp client and server implementation,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,snapshot,Record Audio Snapshot to File,vanilla,,,,))
$(eval $(call BuildPlugin,sndfile,Multi-Format Sound File,vanilla,,,,+libsndfile))
$(eval $(call BuildPlugin,snmp,SNMP AgentX Subagent,vanilla,,,,+libnetsnmp))
$(eval $(call BuildPlugin,snom,SNOM specific features,vanilla,,,,))
$(eval $(call BuildPlugin,sofia,SOFIA SIP,mod,,,,)) # ~/conf
$(eval $(call BuildPlugin,sonar,Sonar Ping Timer,vanilla,,,,))
$(eval $(call BuildPlugin,spandsp,Span DSP,mod,,,,+libjpeg +FS_WITH_LZMA:liblzma @FS_WITH_LZMA)) # ~/conf
$(eval $(call BuildPlugin,spidermonkey,JavaScript,vanilla,,,,@BROKEN)) # fails in js
$(eval $(call BuildPlugin,spidermonkey-core_db,JavaScript DB,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,spidermonkey-curl,JavaScript Curl,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,spidermonkey-odbc,JavaScript ODBC,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,spidermonkey-socket,JavaScript Socket,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,spidermonkey-teletone,JavaScript Teletone,vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,spy,User Spy,vanilla,,,,))
$(eval $(call BuildPlugin,ssml,SSML audio rendering format,mod,,,rayo,))
$(eval $(call BuildPlugin,stress,Voice Stress Detection,vanilla,,,,))
$(eval $(call BuildPlugin,syslog,SysLog logger,vanilla,,,,))
$(eval $(call BuildPlugin,timerfd,Linux Kernel timerfd API,vanilla,,,,))
$(eval $(call BuildPlugin,tone-stream,Tone Generation Stream,vanilla,,,,))
$(eval $(call BuildPlugin,translate,Translate,vanilla,,,,))
$(eval $(call BuildPlugin,tts-commandline,ASR TTS Command Interface,vanilla,,,,))
$(eval $(call BuildPlugin,unimrcp,UniMRCP (MRCP Client),vanilla,,,,@BROKEN))
$(eval $(call BuildPlugin,valet-parking,Valet Parking Application,vanilla,,,,))
$(eval $(call BuildPlugin,verto,HTML5 Verto interface,vanilla,,lib/perl5/$(PERL_VERSION)/MCAST.so lib/perl5/$(PERL_VERSION)/MCAST.la lib/perl5/$(PERL_VERSION)/MCAST.pm,perl rtc,+perl @BROKEN))
$(eval $(call BuildPlugin,vmd,VoiceMail Beep Detection,vanilla,,,,))
$(eval $(call BuildPlugin,voicemail,VoiceMail,vanilla,,,,))
$(eval $(call BuildPlugin,voicemail-ivr,VoiceMail IVR,vanilla,,,,))
$(eval $(call BuildPlugin,xml-cdr,XML-CDR Handler,vanilla,,,,+libcurl))
$(eval $(call BuildPlugin,xml-curl,XML-Curl Gateway,vanilla,,,,+libcurl))
$(eval $(call BuildPlugin,xml-ldap,LDAP-XML Gateway,vanilla,,,,+PACKAGE_$(PKG_NAME)-mod-ldap:libopenldap))
$(eval $(call BuildPlugin,xml-rpc,XML-RPC Interface,vanilla,,,,))
$(eval $(call BuildPlugin,xml-radius,Radius authentication and authorization,vanilla,,,,+freeradius-client @BROKEN)) # freeradius-client isn't yet supported by OpenWRT.
$(eval $(call BuildPlugin,xml-scgi,SCGI XML Gateway,vanilla,,,,))
$(eval $(call BuildPlugin,yaml,YAML language,vanilla,,,,+libyaml))

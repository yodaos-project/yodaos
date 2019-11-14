#
# Copyright (C) 2017 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PRG_NAME:=freeswitch
PKG_NAME:=$(PRG_NAME)-stable
PKG_VERSION:=1.6.19
PKG_RELEASE:=1
PKG_MAINTAINER:=Sebastian Kemper <sebastian_ml@gmx.net>

PKG_SOURCE:=$(PRG_NAME)-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://files.$(PRG_NAME).org/releases/$(PRG_NAME)
PKG_HASH:=e8d271a0d876ba4cca551682af0a4f5047bfd79438fcebdcc17d00711c04a8ff

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PRG_NAME)-$(PKG_VERSION)

# configure fails without libjpeg, but it's only needed for mod_spandsp
PKG_BUILD_DEPENDS:= \
	libjpeg \
	perl/host \
	python/host

# With mod_ssml and mod_rayo enabled the parallel compiles always failed
#PKG_BUILD_PARALLEL:=1
PKG_INSTALL:=1

PKG_LICENSE:= \
	AGPL-3.0 \
	Apache-2.0 \
	BSD-2-Clause \
	BSD-3-Clause \
	BSD-4-Clause \
	BSD-like \
	Beerware \
	GPL-1.0+ \
	GPL-2.0 \
	GPL-2.0+ \
	GPL-3.0 \
	ISC \
	LGPL-2.0+ \
	LGPL-2.1 \
	LGPL-2.1+ \
	MIT/X11 (BSD like) \
	MPL-1.1 \
	OpenLDAP \
	RSA-MD \
	zlib-acknowledgement

PKG_LICENSE_FILES:=debian/copyright

FS_STABLE_DATA_DIR:=/usr/share/$(PRG_NAME)

FS_STABLE_BIN_DIR:=/usr/bin
FS_STABLE_EXAMPLES_DIR:=$(FS_STABLE_DATA_DIR)/examples
FS_STABLE_FONTS_DIR:=$(FS_STABLE_DATA_DIR)/fonts
FS_STABLE_GRAMMAR_DIR:=$(FS_STABLE_DATA_DIR)/grammar
FS_STABLE_HTDOCS_DIR:=$(FS_STABLE_DATA_DIR)/htdocs
FS_STABLE_IMAGES_DIR:=$(FS_STABLE_DATA_DIR)/images
FS_STABLE_INCLUDES_DIR:=/usr/include/$(PRG_NAME)
FS_STABLE_LANG_DIR:=$(FS_STABLE_DATA_DIR)/lang
FS_STABLE_LIB_DIR:=/usr/lib
FS_STABLE_MOD_DIR:=$(FS_STABLE_LIB_DIR)/$(PRG_NAME)/mod
FS_STABLE_PKGCONFIG_DIR:=$(FS_STABLE_LIB_DIR)/pkgconfig
FS_STABLE_SCRIPTS_DIR:=$(FS_STABLE_DATA_DIR)/scripts
FS_STABLE_SOUNDS_DIR:=$(FS_STABLE_DATA_DIR)/sounds
FS_STABLE_SYSCONF_DIR:=/etc
FS_STABLE_TLS_DIR:=$(FS_STABLE_SYSCONF_DIR)/$(PRG_NAME)/tls
FS_STABLE_TZ_DIR:=$(FS_STABLE_DATA_DIR)/tz

# Maintain a list of all non-module packages so they can be added to
# PKG_CONFIG_DEPENDS.
FS_STABLE_MISC_AVAILABLE:= \
	example-curl \
	example-insideout \
	example-minimal \
	example-rayo \
	example-sbc \
	example-softphone \
	example-testing \
	example-vanilla \
	lang-de \
	lang-en \
	lang-es \
	lang-fr \
	lang-he \
	lang-pt \
	lang-ru \
	lang-sv \
	misc-fonts \
	misc-grammar \
	misc-hotplug \
	misc-images \
	misc-perl-esl \
	misc-python-esl \
	misc-timezones \
	util-fs_cli \
	util-fs_encode \
	util-fs_ivrd \
	util-gentls_cert \
	util-tone2wav

FS_STABLE_MOD_AVAILABLE:= \
	abstraction \
	alsa \
	amr \
	amrwb \
	av \
	avmd \
	b64 \
	basic \
	bert \
	blacklist \
	callcenter \
	cdr_csv \
	cdr_mongodb \
	cdr_pg_csv \
	cdr_sqlite \
	cidlookup \
	clearmode \
	cluechoo \
	commands \
	conference \
	console \
	curl \
	cv \
	dahdi_codec \
	db \
	dialplan_asterisk \
	dialplan_directory \
	dialplan_xml \
	dingaling \
	directory \
	distributor \
	dptools \
	easyroute \
	enum \
	erlang_event \
	esf \
	esl \
	event_multicast \
	event_socket \
	event_test \
	event_zmq \
	expr \
	fifo \
	format_cdr \
	fsk \
	fsv \
	g723_1 \
	g729 \
	graylog2 \
	h26x \
	hash \
	hiredis \
	httapi \
	http_cache \
	isac \
	json_cdr \
	kazoo \
	lcr \
	ldap \
	local_stream \
	logfile \
	loopback \
	lua \
	mp4v \
	native_file \
	nibblebill \
	odbc_cdr \
	opus \
	oreka \
	perl \
	png \
	pocketsphinx \
	portaudio \
	portaudio_stream \
	posix_timer \
	prefix \
	python \
	radius_cdr \
	random \
	rayo \
	redis \
	rss \
	rtc \
	rtmp \
	say_de \
	say_en \
	say_es \
	say_es_ar \
	say_fa \
	say_fr \
	say_he \
	say_hr \
	say_hu \
	say_it \
	say_ja \
	say_nl \
	say_pl \
	say_pt \
	say_ru \
	say_sv \
	say_th \
	say_zh \
	shell_stream \
	shout \
	skinny \
	sms \
	snapshot \
	sndfile \
	snmp \
	snom \
	sofia \
	sonar \
	spandsp \
	spy \
	ssml \
	stress \
	syslog \
	theora \
	tone_stream \
	translate \
	tts_commandline \
	unimrcp \
	v8 \
	valet_parking \
	verto \
	vmd \
	voicemail \
	voicemail_ivr \
	xml_cdr \
	xml_curl \
	xml_ldap \
	xml_rpc \
	xml_scgi \
	yaml \
	yuv

PKG_CONFIG_DEPENDS:= \
	$(patsubst %,CONFIG_PACKAGE_$(PKG_NAME)-%,$(FS_STABLE_MISC_AVAILABLE)) \
	$(patsubst %,CONFIG_PACKAGE_$(PKG_NAME)-mod-%,$(FS_STABLE_MOD_AVAILABLE)) \
	CONFIG_FS_STABLE_WITH_DEBUG \
	CONFIG_FS_STABLE_WITH_FREETYPE \
	CONFIG_FS_STABLE_WITH_LIBEDIT \
	CONFIG_FS_STABLE_WITH_LIBYUV \
	CONFIG_FS_STABLE_WITH_MODCONF \
	CONFIG_FS_STABLE_WITH_ODBC \
	CONFIG_FS_STABLE_WITH_PGSQL \
	CONFIG_FS_STABLE_WITH_PNG \
	CONFIG_FS_STABLE_WITH_SRTP \
	CONFIG_FS_STABLE_WITH_V8 \
	CONFIG_FS_STABLE_WITH_V8_SNAPSHOT \
	CONFIG_FS_STABLE_WITH_VPX \
	CONFIG_FS_STABLE_WITH_ZRTP \
	CONFIG_LIBC \
	CONFIG_SOFT_FLOAT

include $(INCLUDE_DIR)/package.mk

FS_STABLE_PERL_FEED:=$(TOPDIR)/feeds/packages/lang/perl

$(call include_mk, python-host.mk)
include $(FS_STABLE_PERL_FEED)/perlmod.mk

FS_STABLE_PERL_LIBS:=$(shell grep "^libs=" \
	$(FS_STABLE_PERL_FEED)/files/base.config | \
			sed "s/^libs=//;s/'//g")

FS_STABLE_PYTHON_SITE_DIR:=$(FS_STABLE_LIB_DIR)/python$(PYTHON_VERSION)/site-packages

define Download/files
define Download/$(1)
  FILE:=$(2)
  URL:=$(3)
  HASH:=$(4)
endef
$$(eval $$(call Download,$(1)))
endef

define Package/$(PKG_NAME)/install/bin
	$(INSTALL_DIR) $(1)$(FS_STABLE_BIN_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)$(FS_STABLE_BIN_DIR)/$(2) \
					$(1)$(FS_STABLE_BIN_DIR)
endef

define Package/$(PKG_NAME)/install/dir
	if [ -d $(2) ]; then $(INSTALL_DIR) $(1); fi
	for dir in $$$$(shell [ -d $(2) ] && cd $(2) && $(FIND) -type d -print | sed 's|^./\?||'); \
	do \
		$(INSTALL_DIR) $(1)/$$$$$$$$dir; \
	done
	for file in $$$$(shell [ -d $(2) ] && cd $(2) && $(FIND) -type f -print | sed 's|^./||'); \
	do \
		$(INSTALL_DATA) $(2)/$$$$$$$$file $(1)/$$$$$$$$file; \
	done
endef

define Package/$(PKG_NAME)/install/lib
	$(INSTALL_DIR) $(1)$(FS_STABLE_LIB_DIR)
	$(CP) $(PKG_INSTALL_DIR)$(FS_STABLE_LIB_DIR)/$(2).so* \
					$(1)$(FS_STABLE_LIB_DIR)
endef

define Package/$(PKG_NAME)/install/mod
	$(INSTALL_DIR) $(1)$(FS_STABLE_MOD_DIR)
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)$(FS_STABLE_MOD_DIR)/mod_$(2).so \
						$(1)$(FS_STABLE_MOD_DIR)
endef

define Package/$(PKG_NAME)/config
	source "$(SOURCE)/Config.in"
endef

define Package/$(PKG_NAME)/Default
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  URL:=https://www.$(PRG_NAME).org
endef

define Package/$(PKG_NAME)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Open source telephony platform, v$(PKG_VERSION)
  MENU:=1
  USERID:=$(PRG_NAME)=372:$(PRG_NAME)=372
  DEPENDS:= \
	  +FS_STABLE_WITH_FREETYPE:libfreetype \
	  +FS_STABLE_WITH_LIBEDIT:libedit \
	  +FS_STABLE_WITH_ODBC:unixodbc \
	  +FS_STABLE_WITH_PGSQL:libpq \
	  +FS_STABLE_WITH_PNG:libpng \
	  +libcurl \
	  +libopenssl \
	  +libpcre \
	  +libpthread \
	  +librt \
	  +libspeex \
	  +libspeexdsp \
	  +libsqlite3 \
	  +libstdcpp \
	  +libuuid \
	  +zlib
  CONFLICTS:=$(PRG_NAME)
endef

define Package/$(PKG_NAME)/description
FreeSWITCH is a scalable open source cross-platform telephony platform
designed to route and interconnect popular communication protocols
using audio, video, text or any other form of media.
endef

define Package/$(PKG_NAME)/conffiles
$(FS_STABLE_SYSCONF_DIR)/$(PRG_NAME)
$(FS_STABLE_SYSCONF_DIR)/default/$(PRG_NAME)
$(FS_STABLE_SYSCONF_DIR)/init.d/$(PRG_NAME)
endef

define Package/$(PKG_NAME)/install
$(call Package/$(PKG_NAME)/install/bin,$(1),$(PRG_NAME))
$(call Package/$(PKG_NAME)/install/lib,$(1),lib$(PRG_NAME))
	$(INSTALL_DIR) $(1)$(FS_STABLE_FONTS_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_GRAMMAR_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_HTDOCS_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_IMAGES_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_SCRIPTS_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_SOUNDS_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_SYSCONF_DIR)/default
	$(INSTALL_DIR) $(1)$(FS_STABLE_SYSCONF_DIR)/init.d
	$(INSTALL_DIR) $(1)$(FS_STABLE_TLS_DIR)
	$(INSTALL_BIN) ./files/$(PRG_NAME).init \
		$(1)$(FS_STABLE_SYSCONF_DIR)/init.d/$(PRG_NAME)
	$(INSTALL_CONF) ./files/$(PRG_NAME).default \
		$(1)$(FS_STABLE_SYSCONF_DIR)/default/$(PRG_NAME)
ifeq ($(CONFIG_FS_STABLE_WITH_LIBEDIT),)
	$(SED) '/^  #procd_append_param command -nc -nf$$$$/s/#//' \
			$(1)$(FS_STABLE_SYSCONF_DIR)/init.d/$(PRG_NAME)
endif
endef

define Package/$(PKG_NAME)/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
  # Prevent autostart of $(PRG_NAME)
  touch $(FS_STABLE_SYSCONF_DIR)/$(PRG_NAME)_disabled

  echo
  echo "o-------------------------------------------------------------------o"
  echo "| FreeSWITCH note                                                   |"
  echo "o-------------------------------------------------------------------o"
  echo "| Edit /etc/default/freeswitch to change basic init configuration.  |"
  echo "o-------------------------------------------------------------=^_^=-o"
  echo
fi
exit 0
endef

define Package/$(PKG_NAME)-misc-hotplug
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Hotplug script
  DEPENDS:=$(PKG_NAME)
  PKGARCH:=all
endef

define Package/$(PKG_NAME)-misc-hotplug/description
This package includes a hotplug script for FreeSWITCH.
endef

define Package/$(PKG_NAME)-misc-hotplug/install
	$(INSTALL_DIR) $(1)$(FS_STABLE_SYSCONF_DIR)/hotplug.d/iface
	$(INSTALL_BIN) ./files/$(PRG_NAME).hotplug \
		$(1)$(FS_STABLE_SYSCONF_DIR)/hotplug.d/iface/99-$(PRG_NAME)
endef

define Package/$(PKG_NAME)-misc-hotplug/postinst
#!/bin/sh
if [ -z "$${IPKG_INSTROOT}" ]; then
  echo
  echo "o-------------------------------------------------------------------o"
  echo "| FreeSWITCH hotplug note                                           |"
  echo "o-------------------------------------------------------------------o"
  echo "| See /etc/default/freeswitch for hotplug hints.                    |"
  echo "o-------------------------------------------------------------=^_^=-o"
  echo
fi
exit 0
endef

define Package/$(PKG_NAME)-misc-perl-esl
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Perl ESL
  DEPENDS:=$(PKG_NAME) \
	  +PACKAGE_$(PKG_NAME)-misc-perl-esl:perlbase-autoloader \
	  +PACKAGE_$(PKG_NAME)-misc-perl-esl:perlbase-data \
	  +PACKAGE_$(PKG_NAME)-misc-perl-esl:perlbase-dynaloader \
	  @PERL_THREADS
endef

define Package/$(PKG_NAME)-misc-perl-esl/description
This package contains the Perl binding for FreeSWITCH's Event Socket
Library (ESL).
endef

define Package/$(PKG_NAME)-misc-perl-esl/install
	$(INSTALL_DIR) $(1)$(PERL_SITELIB)/ESL
	$(INSTALL_DIR) $(1)$(PERL_SITELIB)/auto/ESL
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)$(PERL_SITELIB)/ESL.so \
					$(1)$(PERL_SITELIB)/auto/ESL
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PERL_SITELIB)/ESL.pm \
					$(1)$(PERL_SITELIB)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PERL_SITELIB)/ESL/Dispatch.pm \
					$(1)$(PERL_SITELIB)/ESL
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(PERL_SITELIB)/ESL/IVR.pm \
					$(1)$(PERL_SITELIB)/ESL
endef

define Package/$(PKG_NAME)-misc-python-esl
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Python ESL
  DEPENDS:=$(PKG_NAME) +PACKAGE_$(PKG_NAME)-misc-python-esl:python-light
endef

define Package/$(PKG_NAME)-misc-python-esl/description
This package contains the Python binding for FreeSWITCH's Event Socket
Library (ESL).
endef

define Package/$(PKG_NAME)-misc-python-esl/install
	$(INSTALL_DIR) $(1)$(FS_STABLE_PYTHON_SITE_DIR)
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)$(FS_STABLE_PYTHON_SITE_DIR)/_ESL.so \
					$(1)$(FS_STABLE_PYTHON_SITE_DIR)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(FS_STABLE_PYTHON_SITE_DIR)/ESL.py \
					$(1)$(FS_STABLE_PYTHON_SITE_DIR)
endef

define Package/$(PKG_NAME)-misc-timezones
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Timezones file
  DEPENDS:=$(PKG_NAME)
  PKGARCH:=all
endef

define Package/$(PKG_NAME)-misc-timezones/description
This package includes a timezones file for FreeSWITCH.
endef

define Package/$(PKG_NAME)-misc-timezones/install
	$(INSTALL_DIR) $(1)$(FS_STABLE_TZ_DIR)
	$(INSTALL_DATA) \
	$(PKG_BUILD_DIR)/conf/vanilla/autoload_configs/timezones.conf.xml \
						$(1)$(FS_STABLE_TZ_DIR)
endef

define Package/$(PKG_NAME)/Example
define Package/$(PKG_NAME)-example-$(1)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=Example configuration
  DEPENDS:=$(PKG_NAME)
  PKGARCH:=all
endef
define Package/$(PKG_NAME)-example-$(1)/description
This package does not install any configuration for FreeSWITCH into
/etc/freeswitch. The system administrator is completely responsible
for that directory. If you install one of the example configuration
packages, it will install the corresponding sample configuration to
/usr/share/freeswitch/examples where you can take a look at it.
endef
define Package/$(PKG_NAME)-example-$(1)/install
$(call Package/$(PKG_NAME)/install/dir,$$(1)$(FS_STABLE_EXAMPLES_DIR)/$(1),$(PKG_BUILD_DIR)/conf/$(1))
endef
$$(eval $$(call BuildPackage,$(PKG_NAME)-example-$(1)))
endef

define Package/$(PKG_NAME)/Language
define Package/$(PKG_NAME)-lang-$(1)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=$(2) language files
  DEPENDS:=$(PKG_NAME)
  PKGARCH:=all
endef
define Package/$(PKG_NAME)-lang-$(1)/description
This package includes the $(2) language files for FreeSWITCH.
endef
define Package/$(PKG_NAME)-lang-$(1)/install
$(call Package/$(PKG_NAME)/install/dir,$$(1)$(FS_STABLE_LANG_DIR)/$(1),$(PKG_BUILD_DIR)/conf/vanilla/lang/$(1))
endef
$$(eval $$(call BuildPackage,$(PKG_NAME)-lang-$(1)))
endef

# The next package generator is for miscellaneous files that only
# require being copied from PKG_INSTALL_DIR to the ipkg.
define Package/$(PKG_NAME)/Misc
define Package/$(PKG_NAME)-$(1)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=$(2)
  DEPENDS:=$(PKG_NAME)
  ifeq ($(6),y)
  PKGARCH:=all
  endif
endef
define Package/$(PKG_NAME)-$(1)/description
$(subst \n,$(newline),$(3))
endef
define Package/$(PKG_NAME)-$(1)/install
$(call Package/$(PKG_NAME)/install/dir,$$(1)$(5),$(PKG_INSTALL_DIR)$(4))
endef
$$(eval $$(call BuildPackage,$(PKG_NAME)-$(1)))
endef

define Package/$(PKG_NAME)/Module
define Package/$(PKG_NAME)-mod-$(1)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=$(2) module
  DEPENDS:=$(PKG_NAME) $(patsubst +%,+PACKAGE_$(PKG_NAME)-mod-$(1):%,$(4))
endef
define Package/$(PKG_NAME)-mod-$(1)/description
$(subst \n,$(newline),$(3))
endef
define Package/$(PKG_NAME)-mod-$(1)/install
$(call Package/$(PKG_NAME)/install/mod,$$(1),$(1))
ifeq ($(CONFIG_FS_STABLE_WITH_MODCONF),y)
$(call Package/$(PKG_NAME)/install/dir,$$(1)$(FS_STABLE_EXAMPLES_DIR)/mod_$(1),$(PKG_BUILD_DIR)/src/mod/*/mod_$(1)/conf)
endif
ifeq ($(1),perl)
	$(INSTALL_DIR) $$(1)$(PERL_SITELIB)/auto/$(PRG_NAME)
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/perl/$(PRG_NAME).so \
					$$(1)$(PERL_SITELIB)/auto/$(PRG_NAME)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)/usr/perl/$(PRG_NAME).pm \
							$$(1)$(PERL_SITELIB)
endif
ifeq ($(1),python)
	$(INSTALL_DIR) $$(1)$(FS_STABLE_PYTHON_SITE_DIR)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(FS_STABLE_PYTHON_SITE_DIR)/$(PRG_NAME).py \
					$$(1)$(FS_STABLE_PYTHON_SITE_DIR)
endif
endef
$$(eval $$(call BuildPackage,$(PKG_NAME)-mod-$(1)))
endef

define Package/$(PKG_NAME)/Util
define Package/$(PKG_NAME)-util-$(1)
$(call Package/$(PKG_NAME)/Default)
  TITLE:=$(2) utility
  DEPENDS:=$(PKG_NAME) $(patsubst +%,+PACKAGE_$(PKG_NAME)-util-$(1):%,$(4))
  ifeq ($(5),y)
  PKGARCH:=all
  endif
endef
define Package/$(PKG_NAME)-util-$(1)/description
$(subst \n,$(newline),$(3))
endef
define Package/$(PKG_NAME)-util-$(1)/install
$(call Package/$(PKG_NAME)/install/bin,$$(1),$(1))
endef
ifeq ($(1)$(CONFIG_FS_STABLE_WITH_LIBEDIT),fs_cli)
define Package/$(PKG_NAME)-util-$(1)/postinst
#!/bin/sh
if [ -z "$$$${IPKG_INSTROOT}" ]; then
  echo
  echo "o-------------------------------------------------------------------o"
  echo "| fs_cli note                                                       |"
  echo "o-------------------------------------------------------------------o"
  echo "| Your FreeSWITCH was compiled without libedit support. Due to an   |"
  echo "| interop issue it's possible that when you try to exit fs_cli with |"
  echo "| Ctrl-D, you'll see messages flashing by and have to kill fs_cli   |"
  echo "| manually. To avoid this you can either use Ctrl-C instead (fs_cli |"
  echo "| needs to be started with '-i' for this to work; you could create  |"
  echo "| an appropriate alias in /etc/profile) or type /exit, /quit or     |"
  echo "| /bye.                                                             |"
  echo "o-------------------------------------------------------------=^_^=-o"
  echo
fi
exit 0
endef
endif
$$(eval $$(call BuildPackage,$(PKG_NAME)-util-$(1)))
endef

CONFIGURE_ARGS+= \
	--srcdir=$(PKG_BUILD_DIR) \
	--disable-dependency-tracking \
	--disable-static \
	--disable-system-xmlrpc-c \
	--enable-fhs \
	--with-cachedir=/tmp/$(PRG_NAME)/cache \
	--with-dbdir=/tmp/$(PRG_NAME)/db \
	--with-imagesdir=$(FS_STABLE_IMAGES_DIR) \
	--with-logfiledir=/tmp/$(PRG_NAME)/log \
	--with-recordingsdir=/tmp/$(PRG_NAME)/recordings \
	--with-storagedir=/tmp/$(PRG_NAME)/storage \
	$(call autoconf_bool,CONFIG_FS_STABLE_WITH_LIBEDIT,core-libedit-support) \
	$(call autoconf_bool,CONFIG_FS_STABLE_WITH_ODBC,core-odbc-support) \
	$(call autoconf_bool,CONFIG_FS_STABLE_WITH_SRTP,srtp) \
	$(call autoconf_bool,CONFIG_FS_STABLE_WITH_ZRTP,zrtp) \
	$(if $(CONFIG_FS_STABLE_WITH_DEBUG),,--disable-debug) \
	$(if $(CONFIG_FS_STABLE_WITH_FREETYPE),,--without-freetype) \
	$(if $(CONFIG_FS_STABLE_WITH_LIBYUV),,--disable-libyuv) \
	$(if $(CONFIG_FS_STABLE_WITH_ODBC),--with-odbc-lib="$(STAGING_DIR)$(FS_STABLE_LIB_DIR)") \
	$(if $(CONFIG_FS_STABLE_WITH_ODBC),--with-odbc="$(STAGING_DIR)/usr") \
	$(if $(CONFIG_FS_STABLE_WITH_PGSQL),--enable-core-pgsql-support,--without-pgsql) \
	$(if $(CONFIG_FS_STABLE_WITH_PNG),,--without-png) \
	$(if $(CONFIG_FS_STABLE_WITH_VPX),,--disable-libvpx)

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-python-esl)$(CONFIG_PACKAGE_$(PKG_NAME)-mod-python),)
CONFIGURE_ARGS+= \
	--with-python=$(STAGING_DIR_HOSTPKG)/bin/python$(PYTHON_VERSION)
else
CONFIGURE_ARGS+= \
	--with-python=no
endif

ifeq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-erlang_event)$(CONFIG_PACKAGE_$(PKG_NAME)-mod-kazoo),)
CONFIGURE_ARGS+= \
	--with-erlang=no
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-v8),)
CONFIGURE_ARGS+= \
	--enable-static-v8 \
	$(if $(CONFIG_FS_STABLE_WITH_V8_SNAPSHOT),,--disable-snapshot-v8)
endif

# Make mod_spandsp use fixed point math when soft float support is
# enabled on target devices.
ifeq ($(CONFIG_SOFT_FLOAT),y)
CONFIGURE_ARGS+= \
	--enable-fixed-point
endif

# Don't want host-php
CONFIGURE_VARS+= \
	ac_cv_have_php=no \
	ac_cv_have_php_config=no \
	ac_cv_prog_PHP=false \
	ac_cv_prog_PHP_CONFIG=false

# The autoconf variables in this block are OK for both musl and glibc
CONFIGURE_VARS+= \
	ac_cv_dev_urandom=yes \
	ac_cv_file__dev_ptmx=yes \
	ac_cv_file__dev_urandom=yes \
	ac_cv_file_dbd_apr_dbd_mysql_c=no \
	ac_cv_free_null=yes \
	ac_cv_func_mmap_fixed_mapped=yes \
	ac_cv_func_pthread_rwlock_init=yes \
	ac_cv_func_sem_open=yes \
	ac_cv_have_working_memmove=yes \
	ac_cv_negative_eai=yes \
	ac_cv_o_nonblock_inherited=no \
	ac_cv_struct_rlimit=yes \
	apr_cv_epoll=yes \
	apr_cv_gai_addrconfig=yes \
	apr_cv_mutex_recursive=yes \
	apr_cv_process_shared_works=yes \
	apr_cv_pthreads_lib=-lpthread \
	apr_cv_tcp_nodelay_with_cork=yes \
	apr_cv_type_rwlock_t=yes

# fs_cli
CONFIGURE_VARS+= \
	disable_cc=yes

# Regarding apr_cv_mutex_robust_shared=no see
# http://www.openwall.com/lists/musl/2016/11/26/1
# _Don't_ remove quotes below!
ifeq ($(CONFIG_LIBC),"musl")
CONFIGURE_VARS+= \
	apr_cv_mutex_robust_shared=no \
	ac_cv_strerror_r_rc_int=yes
else
CONFIGURE_VARS+= \
	apr_cv_mutex_robust_shared=yes
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-perl-esl)$(CONFIG_PACKAGE_$(PKG_NAME)-mod-perl),)
CONFIGURE_VARS+= \
	PERL="$(PERL_CMD)" \
	PERL_CFLAGS="-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE -I$(STAGING_DIR)$(PERL_SITELIB)/CORE" \
	PERL_INC="-I$(STAGING_DIR)$(PERL_SITELIB)/CORE" \
	PERL_LDFLAGS="$(FPIC) -Wl,-rpath,$(PERL_SITELIB)/CORE -L$(STAGING_DIR)$(PERL_SITELIB)/CORE -lperl" \
	PERL_LIBDIR="-L$(PERL_SITELIB)/CORE" \
	PERL_LIBS="-lpthread $(FS_STABLE_PERL_LIBS) $(EXTRA_LIBDIRS:%=-L%) $(EXTRA_LIBS:%=-l%)" \
	PERL_SITEDIR="$(PERL_SITELIB)"
else
CONFIGURE_VARS+= \
	ac_cv_have_perl=no \
	ac_cv_prog_PERL=false
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-python-esl)$(CONFIG_PACKAGE_$(PKG_NAME)-mod-python),)
CONFIGURE_VARS+= \
	PYTHON_CFLAGS="-I$(STAGING_DIR)/usr/include/python$(PYTHON_VERSION)" \
	PYTHON_LDFLAGS="-lpython$(PYTHON_VERSION) -ldl -lpthread -lm -Xlinker -export-dynamic" \
	PYTHON_LIB="python$(PYTHON_VERSION)" \
	PYTHON_LIBDIR="$(FS_STABLE_LIB_DIR)" \
	PYTHON_SITE_DIR="$(FS_STABLE_PYTHON_SITE_DIR)"
endif

# mod_radius_cdr runs configure in libs/freeradius-client. Let
# freeradius-client know /dev/urandom is available on target devices.
MAKE_VARS+= \
	ac_cv_dev_urandom=yes

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-v8),)

# arm
ifeq ($(CONFIG_arm),y)
  FS_STABLE_MYARCH:=arm
  FS_STABLE_MYDEFINES:=v8_target_arch=arm
  ifeq ($(CONFIG_arm_v6),y)
    FS_STABLE_MYDEFINES+=arm_version=6
  else
    ifeq ($(CONFIG_arm_v7),y)
      FS_STABLE_MYDEFINES+=arm_version=7
    else
      FS_STABLE_MYDEFINES+=arm_version=default
    endif
  endif
  ifeq ($(CONFIG_VFP),y)
    ifeq ($(CONFIG_VFPv3),y)
      ifeq ($(CONFIG_NEON),y)
        FS_STABLE_MYDEFINES+=arm_fpu=neon
      else
        FS_STABLE_MYDEFINES+=arm_fpu=vfpv3
      endif
    else
      FS_STABLE_MYDEFINES+=arm_fpu=vfp
    endif
  else
    FS_STABLE_MYDEFINES+=arm_fpu=default
  endif
  ifeq ($(CONFIG_SOFT_FLOAT),y)
    FS_STABLE_MYDEFINES+=arm_float_abi=softfp
  else
    FS_STABLE_MYDEFINES+=arm_float_abi=hard
  endif
endif

# i386
ifeq ($(CONFIG_i386),y)
  FS_STABLE_MYARCH:=ia32
  FS_STABLE_MYDEFINES:=v8_target_arch=ia32
endif

# mipsel
ifeq ($(CONFIG_mipsel),y)
  FS_STABLE_MYARCH:=mipsel
  FS_STABLE_MYDEFINES:=v8_target_arch=mipsel
  ifeq ($(CONFIG_CPU_MIPS32),y)
    ifeq ($(CONFIG_CPU_MIPS32_R2),y)
      FS_STABLE_MYDEFINES+=mips_arch_variant=mips32r2
    else
      ifeq ($(CONFIG_CPU_MIPS32_R1),y)
        FS_STABLE_MYDEFINES+=mips_arch_variant=mips32r1
      endif
    endif
  endif
  ifeq ($(CONFIG_SOFT_FLOAT),y)
    FS_STABLE_MYDEFINES+=v8_use_mips_abi_hardfloat=false
  else
    FS_STABLE_MYDEFINES+=v8_use_mips_abi_hardfloat=true
  endif
endif

# x86_64
ifeq ($(CONFIG_x86_64),y)
  FS_STABLE_MYARCH:=x64
  FS_STABLE_MYDEFINES:=v8_target_arch=x64
endif

MAKE_VARS+= \
	FS_STABLE_HOSTCC="$(HOSTCC)" \
	FS_STABLE_HOSTCXX="$(HOSTCXX)" \
	FS_STABLE_HOST_CFLAGS="$(HOST_CFLAGS)" \
	FS_STABLE_HOST_LDFLAGS="$(HOST_LDFLAGS)" \
	FS_STABLE_HOST_PYTHONPATH="$(HOST_PYTHONPATH)" \
	FS_STABLE_MYARCH="$(FS_STABLE_MYARCH)" \
	FS_STABLE_MYDEFINES="$(FS_STABLE_MYDEFINES)"

endif

# Make sphinxbase use fixed point math when soft float support is
# enabled on target devices.
ifeq ($(CONFIG_SOFT_FLOAT),y)
MAKE_VARS+= \
	FS_STABLE_USE_FIXED_POINT="--enable-fixed"
endif

# Some common URLs
FS_STABLE_LIBS_URL:=https://files.$(PRG_NAME).org/downloads/libs
FS_STABLE_SPHINX_URL:=@SF/cmusphinx

# mod_event_zmq
FS_STABLE_ZEROMQ_FILE:=zeromq-2.1.9.tar.gz
FS_STABLE_ZEROMQ_HASH:=f3542f756687e622beef3a75c8e027fe2d95d4654350cbca4c070ffc58d9ace0
FS_STABLE_ZEROMQ_URL:=http://download.zeromq.org

# mod_pocketsphinx
FS_STABLE_POCKETSPHINX_FILE:=pocketsphinx-0.8.tar.gz
FS_STABLE_POCKETSPHINX_HASH:=874c4c083d91c8ff26a2aec250b689e537912ff728923c141c4dac48662cce7a

FS_STABLE_SPHINXBASE_FILE:=sphinxbase-0.8.tar.gz
FS_STABLE_SPHINXBASE_HASH:=55708944872bab1015b8ae07b379bf463764f469163a8fd114cbb16c5e486ca8

FS_STABLE_SPHINXMODEL_FILE:=communicator_semi_6000_20080321.tar.gz
FS_STABLE_SPHINXMODEL_HASH:=dbb5e9fb85000a7cb97d6958a3ef8d77532dc55fc730ac6979705e8645cb0c18

# mod_radius_cdr
FS_STABLE_FREERADIUS_CLIENT_FILE:=freeradius-client-1.1.6.tar.gz
FS_STABLE_FREERADIUS_CLIENT_HASH:=3fc609af328258e00345389d5478b099fe4ea3ad694d0472525ef3adab9cf053

# mod_v8
FS_STABLE_V8_FILE:=v8-3.24.14.tar.bz2
FS_STABLE_V8_HASH:=395f4eaf5580b973b1e33fe0aa27f8d013ddf1b163ad76992c50dd91ff182828

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-event_zmq),)
$(eval $(call Download/files,zmq,$(FS_STABLE_ZEROMQ_FILE),$(FS_STABLE_ZEROMQ_URL),$(FS_STABLE_ZEROMQ_HASH)))
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-pocketsphinx)$(CONFIG_PACKAGE_$(PKG_NAME)-misc-grammar),)
$(eval $(call Download/files,pocketsphinx,$(FS_STABLE_POCKETSPHINX_FILE),$(FS_STABLE_SPHINX_URL),$(FS_STABLE_POCKETSPHINX_HASH)))
$(eval $(call Download/files,sphinxbase,$(FS_STABLE_SPHINXBASE_FILE),$(FS_STABLE_SPHINX_URL),$(FS_STABLE_SPHINXBASE_HASH)))
$(eval $(call Download/files,communicator,$(FS_STABLE_SPHINXMODEL_FILE),$(FS_STABLE_LIBS_URL),$(FS_STABLE_SPHINXMODEL_HASH)))
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-radius_cdr),)
$(eval $(call Download/files,freeradius-client,$(FS_STABLE_FREERADIUS_CLIENT_FILE),$(FS_STABLE_LIBS_URL),$(FS_STABLE_FREERADIUS_CLIENT_HASH)))
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-v8),)
$(eval $(call Download/files,v8,$(FS_STABLE_V8_FILE),$(FS_STABLE_LIBS_URL),$(FS_STABLE_V8_HASH)))
endif

# Need to update LDFLAGS for libs/unimrcp, otherwise it will try to link to a
# different apr/apr-util if found.
# FS_STABLE_ANCHOR: string in build/acmacros/apr.m4 that will be replaced
FS_STABLE_ANCHOR:=dnl Get build information from APR
FS_STABLE_APR_LIBS:=-L$(PKG_BUILD_DIR)/libs/apr -L$(PKG_BUILD_DIR)/libs/apr-util

define Build/Prepare
	$(call Build/Prepare/Default)

	echo '#applications/mod_random' >> $(PKG_BUILD_DIR)/modules.conf
	echo '#codecs/mod_yuv' >> $(PKG_BUILD_DIR)/modules.conf
	echo '#event_handlers/mod_event_test' >> $(PKG_BUILD_DIR)/modules.conf
	$(SED) '/^#/!s/^/#/' $(PKG_BUILD_DIR)/modules.conf
	$(foreach m,$(FS_STABLE_MOD_AVAILABLE),
		$(if $(CONFIG_PACKAGE_$(PKG_NAME)-mod-$(m)),
		$(SED) '/mod_$(m)$$$$/s/^#//' $(PKG_BUILD_DIR)/modules.conf))

	$(SED) 's|$(FS_STABLE_ANCHOR)|APR_SETVAR(LDFLAGS,$(FS_STABLE_APR_LIBS) $(TARGET_LDFLAGS))|' \
		$(PKG_BUILD_DIR)/libs/unimrcp/build/acmacros/apr.m4

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-event_zmq),)
	$(CP) $(DL_DIR)/$(FS_STABLE_ZEROMQ_FILE) $(PKG_BUILD_DIR)/libs
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-pocketsphinx)$(CONFIG_PACKAGE_$(PKG_NAME)-misc-grammar),)
	$(CP) $(DL_DIR)/$(FS_STABLE_POCKETSPHINX_FILE) $(PKG_BUILD_DIR)/libs
	$(CP) $(DL_DIR)/$(FS_STABLE_SPHINXBASE_FILE) $(PKG_BUILD_DIR)/libs
	$(CP) $(DL_DIR)/$(FS_STABLE_SPHINXMODEL_FILE) $(PKG_BUILD_DIR)/libs
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-radius_cdr),)
	$(CP) $(DL_DIR)/$(FS_STABLE_FREERADIUS_CLIENT_FILE) $(PKG_BUILD_DIR)/libs
endif

ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-v8),)
	$(CP) $(DL_DIR)/$(FS_STABLE_V8_FILE) $(PKG_BUILD_DIR)/libs
endif

# Hack for misc-grammar - needs mod_pocketsphinx to provide grammar files
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-grammar),)
ifeq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-pocketsphinx),)
	$(SED) '/mod_pocketsphinx$$$$/s/^#//' $(PKG_BUILD_DIR)/modules.conf
endif
endif

# Hack for mod_unimrcp - it has a build-time dep on mod_sofia
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-unimrcp),)
ifeq ($(CONFIG_PACKAGE_$(PKG_NAME)-mod-sofia),)
	$(SED) '/mod_sofia$$$$/s/^#//' $(PKG_BUILD_DIR)/modules.conf
endif
endif
endef

define Build/Configure
	$(FIND) $(PKG_BUILD_DIR) -name missing -type f -exec rm {} \;
	cd $(PKG_BUILD_DIR); $(AM_TOOL_PATHS) ./rebootstrap.sh
	$(call Build/Configure/Default)
endef

define Build/Compile
	$(call Build/Compile/Default)
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-perl-esl),)
	$(call Build/Compile/Default,-C $(PKG_BUILD_DIR)/libs/esl perlmod)
endif
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-python-esl),)
	$(call Build/Compile/Default,-C $(PKG_BUILD_DIR)/libs/esl pymod)
endif
endef

define Build/Install
	$(call Build/Install/Default)
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-perl-esl),)
	$(call Build/Compile/Default,-C $(PKG_BUILD_DIR)/libs/esl DESTDIR=$(PKG_INSTALL_DIR) perlmod-install)
endif
ifneq ($(CONFIG_PACKAGE_$(PKG_NAME)-misc-python-esl),)
	$(call Build/Install/Default,-C $(PKG_BUILD_DIR)/libs/esl pymod-install)
endif
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)$(FS_STABLE_INCLUDES_DIR)
	$(INSTALL_DIR) $(1)$(FS_STABLE_PKGCONFIG_DIR)
	$(INSTALL_DATA) $(PKG_INSTALL_DIR)$(FS_STABLE_INCLUDES_DIR)/*.h \
						$(1)$(FS_STABLE_INCLUDES_DIR)
	$(INSTALL_DATA) \
		$(PKG_INSTALL_DIR)$(FS_STABLE_PKGCONFIG_DIR)/$(PRG_NAME).pc \
						$(1)$(FS_STABLE_PKGCONFIG_DIR)
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
$(eval $(call BuildPackage,$(PKG_NAME)-misc-hotplug))
$(eval $(call BuildPackage,$(PKG_NAME)-misc-perl-esl))
$(eval $(call BuildPackage,$(PKG_NAME)-misc-python-esl))
$(eval $(call BuildPackage,$(PKG_NAME)-misc-timezones))

################################
# FreeSWITCH example configs
# Params:
# 1 - Package subname
################################

$(eval $(call Package/$(PKG_NAME)/Example,curl))
$(eval $(call Package/$(PKG_NAME)/Example,insideout))
$(eval $(call Package/$(PKG_NAME)/Example,minimal))
$(eval $(call Package/$(PKG_NAME)/Example,rayo))
$(eval $(call Package/$(PKG_NAME)/Example,sbc))
$(eval $(call Package/$(PKG_NAME)/Example,softphone))
$(eval $(call Package/$(PKG_NAME)/Example,testing))
$(eval $(call Package/$(PKG_NAME)/Example,vanilla))

################################
# FreeSWITCH language files
# Params:
# 1 - Language code
# 2 - Language
################################

$(eval $(call Package/$(PKG_NAME)/Language,de,German))
$(eval $(call Package/$(PKG_NAME)/Language,en,English))
$(eval $(call Package/$(PKG_NAME)/Language,es,Spanish))
$(eval $(call Package/$(PKG_NAME)/Language,fr,French))
$(eval $(call Package/$(PKG_NAME)/Language,he,Hebrew))
$(eval $(call Package/$(PKG_NAME)/Language,pt,Portuguese))
$(eval $(call Package/$(PKG_NAME)/Language,ru,Russian))
$(eval $(call Package/$(PKG_NAME)/Language,sv,Swedish))

################################
# FreeSWITCH misc packages
# Params:
# 1 - Package subname
# 2 - Package title
# 3 - Package description
# 4 - Source dir relative to
#     PKG_INSTALL_DIR
# 5 - Dest dir relative to ipkg
# 6 - Arch independent files
################################

$(eval $(call Package/$(PKG_NAME)/Misc,misc-fonts,Fonts,This package includes the fonts bundled with FreeSWITCH.,$(FS_STABLE_FONTS_DIR),$(FS_STABLE_FONTS_DIR),y))
$(eval $(call Package/$(PKG_NAME)/Misc,misc-grammar,Grammar,This package contains grammar files. mod_pocketsphinx would be a\npotential user.,$(FS_STABLE_GRAMMAR_DIR),$(FS_STABLE_GRAMMAR_DIR),y))
$(eval $(call Package/$(PKG_NAME)/Misc,misc-images,Images,This package includes the images bundled with FreeSWITCH.,$(FS_STABLE_IMAGES_DIR),$(FS_STABLE_IMAGES_DIR),y))

################################
# FreeSWITCH modules
# Params:
# 1 - Package subname
# 2 - Package title
# 3 - Module description
# 4 - Module dependencies
################################

$(eval $(call Package/$(PKG_NAME)/Module,abstraction,API abstraction,This module provides a way to create new API functions via regex\nrewriting.,))
$(eval $(call Package/$(PKG_NAME)/Module,alsa,ALSA endpoint,ALSA endpoint module.,+alsa-lib))
$(eval $(call Package/$(PKG_NAME)/Module,amr,AMR passthrough,Passthrough AMR codec support.,))
$(eval $(call Package/$(PKG_NAME)/Module,amrwb,AMR wideband passthrough,Passthrough AMR wideband codec support.,))
$(eval $(call Package/$(PKG_NAME)/Module,av,AV,Video codec and format support via FFmpeg.,+libffmpeg-full @i386||x86_64))
$(eval $(call Package/$(PKG_NAME)/Module,avmd,Voicemail detection,This module attempts to determine when a voicemail system has answered\nthe call.,))
$(eval $(call Package/$(PKG_NAME)/Module,b64,Base64,Transfers data Base64 encoded.,))
$(eval $(call Package/$(PKG_NAME)/Module,basic,BASIC,BASIC module for FreeSWITCH.,))
$(eval $(call Package/$(PKG_NAME)/Module,bert,BERT,Line testing tool.,))
$(eval $(call Package/$(PKG_NAME)/Module,blacklist,Blacklist helper,This module provides tools to blacklist callers.,))
$(eval $(call Package/$(PKG_NAME)/Module,callcenter,Call center,This module implements Automated Call Distribution queues.,))
$(eval $(call Package/$(PKG_NAME)/Module,cdr_csv,CSV CDR,CSV Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,cdr_mongodb,MongoDB CDR,MongoDB Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,cdr_pg_csv,PostgreSQL CDR,PostgreSQL Call Detail Record handler.,@FS_STABLE_WITH_PGSQL))
$(eval $(call Package/$(PKG_NAME)/Module,cdr_sqlite,SQLite CDR,SQLite Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,cidlookup,Caller ID lookup,This module provides an API for querying caller ID name and location\ndata.,))
$(eval $(call Package/$(PKG_NAME)/Module,clearmode,Clearmode,Clearmode codec passthrough support.,))
$(eval $(call Package/$(PKG_NAME)/Module,cluechoo,Clue Choo,This demo module renders a Clue Choo train on the FreeSWITCH console.,+libncurses))
$(eval $(call Package/$(PKG_NAME)/Module,commands,Commands,This module provides miscellaneous API commands.,))
$(eval $(call Package/$(PKG_NAME)/Module,conference,Conference,This module provides multi-party conferencing.,))
$(eval $(call Package/$(PKG_NAME)/Module,console,Console logger,Allows control over what messages get logged to the console. When\nFreeSWITCH is compiled without libedit support - this is not the\ndefault - mod_console has no effect because the init script starts\nFreeSWITCH with the console disabled to work around an\ninteroperability issue that causes high CPU load.,))
$(eval $(call Package/$(PKG_NAME)/Module,curl,cURL,This module provides an API for making HTTP requests with cURL.,))
$(eval $(call Package/$(PKG_NAME)/Module,cv,OpenCV,This module exposes opencv actions to enable computer vision actions.,+opencv @BROKEN)) # opencv package too stripped-down
$(eval $(call Package/$(PKG_NAME)/Module,dahdi_codec,DAHDI codec,DAHDI codec module.,))
$(eval $(call Package/$(PKG_NAME)/Module,db,DB,This module implements a simple db API with group support. Also can be\nused as a limit db backend.,))
$(eval $(call Package/$(PKG_NAME)/Module,dialplan_asterisk,Asterisk dialplan,Asterisk extensions.conf style dialplan parser.,))
$(eval $(call Package/$(PKG_NAME)/Module,dialplan_directory,Directory dialplan,Directory dialplan support.,))
$(eval $(call Package/$(PKG_NAME)/Module,dialplan_xml,XML dialplan,Standard FreeSWITCH XML dialplan support.,))
$(eval $(call Package/$(PKG_NAME)/Module,dingaling,Generic XMPP,Allows FreeSWITCH to be used as a client for XMPP Servers.,))
$(eval $(call Package/$(PKG_NAME)/Module,directory,Dial-by-name directory,This module implements a dial-by-name directory IVR.,))
$(eval $(call Package/$(PKG_NAME)/Module,distributor,Load distributor,This module implements a mechanism for performing load balancing.,))
$(eval $(call Package/$(PKG_NAME)/Module,dptools,Dialplan tools,This module implements basic dialplan tools.,))
$(eval $(call Package/$(PKG_NAME)/Module,easyroute,DID routing,This module does destination lookup based on DID.,))
$(eval $(call Package/$(PKG_NAME)/Module,enum,ENUM routing,This module implements ENUM support.,+libldns))
$(eval $(call Package/$(PKG_NAME)/Module,erlang_event,Erlang event,Erlang event module.,+erlang))
$(eval $(call Package/$(PKG_NAME)/Module,esf,Multicast,This module adds multi-cast support.,))
$(eval $(call Package/$(PKG_NAME)/Module,esl,Single ESL,This module adds an API for generating one-off ESL requests.,))
$(eval $(call Package/$(PKG_NAME)/Module,event_multicast,Multicast Event,Multicast Event System for FreeSWITCH.,))
$(eval $(call Package/$(PKG_NAME)/Module,event_socket,Event socket,Sends events via a single socket. Needed for fs_cli.,))
$(eval $(call Package/$(PKG_NAME)/Module,event_test,Event test,Event demo module.,))
$(eval $(call Package/$(PKG_NAME)/Module,event_zmq,ZMQ event,ZMQ event module.,))
$(eval $(call Package/$(PKG_NAME)/Module,expr,Expr,This module adds expr support for expression evaluation.,))
$(eval $(call Package/$(PKG_NAME)/Module,fifo,FIFO,This module adds a first-in first-out queue system.,))
$(eval $(call Package/$(PKG_NAME)/Module,format_cdr,Multiformat CDR,A superset of mod_json_cdr and mod_xml_cdr.,))
$(eval $(call Package/$(PKG_NAME)/Module,fsk,FSK,This module adds frequency-shift keying support which can be used to\nsend and receive caller ID.,))
$(eval $(call Package/$(PKG_NAME)/Module,fsv,FSV,This module provides dialplan applications for recording and playing\nvideos.,))
$(eval $(call Package/$(PKG_NAME)/Module,g723_1,G.723.1 passthrough,G.723.1 codec passthrough.,))
$(eval $(call Package/$(PKG_NAME)/Module,g729,G.729 passthrough,G.729 codec passthrough.,))
$(eval $(call Package/$(PKG_NAME)/Module,graylog2,Graylog2 logger,GELF logger for Graylog2 and Logstash.,))
$(eval $(call Package/$(PKG_NAME)/Module,h26x,H.26x passthrough,H.26x video codec passthrough.,))
$(eval $(call Package/$(PKG_NAME)/Module,hash,Hash,This module provides a key-value in-memory datastore. Usable as a\nlimit backend.,))
$(eval $(call Package/$(PKG_NAME)/Module,hiredis,Redis client,This module provides a mechanism to use Redis as a datastore.,+libhiredis))
$(eval $(call Package/$(PKG_NAME)/Module,httapi,HT-TAPI,This module provides an API for controlling the switch by responding\nto HTTP requests.,))
$(eval $(call Package/$(PKG_NAME)/Module,http_cache,HTTP GET with caching,This module provides an API for making HTTP GET requests where the\nresult is cached.,))
$(eval $(call Package/$(PKG_NAME)/Module,isac,iSAC,iSAC codec support.,@arm||i386||mips||mips64||mips64el||mipsel||x86_64))
$(eval $(call Package/$(PKG_NAME)/Module,json_cdr,JSON CDR,JSON-based Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,kazoo,Kazoo,Kazoo module for FreeSWITCH.,+erlang))
$(eval $(call Package/$(PKG_NAME)/Module,lcr,LCR,This module adds a facility for least-cost routing.,))
$(eval $(call Package/$(PKG_NAME)/Module,ldap,LDAP,LDAP module for FreeSWITCH.,+libopenldap))
$(eval $(call Package/$(PKG_NAME)/Module,local_stream,Local stream,Connects multiple channels to a looped stream.,))
$(eval $(call Package/$(PKG_NAME)/Module,logfile,File logger,Logs FreeSWITCH output to a file.,))
$(eval $(call Package/$(PKG_NAME)/Module,loopback,Loopback,A loopback channel driver.,))
$(eval $(call Package/$(PKG_NAME)/Module,lua,Lua,Lua language interface for FreeSWITCH.,+liblua))
$(eval $(call Package/$(PKG_NAME)/Module,mp4v,MP4 video passthrough,MP4 video passthrough..,))
$(eval $(call Package/$(PKG_NAME)/Module,native_file,Native file,Plays files that are already encoded in the right format.,))
$(eval $(call Package/$(PKG_NAME)/Module,nibblebill,Nibblebill,This module allows for real-time accounting of a cash balance and\nusing that information for call routing.,))
$(eval $(call Package/$(PKG_NAME)/Module,odbc_cdr,ODBC CDR,ODBC Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,opus,Opus,Opus codec support.,+libopus))
$(eval $(call Package/$(PKG_NAME)/Module,oreka,Oreka,This module provides media recording with the Oreka cross-platform\naudio stream recording and retrieval system.,))
$(eval $(call Package/$(PKG_NAME)/Module,perl,Perl,This package contains mod_perl for FreeSWITCH.,+libdb47 +libgdbm +perlbase-essential @PERL_THREADS))
$(eval $(call Package/$(PKG_NAME)/Module,png,PNG,Allows playback of video using PNG files.,@FS_STABLE_WITH_PNG))
$(eval $(call Package/$(PKG_NAME)/Module,pocketsphinx,Pocketsphinx,This module allows speech recognition. You might want to install\n$(PKG_NAME)-misc-grammar as well.,+libsamplerate)) # When libsamplerate is found it'll be linked against, there is no switch to turn it off
$(eval $(call Package/$(PKG_NAME)/Module,portaudio,Portaudio,Voice through a local soundcard.,+portaudio))
$(eval $(call Package/$(PKG_NAME)/Module,portaudio_stream,Portaudio streaming,Stream from an external audio source for Music on Hold.,+portaudio))
$(eval $(call Package/$(PKG_NAME)/Module,posix_timer,POSIX timer,Add POSIX timer support.,))
$(eval $(call Package/$(PKG_NAME)/Module,prefix,Prefix match,This module provides a data store with fast lookups by the longest\nprefix match rule.,))
$(eval $(call Package/$(PKG_NAME)/Module,python,Python,Python support module.,+python-light))
$(eval $(call Package/$(PKG_NAME)/Module,radius_cdr,Radius CDR,Radius Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,random,Entropy,This module extracts entropy from FreeSWITCH and feeds it into\n/dev/random.,))
$(eval $(call Package/$(PKG_NAME)/Module,rayo,Rayo,Rayo/XMPP 3PCC server for FreeSWITCH.,+$(PKG_NAME)-mod-ssml))
$(eval $(call Package/$(PKG_NAME)/Module,redis,Redis limit backend,This module provides a mechanism to use Redis as a limit backend data\nstore.,))
$(eval $(call Package/$(PKG_NAME)/Module,rss,RSS,Parses and reads XML based RSS feeds and reads the entries aloud via a TTS engine.,))
$(eval $(call Package/$(PKG_NAME)/Module,rtc,Media streaming,Media streaming as used by WebRTC and mod_verto.,))
$(eval $(call Package/$(PKG_NAME)/Module,rtmp,RTMP endpoint,RTMP endpoint support. Allows FreeSWITCH to be used from RTMP clients.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_de,German Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_en,English Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_es,Spanish Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_es_ar,Argentinian Spanish Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_fa,Persian Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_fr,French Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_he,Hebrew Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_hr,Croatian Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_hu,Hungarian Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_it,Italian Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_ja,Japanese Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_nl,Dutch Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_pl,Polish Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_pt,Portuguese Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_ru,Russian Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_sv,Swedish Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_th,Thai Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,say_zh,Chinese Say,Uses prerecorded sounds to read or say various things.,))
$(eval $(call Package/$(PKG_NAME)/Module,shell_stream,Shell stream,Allows to stream audio from an arbitrary shell command.,))
$(eval $(call Package/$(PKG_NAME)/Module,shout,Shout,Allows to stream audio from MP3s or Shoutcast streams.,+lame-lib +libmpg123 +libshout))
$(eval $(call Package/$(PKG_NAME)/Module,skinny,Skinny,Skinny Call Control Protocol endpoint support.,))
$(eval $(call Package/$(PKG_NAME)/Module,sms,SMS,This module provides an abstract facility for interfacing with SMS\nsystems.,))
$(eval $(call Package/$(PKG_NAME)/Module,snapshot,Snapshot,This module can record a sliding window of audio and take snapshots\nto disk.,))
$(eval $(call Package/$(PKG_NAME)/Module,sndfile,Soundfile,Adds sound format support via libsndfile.,+libsndfile))
$(eval $(call Package/$(PKG_NAME)/Module,snmp,SNMP,An SNMP stats reporter.,+libnetsnmp))
$(eval $(call Package/$(PKG_NAME)/Module,snom,SNOM,This module implements features specific to SNOM phones.,))
$(eval $(call Package/$(PKG_NAME)/Module,sofia,Sofia SIP,SIP module.,))
$(eval $(call Package/$(PKG_NAME)/Module,sonar,Sonar,This module measures the latency on an audio link by sending audible\naudio sonar pings.,))
$(eval $(call Package/$(PKG_NAME)/Module,spandsp,SpanDSP,This module implements SpanDSP fax. It includes DSP and codec\nfunctionality.,+libjpeg +liblzma))
$(eval $(call Package/$(PKG_NAME)/Module,spy,User Spy,This module adds the ability to monitor the audio of a channel.,))
$(eval $(call Package/$(PKG_NAME)/Module,ssml,SSML,mod_ssml is a FreeSWITCH module that renders SSML into audio. This\nmodule requires a text-to-speech module for speech synthesis.,))
$(eval $(call Package/$(PKG_NAME)/Module,stress,Stress,This module attempts to detect voice stress on an audio channel.,))
$(eval $(call Package/$(PKG_NAME)/Module,syslog,Syslog logger,Logs FreeSWITCH output to the syslog.,))
$(eval $(call Package/$(PKG_NAME)/Module,theora,Theora passthrough,Theora video codec passthrough.,))
$(eval $(call Package/$(PKG_NAME)/Module,tone_stream,Tone stream,Tone generation stream.,))
$(eval $(call Package/$(PKG_NAME)/Module,translate,Number translation,This module implements number translation.,))
$(eval $(call Package/$(PKG_NAME)/Module,tts_commandline,TTS command-line,Run a command-line and play the output file.,))
$(eval $(call Package/$(PKG_NAME)/Module,unimrcp,UniMRCP,Allows communication with Media Resource Control Protocol servers.,))
$(eval $(call Package/$(PKG_NAME)/Module,v8,V8,This package contains mod_v8 for FreeSWITCH.,@FS_STABLE_WITH_V8 @arm||i386||mipsel||x86_64))
$(eval $(call Package/$(PKG_NAME)/Module,valet_parking,Valet parking,This module implements the valet call parking strategy.,))
$(eval $(call Package/$(PKG_NAME)/Module,verto,Verto,Verto signaling protocol.,))
$(eval $(call Package/$(PKG_NAME)/Module,vmd,Voicemail detection,This module detects voicemail beeps.,))
$(eval $(call Package/$(PKG_NAME)/Module,voicemail,Voicemail,This module provides a voicemail system.,))
$(eval $(call Package/$(PKG_NAME)/Module,voicemail_ivr,Voicemail IVR,This module provides an extensible voicemail IVR system.,))
$(eval $(call Package/$(PKG_NAME)/Module,xml_cdr,XML CDR,XML Call Detail Record handler.,))
$(eval $(call Package/$(PKG_NAME)/Module,xml_curl,XML cURL,Provides an XML cURL interfaces to pull dynamic XML configuration for\nFreeSWITCH over HTTP.,))
$(eval $(call Package/$(PKG_NAME)/Module,xml_ldap,XML LDAP,LDAP XML gateway.,+libopenldap))
$(eval $(call Package/$(PKG_NAME)/Module,xml_rpc,XML RPC,Allows using the webapi to control FreeSWITCH.,))
$(eval $(call Package/$(PKG_NAME)/Module,xml_scgi,XML SCGI,SCGI XML Gateway.,))
$(eval $(call Package/$(PKG_NAME)/Module,yaml,YAML,YAML language module.,+libyaml))
$(eval $(call Package/$(PKG_NAME)/Module,yuv,Raw YUV,Raw YUV I420 video codec support.,@FS_STABLE_WITH_LIBYUV))

################################
# FreeSWITCH utilities
# Params:
# 1 - Package subname
# 2 - Package title
# 3 - Utility description
# 4 - Utility dependencies
# 5 - Utility is a script (y/n)
################################

$(eval $(call Package/$(PKG_NAME)/Util,fs_cli,CLI,The fs_cli program is a Command-Line Interface that allows a user to\nconnect to a FreeSWITCH instance running on the local or a remote\nsystem.,,n))
$(eval $(call Package/$(PKG_NAME)/Util,fs_encode,Sound file conversion,Format conversion of sound files so the result can be used by\nmod_native_file.,+$(PKG_NAME)-mod-native_file +$(PKG_NAME)-mod-sndfile +$(PKG_NAME)-mod-spandsp,n))
$(eval $(call Package/$(PKG_NAME)/Util,fs_ivrd,IVR daemon,The FreeSWITCH IVR daemon is an abstraction layer that sits on top of\nthe ESL. The basic idea is that the ivrd will allow the user to have\na STDIN/STDOUT interface for simple call control.,,n))
$(eval $(call Package/$(PKG_NAME)/Util,gentls_cert,TLS certificate,Can be used to create TLS certificates and setup CAs.,+openssl-util,y))
$(eval $(call Package/$(PKG_NAME)/Util,tone2wav,Sound file generation,Generates a sound file from a teletone script. The output can be in\nany format that is supported by libsndfile.,+$(PKG_NAME)-mod-sndfile,n))

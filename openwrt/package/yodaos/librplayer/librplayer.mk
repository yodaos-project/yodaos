################################################################################
#
# librplayer -- rokid librplayer for media play.
#
################################################################################

LIBRPLAYER_SITE = $(call qstrip,$(BR2_EXTERNAL_ROKID_PATH)/../frameworks/native/libs/librplayer)
LIBRPLAYER_SITE_METHOD = local
LIBRPLAYER_INSTALL_STAGING = YES
LIBRPLAYER_DEPENDENCIES = ffmpeg sdl2 pulseaudio

#ifeq ($(BR2_STATIC_LIBS),y)
#LIBRPLAYER_PIC =
#LIBRPLAYER_SHARED = --static
#else
#LIBRPLAYER_PIC = -fPIC
#LIBRPLAYER_SHARED = --shared
#endif

LIBRPLAYER_INC_HEADERS = audio/pulseaudio/audioplayer.h video/sdl2/videoplayer.h \
	ffmpeg_mediaplayer.h ffmpeg_utils.h \
	mediaplayer.h

LIBRPLAYER_INC_HEADERS += $(addprefix include/librplayer/, rplayer_error.h  rplayer_mutex.h  rplayer_type.h)

define LIBRPLAYER_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CROSS_COMPILE="$(TARGET_CROSS)" \
		PKG_CONFIG="$(PKG_CONFIG_HOST_BINARY)" \
		SYSROOT="$(STAGING_DIR)" \
                -C $(@D) all
endef

define LIBRPLAYER_INSTALL_STAGING_CMDS
        $(INSTALL) -D -m 0755 $(@D)/librplayer.so $(STAGING_DIR)/usr/lib/librplayer.so
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/librplayer
	cp -pf $(addprefix $(@D)/, $(LIBRPLAYER_INC_HEADERS)) $(STAGING_DIR)/usr/include/librplayer/
endef

define LIBRPLAYER_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/librplayer.so $(TARGET_DIR)/usr/lib/librplayer.so
endef

$(eval $(generic-package))


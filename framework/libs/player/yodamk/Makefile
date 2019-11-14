
include $(TOPDIR)/rules.mk

PKG_NAME:=librplayer
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/native/libs/librplayer

PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

ifeq ($(CONFIG_PACKAGE_soundtouch),y)
CMAKE_OPTIONS:= -DROKID_LIBS_SOUNDTOUCH=y
endif

ifeq ($(CONFIG_PACKAGE_eq_ctrl),y)
CMAKE_OPTIONS += -DROKID_LIBS_EQ_CTRL=y
endif

ifeq ($(CONFIG_AUDIO_LINK_CUT),y)
else
     CMAKE_OPTIONS += -DNEED_AUDIO_PRE_LINK=1
endif

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/librplayer
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=librplayer
  DEPENDS:=+libstdcpp +SDL2 +libffmpeg-audio-dec +pulseaudio +libopus +rklog +PACKAGE_eq_ctrl:eq_ctrl +PACKAGE_soundtouch:soundtouch
endef

define Package/librplayer/description
	librplayer
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

define Package/librplayer/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,librplayer))

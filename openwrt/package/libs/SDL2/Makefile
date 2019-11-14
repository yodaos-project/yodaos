
include $(TOPDIR)/rules.mk

PKG_NAME:=SDL2
PKG_VERSION:=2.0.5
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/SDL2-$(PKG_VERSION)
PKG_SOURCE:=SDL2-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.libsdl.org/release

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/host-build.mk
include $(INCLUDE_DIR)/package.mk

define Package/SDL2
  SECTION:=libs
  CATEGORY:=Libraries
  DEPENDS:=+zlib
  TITLE:=Simple DirectMedia Layer
  DEPENDS:=+libstdcpp +pulseaudio
ifeq ($(CONFIG_SDL2_DIRECTFB),y)
  DEPENDS:=+directfb
endif
endef

define Package/SDL2/description
  Simple DirectMedia Layer
endef

TARGET_CFLAGS += $(FPIC)

MAKE_FLAGS += \
	PREFIX=/usr

CONFIGURE_ARGS += \
	--disable-rpath \
	--disable-arts \
	--disable-esd \
	--disable-dbus \
	--disable-joystick \
	--disable-haptic  \
	--disable-video-opengl \
	--disable-video-opengles \
	--disable-video-wayland \
	--enable-libudev \
	--enable-video-directfb \
	--enable-pulseaudio

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include
	$(CP) $(PKG_INSTALL_DIR)/usr/include/* $(1)/usr/include/

	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/*.so* $(1)/usr/lib/

	$(INSTALL_DIR) $(1)/usr/lib/pkgconfig
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/pkgconfig/sdl2.pc $(1)/usr/lib/pkgconfig/
endef

define Package/SDL2/config
	source "$(SOURCE)/Config.in"
endef

define Package/SDL2/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/lib*.so* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,SDL2))


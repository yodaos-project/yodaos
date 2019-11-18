include $(TOPDIR)/rules.mk
PKG_NAME:=yoda-pack

include $(INCLUDE_DIR)/host-build.mk
PKG_SOURCE_DIR:=$(TOPDIR)/../framework/libs/ota
PKG_CONFIG_DIR:=$(TOPDIR)/../products/$(CONFIG_PRODUCT_PATHNAME)/ota

define Package/yoda-pack
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A library for accessing Linux USB devices
  DEPENDS:=+libpthread
endef

define Host/Compile
	$(HOSTCC) $(HOST_CFLAGS) \
		$(PKG_SOURCE_DIR)/rokid_package.c \
		$(PKG_SOURCE_DIR)/sha1.c \
			-o $(HOST_BUILD_DIR)/yoda-pack
endef

define Host/Install
	cp $(PKG_CONFIG_DIR)/*.conf $(HOST_BUILD_DIR)/
endef

define Host/Clean
endef
$(eval $(call HostBuild))

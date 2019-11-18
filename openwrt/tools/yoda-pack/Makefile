include $(TOPDIR)/rules.mk
PKG_NAME:=yoda-pack

include $(INCLUDE_DIR)/host-build.mk
PKG_SOURCE_DIR:=$(TOPDIR)/../framework/libs/ota
PKG_CONFIG_DIR:=$(TOPDIR)/../products/$(CONFIG_PRODUCT_PATHNAME)/ota

define Package/yoda-pack
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=A library for accessing Linux USB devices
  DEPENDS:=+libpthread +libusb
endef

define Host/Compile
	$(HOSTCC) $(HOST_CFLAGS) $(PKG_SOURCE_DIR)/rokid_package.c  $(PKG_SOURCE_DIR)/sha1.c -o $(HOST_BUILD_DIR)/rokid_package

	$(HOSTCXX) $(HOST_CFLAGS) \
	$(PKG_SOURCE_DIR)/rokid_unpack.c \
	$(PKG_SOURCE_DIR)/sha1.c  \
	$(PKG_SOURCE_DIR)/bootx/boot.cpp  \
	$(PKG_SOURCE_DIR)/bootx/os.c  \
	$(PKG_SOURCE_DIR)/bootx/resolve_conf.c  \
	$(PKG_SOURCE_DIR)/bootx/str2argv.c  \
	$(PKG_SOURCE_DIR)/bootx/tftpd.c  \
	$(PKG_SOURCE_DIR)/bootx/util.c  \
	$(PKG_SOURCE_DIR)/bootx/bootfile/leo_boot.c  \
	$(PKG_SOURCE_DIR)/bootx/bootfile/leo_mpw_boot.c  \
		-I$(PKG_SOURCE_DIR) \
		-I$(PKG_SOURCE_DIR)/bootx \
		-I/usr/include/libusb-1.0 \
		-lusb-1.0 -lpthread -o $(HOST_BUILD_DIR)/rokid_unpack
endef

define Host/Install
	cp $(PKG_CONFIG_DIR)/*.conf $(HOST_BUILD_DIR)/
endef

define Host/Clean
endef
$(eval $(call HostBuild))

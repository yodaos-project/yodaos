include $(TOPDIR)/rules.mk

PKG_NAME:=openfst
PKG_VERSION:=1.6.5
PKG_RELEASE:=1

PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=http://www.openfst.org/twiki/pub/FST/FstDownload/

PKG_FIXUP:=autoreconf
PKG_INSTALL:=1

include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += $(FPIC)

define Package/openfst
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=openfst
  DEPENDS:=+libstdcpp
  URL:=http://www.openfst.org
endef

define Package/openfst/description
	OpenFst is a library for constructing, combining, optimizing, and searching weighted finite-state transducers (FSTs).
endef

CONFIGURE_ARGS += \
	--enable-shared \
	--disable-static

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/openfst
	$(CP) $(PKG_INSTALL_DIR)/usr/include/fst/* $(1)/usr/include/openfst
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libfst.* $(1)/usr/lib/
	$(INSTALL_DIR) $(1)/usr/bin
	#$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/
endef

define Package/openfst/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) $(PKG_INSTALL_DIR)/usr/lib/libfst.* $(1)/usr/lib/
endef

$(eval $(call BuildPackage,openfst))

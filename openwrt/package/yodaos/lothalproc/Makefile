
include $(TOPDIR)/rules.mk

PKG_NAME:=lothalproc
PKG_VERSION:=
PKG_RELEASE:=1

PKG_SOURCE_DIR:=$(ROKID_DIR)/frameworks/aial/openvoice/lothalproc

APP_EXE=lothalproc
PKG_FIXUP:=libtool
PKG_INSTALL:=1
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/lothalproc
  SECTION:=libs
  CATEGORY:=rokid
  TITLE:=lothalproc
  DEPENDS:=+mutils +flora +ai-libs-common +products
endef

define Package/lothalproc/description
	property
endef

CMAKE_OPTIONS += \
				 -DCUSTOM_CMAKE_MODULES=${PKG_SOURCE_DIR}/../../../allspark/cmake-modules \
				 -DHAVE_FLORA=ON

ifeq ($(CONFIG_AI_KAMINO),y)
    CMAKE_OPTIONS += -DAI_KAMINO=ON
    CMAKE_OPTIONS += -DBUILD_TARGET=k18
    CMAKE_OPTIONS += -DHAVE_MIC_ARRAY=ON
else ifeq ($(CONFIG_AI_A113),y)
    CMAKE_OPTIONS += -DAI_A113=ON
    CMAKE_OPTIONS += -DBUILD_TARGET=a113
    CMAKE_OPTIONS += -DHAVE_MIC_ARRAY=ON
else ifeq ($(CONFIG_AI_ANDROID),y)
    CMAKE_OPTIONS += -DAI_ANDROID=ON
    CMAKE_OPTIONS += -DBUILD_TARGET=android
    CMAKE_OPTIONS += -DHAVE_MIC_ARRAY=ON
else ifeq ($(CONFIG_AI_X86),y)
    CMAKE_OPTIONS += -DBUILD_TARGET=host
    CMAKE_OPTIONS += -DAI_X86=ON
else ifeq ($(CONFIG_AI_RASPI),y)
    CMAKE_OPTIONS += -DAI_RASPI=ON
    CMAKE_OPTIONS += -DBUILD_TARGET=raspi
    CMAKE_OPTIONS += -DHAVE_MIC_ARRAY=ON
else
	CMAKE_OPTIONS += -DHAVE_MIC_ARRAY:=ON
endif

CMAKE_FIND_ROOT_PATH := ${CMAKE_FIND_ROOT_PATH};${STAGING_DIR}

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(INSTALL_DIR) $(1)/usr/bin; \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/; \
	fi
endef

define Package/lothalproc/install
	$(INSTALL_DIR) $(1)/usr/lib
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(INSTALL_DIR) $(1)/usr/bin; \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin/; \
	fi

	$(INSTALL_DIR) $(1)/etc/init.d
	$(CP) ./files/$(APP_EXE) $(1)/etc/init.d/$(APP_EXE)
	if [ "$(CONFIG_TURENPROC_AUTO_RUN)" != "y" ]; then \
		$(SED) "s/^START/#START/g" $(1)/etc/init.d/$(APP_EXE); \
	fi
endef

$(eval $(call BuildPackage,lothalproc))


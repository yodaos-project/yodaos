include $(TOPDIR)/rules.mk

PKG_NAME:=products
PKG_RELEASE:=1
PKG_SOURCE_DIR:=$(ROKID_DIR)/products
PKG_FIXUP:=libtool
PKG_BUILD_PARALLEL:=1
PKG_LICENSE:=GPL-2.0+

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

PKG_INSTALL:=
CMAKE_OPTIONS += -DPRODUCT_NAME=$(CONFIG_PRODUCT_NAME)
CMAKE_OPTIONS += -DPRODUCT_PATHNAME=$(CONFIG_PRODUCT_PATHNAME)
CMAKE_OPTIONS += -DCMAKE_INSTALL_DIR='$(PKG_INSTALL_DIR)'
CMAKE_OPTIONS += -DCMAKE_INCLUDE_DIR='$(STAGING_DIR)'

ifeq ($(CONFIG_TARGET_amlogic_a113), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='aarch64-a113'
else ifeq ($(CONFIG_TARGET_rockchip_rk3308), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='aarch64-rk3308'
else ifeq ($(CONFIG_TARGET_brcm2708), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='aarch64-rpi3'
else ifeq ($(CONFIG_TARGET_leo), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='armhf-k18'
else ifeq ($(CONFIG_TARGET_quanzhi), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='arm-quanzhi'
else ifeq (${CONFIG_TARGET_hisilicon}, y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='armhf-hi3561'
else ifeq ($(CONFIG_TARGET_x86_64), y)
	CMAKE_OPTIONS += -DTARGET_PLATFORM='x86_64-intel'
endif

define Package/products
	SECTION:=libs
	CATEGORY:=rokid
	TITLE:=products
	DEPENDS:= +!PRODUCT_WITHOUT_JS:jsruntime +ai-libs-common
endef

define Package/products/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	$(CP) $(PKG_SOURCE_DIR)/* $(PKG_BUILD_DIR)
	$(call Build/Prepare/Default,)
endef

define Build/InstallDev
	echo "install product to staging..."
	$(INSTALL_DIR) $(1)/opt
	$(INSTALL_DIR) $(1)/etc
	$(INSTALL_DIR) $(1)/vendor
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	if [ -d $(PKG_INSTALL_DIR)/opt ]; \
		then $(CP) $(PKG_INSTALL_DIR)/opt/* $(1)/opt/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/etc ]; then \
		$(CP) $(PKG_INSTALL_DIR)/etc/* $(1)/etc/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/vendor ]; then \
		$(CP) $(PKG_INSTALL_DIR)/vendor/* $(1)/vendor/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/usr/lib ]; then \
		$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib; \
	fi

	$(INSTALL_DIR) $(1)/system/workdir_asr_cn
	if [ -d $(PKG_INSTALL_DIR)/system/workdir_asr_cn ]; \
		then $(CP) $(PKG_INSTALL_DIR)/system/workdir_asr_cn/* $(1)/system/workdir_asr_cn/; \
	fi

	if [ -d $(PKG_INSTALL_DIR)/etc/volume ]; then \
		$(INSTALL_DIR) $(1)/etc/volume/; \
		$(CP) $(PKG_INSTALL_DIR)/etc/volume/* $(1)/etc/volume/; \
	fi

	if [ -f $(PKG_BUILD_DIR)/$(CONFIG_PRODUCT_PATHNAME)/manifest.json ];  \
		then $(CP) $(PKG_BUILD_DIR)/$(CONFIG_PRODUCT_PATHNAME)/manifest.json $(1)/etc/; \
	fi

	echo "installed"
endef

define Package/products/install
	echo "install product to target...$(1)"
	$(INSTALL_DIR) $(1)/opt
	$(INSTALL_DIR) $(1)/etc/
	$(INSTALL_DIR) $(1)/vendor/
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/usr/lib
	if [ -d $(PKG_INSTALL_DIR)/opt ]; \
		then $(CP) $(PKG_INSTALL_DIR)/opt/* $(1)/opt/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/etc ]; then \
		$(CP) $(PKG_INSTALL_DIR)/etc/* $(1)/etc/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/vendor ]; then \
		$(CP) $(PKG_INSTALL_DIR)/vendor/* $(1)/vendor/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/usr/bin ]; then \
		$(CP) $(PKG_INSTALL_DIR)/usr/bin/* $(1)/usr/bin; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/usr/lib ]; then \
		$(CP) $(PKG_INSTALL_DIR)/usr/lib/* $(1)/usr/lib; \
	fi


	$(INSTALL_DIR) $(1)/system/workdir_asr_cn
	if [ -d $(PKG_INSTALL_DIR)/system/workdir_asr_cn ]; \
		then $(CP) $(PKG_INSTALL_DIR)/system/workdir_asr_cn/* $(1)/system/workdir_asr_cn/; \
	fi
	if [ -d $(PKG_INSTALL_DIR)/etc/volume ]; then \
		$(INSTALL_DIR) $(1)/etc/volume/; \
		$(CP) $(PKG_INSTALL_DIR)/etc/volume/* $(1)/etc/volume/; \
	fi

	if [ -f $(PKG_BUILD_DIR)/$(CONFIG_PRODUCT_PATHNAME)/manifest.json ];  \
		then $(CP) $(PKG_BUILD_DIR)/$(CONFIG_PRODUCT_PATHNAME)/manifest.json $(1)/etc/; \
	fi

	echo "installed"
endef

$(eval $(call BuildPackage,products))

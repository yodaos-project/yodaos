################################################################################
#
# property -- rokid porting property to buildroot.
#
################################################################################

PROPERTY_SITE = $(call qstrip,$(BR2_EXTERNAL_ROKID_PATH)/../3rd/android_lib/bionic/property)
PROPERTY_SITE_METHOD = local
PROPERTY_INSTALL_STAGING = YES
PROPERTY_DEPENDENCIES = android-kernel-headers

define PROPERTY_INSTALL_STAGING_CMDS
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/property
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/system
        cp -rp $(@D)/include/* $(STAGING_DIR)/usr/include/property
        $(INSTALL) -D -m 0644 $(@D)/prop/default.prop $(STAGING_DIR)/default.prop
        $(INSTALL) -D -m 0644 $(@D)/prop/build.prop $(STAGING_DIR)/system/build.prop
        $(INSTALL) -D -m 0755 $(@D)/libproperty.so $(STAGING_DIR)/usr/lib/libproperty.so
endef

define PROPERTY_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0644 $(@D)/prop/default.prop $(TARGET_DIR)/default.prop
        $(INSTALL) -D -m 0644 $(@D)/prop/build.prop $(TARGET_DIR)/system/build.prop
        $(INSTALL) -D -m 0755 $(@D)/libproperty.so $(TARGET_DIR)/usr/lib/libproperty.so
endef

$(eval $(cmake-package))

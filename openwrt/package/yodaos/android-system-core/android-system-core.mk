################################################################################
#
# android-system-core -- rokid porting android cutils&utils to buildroot.
#
################################################################################

ANDROID_SYSTEM_CORE_SITE = $(call qstrip,$(BR2_EXTERNAL_ROKID_PATH)/../3rd/android_lib/system/core)
ANDROID_SYSTEM_CORE_SITE_METHOD = local
ANDROID_SYSTEM_CORE_INSTALL_STAGING = YES
ANDROID_SYSTEM_CORE_DEPENDENCIES = android-kernel-headers property

#ifeq ($(BR2_STATIC_LIBS),y)
#ANDROID_SYSTEM_CORE_PIC =
#ANDROID_SYSTEM_CORE_SHARED = --static
#else
#ANDROID_SYSTEM_CORE_PIC = -fPIC
#ANDROID_SYSTEM_CORE_SHARED = --shared
#endif

define ANDROID_SYSTEM_CORE_INSTALL_STAGING_CMDS
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/cutils
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/utils
        cp -rp $(@D)/include/cutils $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/include/utils $(STAGING_DIR)/usr/include/
        $(INSTALL) -D -m 0755 $(@D)/libcutils/libandroid_cutils.so $(STAGING_DIR)/usr/lib/libandroid_cutils.so
        $(INSTALL) -D -m 0755 $(@D)/libutils/libandroid_utils.so $(STAGING_DIR)/usr/lib/libandroid_utils.so
endef

define ANDROID_SYSTEM_CORE_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/libcutils/libandroid_cutils.so $(TARGET_DIR)/usr/lib/libandroid_cutils.so
        $(INSTALL) -D -m 0755 $(@D)/libutils/libandroid_utils.so $(TARGET_DIR)/usr/lib/libandroid_utils.so
endef

$(eval $(cmake-package))

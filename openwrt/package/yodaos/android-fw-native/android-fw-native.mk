################################################################################
#
# android-fw-native -- rokid porting android binder to buildroot.
#
################################################################################

ANDROID_FW_NATIVE_SITE = $(call qstrip,$(BR2_EXTERNAL_ROKID_PATH)/../frameworks/native)
ANDROID_FW_NATIVE_SITE_METHOD = local
ANDROID_FW_NATIVE_INSTALL_STAGING = YES
ANDROID_FW_NATIVE_DEPENDENCIES = android-system-core android-kernel-headers android-hardware openssl

ifeq ($(BR2_STATIC_LIBS),y)
ANDROID_FW_NATIVE_PIC =
ANDROID_FW_NATIVE_SHARED = --static
else
ANDROID_FW_NATIVE_PIC = -fPIC
ANDROID_FW_NATIVE_SHARED = --shared
endif

define ANDROID_FW_NATIVE_INSTALL_STAGING_CMDS
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/android
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/binder
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/input
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/ui
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/powermanager
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/rokid
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/etc/idc
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/etc/keychars
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/etc/keylayout
        $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/inputflinger
        cp -rp $(@D)/libs/input/data/idc $(STAGING_DIR)/etc/
        cp -rp $(@D)/libs/input/data/keychars $(STAGING_DIR)/etc/
        cp -rp $(@D)/libs/input/data/keylayout $(STAGING_DIR)/etc/
        cp -rp $(@D)/include/android/ $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/include/binder/ $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/include/input/ $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/include/ui/ $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/include/powermanager/ $(STAGING_DIR)/usr/include/
        cp -rp $(@D)/services/inputflinger/include/* $(STAGING_DIR)/usr/include/inputflinger
        cp -rp $(@D)/include/private/rokid_types.h $(STAGING_DIR)/usr/include/rokid/
        $(INSTALL) -D -m 0755 $(@D)/libs/binder/libandroid_binder.so $(STAGING_DIR)/usr/lib/libandroid_binder.so
        $(INSTALL) -D -m 0755 $(@D)/libs/input/libandroid_input.so $(STAGING_DIR)/usr/lib/libandroid_input.so
        $(INSTALL) -D -m 0755 $(@D)/services/inputflinger/libinputflinger.so $(STAGING_DIR)/usr/lib/libinputflinger.so
endef

define ANDROID_FW_NATIVE_INSTALL_TARGET_CMDS
        $(INSTALL) -d -m 0755 $(TARGET_DIR)/etc/idc
        $(INSTALL) -d -m 0755 $(TARGET_DIR)/etc/keychars
        $(INSTALL) -d -m 0755 $(TARGET_DIR)/etc/keylayout
        cp -rp $(@D)/libs/input/data/idc $(TARGET_DIR)/etc/
        cp -rp $(@D)/libs/input/data/keychars $(TARGET_DIR)/etc/
        cp -rp $(@D)/libs/input/data/keylayout $(TARGET_DIR)/etc/
        $(INSTALL) -D -m 0755 $(@D)/libs/binder/libandroid_binder.so $(TARGET_DIR)/usr/lib/libandroid_binder.so
        $(INSTALL) -D -m 0755 $(@D)/libs/input/libandroid_input.so $(TARGET_DIR)/usr/lib/libandroid_input.so
        $(INSTALL) -D -m 0755 $(@D)/services/inputflinger/libinputflinger.so $(TARGET_DIR)/usr/lib/libinputflinger.so
        $(INSTALL) -D -m 0755 $(@D)/cmds/servicemanager/servicemanager $(TARGET_DIR)/usr/bin/servicemanager
		$(INSTALL) -D -m 644 $(ROKID_DIR)/rokid_br_external/package/android-fw-native/servicemanager.service \
			$(TARGET_DIR)/usr/lib/systemd/system/servicemanager.service
		if [ ! -d $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants ] ; then \
			mkdir -p $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants ; \
		fi
		ln -sf ../../../../usr/lib/systemd/system/servicemanager.service \
			$(TARGET_DIR)/etc/systemd/system/multi-user.target.wants/servicemanager.service
endef
define ANDROID_FW_NATIVE_INSTALL_INIT_SYSV
	$(INSTALL) -m 0755 -D  $(ROKID_DIR)/rokid_br_external/package/android-fw-native/S52rokid_servicemanager \
		$(TARGET_DIR)/etc/init.d/S52rokid_servicemanager
endef
$(eval $(cmake-package))

################################################################################
#
# lumenflinger -- rokid porting android lumenflinger to buildroot.
#
################################################################################

LUMENFLINGER_SITE = $(call qstrip,$(BR2_EXTERNAL_ROKID_PATH)/../frameworks/native/services/lumenflinger)
LUMENFLINGER_SITE_METHOD = local
LUMENFLINGER_INSTALL_STAGING = YES
LUMENFLINGER_DEPENDENCIES = android-kernel-headers android-fw-native android-hardware

define LUMENFLINGER_INSTALL_STAGING_CMDS
    $(INSTALL) -d -m 0755 $(STAGING_DIR)/usr/include/lumenflinger
    cp -rp $(@D)/include/* $(STAGING_DIR)/usr/include/lumenflinger
    cp -rp $(@D)/client/include/* $(STAGING_DIR)/usr/include/lumenflinger
    $(INSTALL) -D -m 0755 $(@D)/lumenflinger $(STAGING_DIR)/usr/bin/lumenflinger
    $(INSTALL) -D -m 0755 $(@D)/client/librklumen_light.so $(STAGING_DIR)/usr/lib/librklumen_light.so
endef
define LUMENFLINGER_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/lumenflinger $(TARGET_DIR)/usr/bin/lumenflinger
    $(INSTALL) -D -m 0755 $(@D)/client/librklumen_light.so $(TARGET_DIR)/usr/lib/librklumen_light.so
    $(INSTALL) -D -m 644 $(ROKID_DIR)/rokid_br_external/package/lumenflinger/lumenflinger.service \
    $(TARGET_DIR)/usr/lib/systemd/system/lumenflinger.service
    if [ ! -d $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants ] ; then \
        mkdir -p $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants ; \
    fi
    ln -sf ../../../../usr/lib/systemd/system/lumenflinger.service \
    $(TARGET_DIR)/etc/systemd/system/multi-user.target.wants/lumenflinger.service
endef
define LUMENFLINGER_INSTALL_INIT_SYSV
	$(INSTALL) -m 0755 -D  $(ROKID_DIR)/rokid_br_external/package/lumenflinger/S55rokid_lumenflinger $(TARGET_DIR)/etc/init.d/S55rokid_lumenflinger
endef

$(eval $(cmake-package))

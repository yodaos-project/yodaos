CC = $(CROSS_COMPILE)gcc
TARGET = bcmdhd_fw

LOCAL_CFLAGS += -I../../include
LOCAL_LDFLAGS += -L../../libs
LOCAL_LDFLAGS += -lusb-1.0

$(TARGET): multi_wifi_load_driver.c
	$(CC) -Wall multi_wifi_load_driver.c $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -DWIFI_TYPE_NAME=\"$(WIFI_TYPE_NAME)\" -o $@
	$(CC) -Wall brcm-patchram-plus.c $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS)  -o brcm_patchram_plus

clean:
	-rm $(TARGET)
	-rm brcm_patchram_plus



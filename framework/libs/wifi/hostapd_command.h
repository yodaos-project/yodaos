
#ifndef __HOSTAPD_COMMAND_H
#define __HOSTAPD_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_HOSTAPD_CTRL_PATH "/var/run/hostapd/wlan0"

struct hostapd_network {
    char ssid[36];
    char psk[68];
    char ip[32];
};

enum {
    WIFI_STATION_MODE,
    WIFI_SOFTAP_MODE,
    WIFI_MIX_MODE,
    WIFI_NONE_MODE,
}WIFI_MODE;


struct wifi_mode {
    int mode;
    pthread_mutex_t mode_mutex;
};


int hostapd_enable(struct hostapd_network *net);
int hostapd_disable();
int softap_set_ip();
int supplicant_enable();
int wifi_mode_init(int mode);

#ifdef __cplusplus
}
#endif
#endif

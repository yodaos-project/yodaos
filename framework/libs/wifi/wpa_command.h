
#ifndef __WIFI_COMMAND_H
#define __WIFI_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#define WIFI_WPA_CTRL_PATH "/var/run/wpa_supplicant/wlan0"

#define SERVER_ADDRESS  "www.taobao.com"
#define SERVER_ADDRESS_PUB_DNS  "180.76.76.76"

#define WPA_ALL_NETWORK -1

#define SSID_DEFAULT_LEN 128  // chinese utf-8
#define PSK_WPA_HEX_LEN 64

unsigned inline char a2x(const char c)
{
    switch(c) {
    case '0'...'9':
        return 0x0 + (c-'0');
    case 'a'...'f':
        return 0xa + (c-'a');
    case 'A'...'F':
        return 0xa + (c-'A');
    default:
        goto error;
    }
error:
    return 0;
}

#define WLAN_DEVICE_MAC_LEN 6

#define COPY_STR2MAC(mac, str)                                   \
    do {                                                        \
        int i = 0;                                              \
        for(i = 0; i < WLAN_DEVICE_MAC_LEN; i++) {              \
            mac[i] = (a2x(str[i*3]) << 4) + a2x(str[i*3 + 1]);  \
        }                                                       \
    } while(0)


enum WIFI_COMMAND_ACK{
    WIFI_COMMAND_ACK_INT,
    WIFI_COMMAND_ACK_OK,
    WIFI_COMMAND_ACK_STRING,
};

enum WIFI_COMMAND{
    WIFI_COMMAND_SCAN,
    WIFI_COMMAND_ADDNETWORK,
    WIFI_COMMAND_SETNETWORK_SSID,
    WIFI_COMMAND_SETNETWORK_SCANSSID,
    WIFI_COMMAND_SETNETWORK_PSK,
    WIFI_COMMAND_SETNETWORK_KEYMETH,
    WIFI_COMMAND_SETNETWORK_AUTHALG,
    WIFI_COMMAND_SETNETWORK_WEPKEY0,
    WIFI_COMMAND_SETNETWORK_WEPTXKEYID,
    WIFI_COMMAND_ENABLENETWORK,
    WIFI_COMMAND_SELECTNETWORK,
    WIFI_COMMAND_RECONFIGURE,
    WIFI_COMMAND_REATTACH,
    WIFI_COMMAND_GET_STATUS,
    WIFI_COMMAND_GET_SIGNAL,
    WIFI_COMMAND_SAVECONFIG,
    WIFI_COMMAND_SCAN_R,
    WIFI_COMMAND_LIST_ENABLED_NETWORK,
    WIFI_COMMAND_DISABLENETWORK,
    WIFI_COMMAND_GET_CHANNEL,
    WIFI_COMMAND_DISCONNCET,
    WIFI_COMMAND_RECONNCET,
    WIFI_COMMAND_REMOVENETWORK,
    WIFI_COMMAND_GET_CURRENT_SSID,
    WIFI_COMMAND_BLACKLIST_CLEAR,
    WIFI_COMMAND_BSSFLUSH,
    WIFI_COMMAND_ABORT_SCAN,
    WIFI_COMMAND_GET_DEVICE_MAC,
    WIFI_COMMAND_GET_APP_BSSID,
    WIFI_COMMAND_GET_CURRENT_ID,
    WIFI_COMMAND_GET_CURRENT_PSK,
    WIFI_COMMAND_LIST_CONFIGED_NETWORK,
};

enum WIFI_KEY_MGMT{
    WIFI_KEY_WPA2PSK,
    WIFI_KEY_WPAPSK,
    WIFI_KEY_WEP,
    WIFI_KEY_NONE,
    WIFI_KEY_OTHER,
};


enum WIFI_NETWORK_STATE{
    WIFI_INIVATE,
    WIFI_SCANING,
    WIFI_CONNECTED,
    WIFI_UNCONNECTED,
    NETSERVER_CONNECTED,
    NETSERVER_UNCONNECTED,
};


struct wifi_network{
    int id;
    char ssid[36];
    char psk[68];
    int key_mgmt;
};

struct wifi_command_pack{
    int cmd;
    int ack;
};


struct wifi_scan_res {
    char mac_addr[18];
    int freq;
    int sig;
    int key_mgmt;
    char ssid[128];
};

struct wifi_scan_list {
    int num;
    struct wifi_scan_res *ssid;
};

int wifi_join_network(struct wifi_network *net);
void dhcp_reset();
int wifi_save_network();
int wifi_get_signal(int *sig);
int wifi_get_status(int *status);
int wifi_get_scan_result(struct wifi_scan_list *list);
int network_get_status(int *status);
int wifi_get_enable_network(int *num);
int wifi_get_config_network(int *num);
int wifi_scan();
int wifi_scan_channel(int num, int *freq);
int wifi_connect_moni_socket(const char * path) ;
int wifi_ctrl_recv(char *reply, int *reply_len);
void wifi_monitor_release();
int wifi_disable_network(int *id);
int wifi_disable_all_network();
int wifi_reconfigure();
int wifi_get_current_channel(int *channel);
int wifi_disconnect_network();
int wifi_reconnect_network();
int wifi_enable_network();
int wifi_enable_all_network();
int ping_net_address(char *addr);
int wifi_remove_network(int *id);
int wifi_remove_all_network();
void wifi_goto_lowermode(int state);
int wifi_get_current_ssid(char *ssid);
int wifi_get_block_scan_result(struct wifi_scan_list *list, int timeout);
int wifi_blacklist_clear();
int wifi_clear_scan_results();
int get_local_ip(const char *eth_inf, char *ip);
int wifi_abort_scan();
int wifi_get_device_mac(uint8_t *mac);
int wifi_get_ap_bssid(uint8_t *mac);
int wifi_get_current_id(int *id);
int wifi_get_current_password(char *password);
int wifi_get_listnetwork(int *num);
int wifi_change_station_to_ap(char * ssid,char *psk,char *ip);
int wifi_change_ap_to_station(void);

#ifdef __cplusplus
}
#endif
#endif

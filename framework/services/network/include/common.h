#ifndef __NET_SERVICE_COMMON_H_
#define __NET_SERVICE_COMMON_H_
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <sys/un.h>
#include <flora-agent.h>
#include <pthread.h>
#include <eloop_mini/eloop.h>
#include "nm_log.h"


__BEGIN_DECLS

#define NET_CAPACITY_NONE        (0<<0)
#define NET_CAPACITY_MODEM       (1<<0)
#define NET_CAPACITY_WIFI        (1<<1)
#define NET_CAPACITY_ETHERENT    (1<<2)
#define NET_DEFAULT_CAPACITY     (NET_CAPACITY_NONE)

enum {
    WIFI_STATE_CLOSE = 0,
    WIFI_STATION_PRE_JOIN,
    WIFI_STATION_JOINING,
    WIFI_STATION_JOIN_FAILED,
    WIFI_STATION_CONNECTED,
    WIFI_STATION_UNCONNECTED,
    WIFI_AP_CLOSE, 
    WIFI_AP_PRE_JOIN,
    WIFI_AP_JOINING,
    WIFI_AP_CONNECTED,
    WIFI_AP_UNCONNECTED,
    MODEM_CONNECTED,
    MODEM_UNCONNECTED,
    ETHERENT_CONNECTED,
    ETHERENT_UNCONNECTED

};

struct network_service 
{
//  int network_mode;
    int wifi_event_fd;
    eloop_id_t wifi_scan;
    eloop_id_t wifi_connect;
//    eloop_id_t etherent_monitor;
    eloop_id_t modem_monitor;
    eloop_id_t network_monitor;
    eloop_id_t network_sched;
    eloop_id_t wifi_ap_timeout;

    int wpa_network_id;
    char    ssid[128];
//    char psk[68];

    int wifi_state;
    int modem_state;
    int etherent_state;
 //   pthread_mutex_t mutex;
};

enum
{
    COMMAND_OK = 0,
    COMMAND_NOK,
}; 

enum
{
 eREASON_NONE = 0,
 eREASON_WIFI_NOT_CONNECT,
 eREASON_WIFI_NOT_FOUND,
 eREASON_WIFI_WRONG_KEY,
 eREASON_WIFI_TIMEOUT,
 eREASON_WIFI_CFGNET_FAIL,
 eREASON_OUT_OF_CAPACITY,
 eREASON_WPA_SUPLICANT_ENABLE_FAIL,
 eREASON_HOSTAPD_ENABLE_FAIL,
 eREASON_COMMAND_FORMAT_ERROR,
 eREASON_COMMAND_RESPOND_ERROR,
 eREASON_WIFI_MONITOR_ENABLE_FAIL,
 eREASON_WIFI_SSID_PSK_SET_FAIL
};

extern struct wifi_network * get_wifi_network(void);
extern uint32_t get_network_capacity(void);
extern struct network_service * get_network_service(void);

__END_DECLS


#endif /* __NET_SERVICE_COMMON_H_ */

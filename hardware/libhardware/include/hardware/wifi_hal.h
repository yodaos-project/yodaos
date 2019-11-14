#ifndef _HARDWARE_WIFI_H
#define _HARDWARE_WIFI_H

#include <hardware/hardware.h>
#include <stdint.h>

__BEGIN_DECLS;
#define WIFI_HARDWARE_MODULE_ID "wifi_hal"


struct wifi_module_t {
	struct hw_module_t common;
};

struct wifi_device_t {
	struct hw_device_t common;
	int (*on)(void);
	int (*off)(void);
	int (*get_status)(void);
	int (*start_station)(void);
	int (*stop_station)(void);
	int (*start_ap)(char *ssid, char *psk, char *ipaddr);
	int (*stop_ap)(void);
};

__END_DECLS;
#endif // _HARDWARE_WIFI_H

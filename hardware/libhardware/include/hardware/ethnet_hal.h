#ifndef _HARDWARE_ETHNET_H
#define _HARDWARE_ETHNET_H

#include <hardware/hardware.h>
#include <stdint.h>

__BEGIN_DECLS;
#define ETHNET_HARDWARE_MODULE_ID "ethnet_hal"


struct eth_module_t {
	struct hw_module_t common;
};

struct eth_device_t {
	struct hw_device_t common;
	int (*on)(void);
	int (*off)(void);
	int (*get_status)(void);
	int (*start_dhcpcd)(void);
	int (*stop_dhcpcd)(void);
};

__END_DECLS;
#endif // _HARDWARE_WIFI_H

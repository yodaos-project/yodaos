#ifndef AUTOCONNECT_H
#define AUTOCONNECT_H

#include <hardware/hardware.h>

__BEGIN_DECLS;

#define AUTOCONNECT_HW_ID "autoconnect"
#define AUTOCONNECT_API_VERSION HARDWARE_MODULE_API_VERSION(1,0)

#ifndef __unused
#define __unused __attribute__((unused))
#endif

//extern void (*p_pkt_process)(char *buf, int length);

typedef struct autoconnect_device_t {
    struct hw_device_t common;

    int (*dev_close)(struct autoconnect_device_t *device);
    int (*dev_init)(void);
    int (*dev_deinit)(void);
    int (*recv_start)(void (*p_pkt_process)(char *buf, int length));
    int (*recv_stop)(void);
    int (*send_action)(uint8_t *buf, int len);
    int (*get_info)(int *freq, char *bssid, char *mac, char *ssid);

} autoconnect_device_t;

__END_DECLS;

#endif


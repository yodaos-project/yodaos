#ifndef MODEM_TTY_H
#define MODEM_TTY_H

#include <hardware/hardware.h>

__BEGIN_DECLS;

#define MODEM_TTY_HW_ID "modem_tty"
#define MODEM_API_VERSION HARDWARE_MODULE_API_VERSION(1,0)

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#define MAX_LEN 64 

struct modem_module_t {
    struct hw_module_t common;
};

typedef enum
{
   eSIGNAL_SERVICE_NONE=0,
   eSIGNAL_SERVICE_2G=2,
   eSIGNAL_SERVICE_3G=3,
   eSIGNAL_SERVICE_4G=4,
 }eMODEM_SIGNAL_TYPE;

typedef enum
{
   eSIGNAL_REG_FAIL = 0,
   eSIGNAL_IN_CMCC_MODE = 1,
   eSIGNAL_IN_CUCC_MODE = 2,
   eSIGNAL_IN_CTCC_MODE = 3,
   eSIGNAL_IN_UNDEFIND_MODE = 4,
}eMODEM_SIGNAL_VENDOR;

typedef struct 
{
  char vendor[MAX_LEN];
  char type[MAX_LEN];
  char version[MAX_LEN]; 
}sMODEM_VENDOR_INFO;

typedef struct modemtty_device_t 
{
    struct hw_device_t common;
    int (*on)(void);
    int (*off)(void);
    int (*get_status)(void);
    int (*get_signal_strength)(int*signal);
    int (*get_signal_type)(eMODEM_SIGNAL_TYPE *type,eMODEM_SIGNAL_VENDOR *vendor);
    int (*get_vendor)(sMODEM_VENDOR_INFO *info);

    int (*start_dhcpcd)(void);
    void (*stop_dhcpcd)(void);
    int (*dev_close)(struct modemtty_device_t *device);

} modemtty_device_t;

__END_DECLS;

#endif


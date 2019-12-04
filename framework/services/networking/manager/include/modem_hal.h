#ifndef _MODEM_HAL_H_
#define _MODEM_HAL_H_
#include <hardware/modem_tty.h>

extern int modem_hal_init(void);
extern void modem_hal_exit(void);
extern int modem_hal_get_capacity(void);
extern int modem_hal_start_modem(void);
extern void modem_hal_stop_modem(void);
extern int modem_hal_get_vendor(sMODEM_VENDOR_INFO *vendor);
extern int modem_hal_get_signal_type(eMODEM_SIGNAL_TYPE*type,eMODEM_SIGNAL_VENDOR *vendor);
extern int modem_hal_get_signal_strength(int*signal);

#endif

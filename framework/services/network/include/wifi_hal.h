#ifndef _WIFI_HAL_
#define _WIFI_HAL_

extern int wifi_hal_init(void);
extern void wifi_hal_exit(void);
extern int wifi_hal_get_capacity(void);
extern int wifi_hal_start_station(void); 
extern void wifi_hal_stop_station(void); 
extern int wifi_hal_start_ap(char *ssid,char *psk,char *ip);
extern void wifi_hal_stop_ap(void);

#endif

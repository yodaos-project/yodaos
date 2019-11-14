#ifndef _NETWORK_MODE_H_
#define _NETWORK_MODE_H_

extern int network_set_initmode(void);
extern uint32_t get_network_mode(void);

extern int wifi_init_start(void);
extern int network_set_mode(uint32_t mode,uint32_t start,tNETWORK_MODE_INFO*p_info);


#endif

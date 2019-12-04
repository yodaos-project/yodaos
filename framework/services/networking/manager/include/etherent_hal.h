#ifndef _ETHERENT_HAL_H_
#define _ETHERENT_HAL_H_

extern int etherent_hal_init(void);
extern void etherent_hal_exit(void);
extern int etherent_hal_get_capacity(void);
extern int etherent_hal_start_eth(void);
extern void etherent_hal_stop_eth(void);

#endif


#ifndef __POWER_SERVICE_H_
#define __POWER_SERVICE_H_

#include <hardware/power.h>

#define POWER_HARDWARE_MODULE_ID "power_hal"
#define AUTOSLEEP_TIMEOUT  60 * 1000
#define PMS_WAKELOCK "pm_service"
#define CHG_WAKELOCK "charger"
#define VAD_WAKELOCK "turen_vad"
#define SLEEP_ON "mem"
#define SLEEP_OFF "off"

#define DEEP_IDLE_ENTER 2
#define IDLE_ENTER      1
#define IDLE_EXIT       0

extern int partial_lock_total;
extern int full_lock_total;

enum {
	NO_LOCK_STATE = 0,
	PART_LOCK_STATE,
	FULL_LOCK_STATE,
};

int power_module_init();
int power_wakelock_request(int lock, const char* id);
int power_wakelock_release(const char* id);
int power_autosleep_request(const char* id);
int power_sleep_request(const char* id);
int power_deep_idle_enter();
int power_idle_enter();
int power_idle_exit();
int power_lock_state_poll();

#endif

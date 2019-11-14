#include <stdio.h>
#include <string.h>
#include <hardware/power.h>
#include <hardware/hardware.h>

#include <common.h>
#include "rklog/RKLog.h"

struct power_module_t *g_power_module;
struct power_device_t *g_power_device;
int partial_lock_total = 0;
int full_lock_total = 0;

static inline int power_device_open(const hw_module_t *module, struct power_device_t **device) {
        return module->methods->open(module, POWER_HARDWARE_MODULE_ID, (struct hw_device_t **)device);
}

int power_module_init()
{
        int status;
        status = hw_get_module(POWER_HARDWARE_MODULE_ID, (const struct hw_module_t **) &g_power_module);
        if(status == 0) {
                RKLog("hw_get_module run successfully.\n");
		status = power_device_open(&g_power_module->common, &g_power_device);
	}

	if (status != 0) {
		RKLoge("failed to get hw module: %s\n",POWER_HARDWARE_MODULE_ID);
		return status;
	}

        return 0;
}

int power_wakelock_request(int lock, const char* id)
{
	int status;

	if (g_power_device) {
		status = g_power_device->acquire_wakelock(lock, id);
		if (/*(strcmp(id, PMS_WAKELOCK) == 0) &&*/(status>0)) {
			partial_lock_total++;
			RKLog("pms: wakelock_cnt++ = %d\n", partial_lock_total);
			return 0;
		}
	}

	return -1;
}


int power_wakelock_release(const char* id)
{
	int status;

	if (g_power_device) {
		status = g_power_device->release_wakelock(id);
		if (/*(strcmp(id, PMS_WAKELOCK) == 0) &&*/(status>0)) {
			partial_lock_total--;
			RKLog("pms: wakelock_cnt-- = %d\n", partial_lock_total);
			return 0;
		}
	}

	return -1;
}

int power_autosleep_request(const char* id)
{
        int status;

        if (g_power_device) {
                status = g_power_device->set_autosleep(id);
                if (status == 0) {
                        RKLog("pms: autosleep command %s begin\n", id);
                        return 0;
                }
        }

        return -1;
}

int power_sleep_request(const char* id)
{
	int status;

	if (g_power_device) {
		status = g_power_device->set_sleep(id);
		if (status == 0) {
			RKLog("pms: sleep command %s begin\n", id);
			return 0;
		}
	}

	return -1;
}

int power_deep_idle_enter()
{
        int status;

        if (g_power_device) {
                status = g_power_device->set_idle(DEEP_IDLE_ENTER);
                if (status == 0) {
                        RKLog("pms: deep idle enter\n");
                        return 0;
                }
        }
        return -1;
}

int power_idle_enter()
{
	int status;

        if (g_power_device) {
                status = g_power_device->set_idle(IDLE_ENTER);
                if (status == 0) {
                        RKLog("pms: idle enter\n");
                        return 0;
                }
        }
	return -1;
}

int power_idle_exit()
{
	int status;

        if (g_power_device) {
                status = g_power_device->set_idle(IDLE_EXIT);
                if (status == 0) {
                        RKLog("pms: idle exit\n");
                        return 0;
                }
        }
	return -1;
}

int power_lock_state_poll()
{
	if (full_lock_total > 0) {
		return FULL_LOCK_STATE;
	} else if (partial_lock_total > 0) {
		return PART_LOCK_STATE;
	} else {
		return NO_LOCK_STATE;
	}
}


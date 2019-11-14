#ifndef _HARDWARE_POWER_H
#define _HARDWARE_POWER_H

#include <hardware/hardware.h>
#include <stdint.h>

__BEGIN_DECLS;
#define POWER_HARDWARE_MODULE_ID "power_hal"

enum {
    PARTIAL_WAKE_LOCK = 1,  // the cpu stays on, but the screen is off
    FULL_WAKE_LOCK = 2      // the screen is also on
};

// while you have a lock held, the device will stay on at least at the
// level you request.
int acquire_wake_lock(int lock, const char* id);
int release_wake_lock(const char* id);
int set_auto_sleep(const char* id);

struct power_module_t {
	struct hw_module_t common;
};

struct power_device_t {
	struct hw_device_t common;
	int (*acquire_wakelock)(int lock, const char* id);
	int (*release_wakelock)(const char* id);
	int (*set_autosleep)(const char* id);
	int (*set_sleep)(const char* id);
	int (*set_idle)(int idle);
};

__END_DECLS;
#endif // _HARDWARE_POWER_H

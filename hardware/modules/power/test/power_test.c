#include <stdio.h>
#include <hardware/power.h>
#include <hardware/hardware.h>

static inline int power_device_open(const hw_module_t *module, struct power_device_t **device) {
	return module->methods->open(module, POWER_HARDWARE_MODULE_ID,(struct hw_device_t **)device);
}

struct power_module_t *module = NULL;
struct power_device_t *power_device = NULL;

int main()
{
	int status;
	status = hw_get_module(POWER_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module);
	printf("Power HAL test begin.\n");
	if(status == 0) {
		printf("hw_get_module run successfully.\n");
		if(0 != power_device_open(&module->common, &power_device))
			return;
	}

	if (!module) {
		printf("Power HAL test error %d\n", status);
	}

	power_device->acquire_wakelock(1, "mytest");
	power_device->acquire_wakelock(1, "yourtest");
	power_device->release_wakelock("mytest");
	power_device->set_autosleep("mem");

	printf("Power HAL test end.\n");

	return 0;
}

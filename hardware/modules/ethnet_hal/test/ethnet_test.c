#include <stdio.h>
#include <string.h>
#include <hardware/ethnet_hal.h>
#include <hardware/hardware.h>

struct eth_module_t *module = NULL;
struct eth_device_t *eth_device = NULL;

static inline int eth_device_open(const hw_module_t *module, struct eth_device_t **device) {
  return module->methods->open(module, ETHNET_HARDWARE_MODULE_ID,(struct hw_device_t **)device);
}

static void show_help(void)
{
	fprintf(stdout, "usage: eth_hal_test <command>\n");
	fprintf(stdout, "<command> : status/on/off/start_dhcpcd/stop_dhcpcd/help\n");
}

int main(int argc, char **argv)
{
	int ret = -1, status = -1;
	char **parse = argv;

	if (argc == 1)
		show_help();
	else {
		status = hw_get_module(ETHNET_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module);
		printf("eth HAL test begin.\n");
		if(status == 0) {
			printf("hw_get_module run successfully.\n");
			if(0 != eth_device_open(&module->common, (struct eth_device_t *)&eth_device))
				return -1;
		}

		if (!module) {
			printf("eth HAL test error %d\n", status);
			return -1;
		}

		if (!strcmp(argv[1], "status")) {
			ret = eth_device->get_status();
			if(ret == 0)
				printf("eth module has been ready.\n");
			else
				printf("eth module is not ready.\n");
		} else if (!strcmp(argv[1], "on")) {
			eth_device->on();
		} else if (!strcmp(argv[1], "off")) {
			eth_device->off();
    } else if (!strcmp(argv[1], "start_dhcpcd")) {
			eth_device->start_dhcpcd();
		} else if (!strcmp(argv[1], "stop_dhcpcd")) {
			eth_device->stop_dhcpcd();
    } else
			show_help();
	}
	printf("test rokid eth exit\n");
	return ret;
}

#include <stdio.h>
#include <string.h>
#include <hardware/wifi_hal.h>
#include <hardware/hardware.h>

struct wifi_module_t *module = NULL;
struct wifi_device_t *wifi_device = NULL;

static inline int wifi_device_open(const hw_module_t *module, struct wifi_device_t **device) {
	return module->methods->open(module, WIFI_HARDWARE_MODULE_ID,(struct hw_device_t **)device);
}

static void show_help(void)
{
	fprintf(stdout, "usage: wifi_hal_test <command> <data>\n");
	fprintf(stdout, "<command> : status/station start or stop/ap start ssid psk ap or stop/help\n");
}

int main(int argc, char **argv)
{
	int ret = -1, status = -1;
	char **parse = argv;

	if (argc == 1)
		show_help();
	else {
		status = hw_get_module(WIFI_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module);
		printf("Wifi HAL test begin.\n");
		if(status == 0) {
			printf("hw_get_module run successfully.\n");
			if(0 != wifi_device_open(&module->common, &wifi_device))
				return -1;
		}

		if (!module) {
			printf("Wifi HAL test error %d\n", status);
			return -1;
		}

		if (!strcmp(argv[1], "status")) {
			ret = wifi_device->get_status();
			if(ret == 0)
				printf("wifi module has been ready.\n");
			else
				printf("wifi module has been not ready.\n");
		} else if (!strcmp(argv[1], "station")) {
			if (!strcmp(argv[2], "start"))
				wifi_device->start_station();
			else
				wifi_device->stop_station();
		} else if (!strcmp(argv[1], "ap")) {
			if (!strcmp(argv[2], "start")){
				printf("ssid=%s,psk=%s,ipaddr=%s\n", argv[3],argv[4],argv[5]);
				wifi_device->start_ap(argv[3], argv[4], argv[5]);
			}else
				wifi_device->stop_ap();
		} else
			show_help();

		wifi_device->common.close(wifi_device);
	}
	printf("test rokid wifi exit\n");
	return ret;
}

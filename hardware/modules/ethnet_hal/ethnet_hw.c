#define LOG_TAG "EthnetHAL"

#include <hardware/hardware.h>
#include <hardware/ethnet_hal.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#define MODULE_NAME "EthnetHAL"
#define MODULE_AUTHOR "Eric.yang@rokid.com"

#define ETH_DRIVER_PATH "/sys/class/net/eth0"

int on(void)
{
	int ret = 0;
	ret = system("ifconfig eth0 up");
	if (ret) {
		ALOGE("eth0 up error %d\n", errno);
		ret = -1;
	}
	return ret;
}

int off(void)
{
	int ret = 0;
	ret = system("ifconfig eth0 down");
	if (ret) {
		ALOGE("eth0 off error %d\n", errno);
		ret = -1;
	}
	return ret;
}

int get_status(void)
{
	if(!access(ETH_DRIVER_PATH,0))
		return 0;
	else
		return -1;
}

int start_dhcpcd(void)
{
	int ret = 0;
	ret = system("/etc/init.d/dhcpcd_eth start");
	if (ret) {
		ALOGE(" start_dhcpcd failed %d\n", errno);
		ret = -1;
	}
	return ret;
}

int stop_dhcpcd(void)
{
	int ret = 0;
	ret = system("/etc/init.d/dhcpcd_eth stop");
	if (ret) {
		ALOGE(" stop_dhcpcd failed %d\n", errno);
		ret = -1;
	}
	return ret;
}


static int eth_device_close(struct eth_device_t *device)
{
	struct eth_device_t *eth_device = (struct eth_device_t *)device;

	if (eth_device) {
		free(eth_device);
	}

	return 0;

}

static int eth_device_open(const struct hw_module_t *module, const char *name,
			     struct hw_device_t **device)
{
	struct eth_device_t *dev;
	dev = (struct eth_device_t *)malloc(sizeof(struct eth_device_t));

	if (!dev) {
		ALOGE("Eth HAL: failed to alloc space");
		return -EFAULT;
	}

	memset(dev, 0, sizeof(struct eth_device_t));
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (hw_module_t *) module;
	dev->common.close = eth_device_close;

	dev->on = on;
	dev->off = off;
	dev->get_status = get_status;
	dev->start_dhcpcd = start_dhcpcd;
	dev->stop_dhcpcd = stop_dhcpcd;

	*device = &(dev->common);
	ALOGE("Eth HAL: open successfully.\n");

	return 0;
}

static struct hw_module_methods_t eth_module_methods = {
	.open = eth_device_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = ETHNET_HARDWARE_MODULE_ID,
	.name = MODULE_NAME,
	.author = MODULE_AUTHOR,
	.methods = &eth_module_methods,
};

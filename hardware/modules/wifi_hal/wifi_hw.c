#define LOG_TAG "WifiHAL"

#include <hardware/hardware.h>
#include <hardware/wifi_hal.h>
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
#include <rklog/RKLog.h>
#include <cutils/atomic.h>

#define MODULE_NAME "WifiHAL"
#define MODULE_AUTHOR "Eric.yang@rokid.com"

#define WIFI_DRIVER_PATH "/sys/class/net/wlan0"

char g_ipaddr_ap[20] = {0};

int on(void)
{
	int ret = 0;
	RKLogi("wifi wlan0 on\n");
	ret = system("ifconfig wlan0 up");
	if (ret) {
		RKLoge("wlan0 up error %d\n", errno);
		ret = -1;
	}
	return ret;
}

int off(void)
{
	int ret = 0;

	RKLogi("wifi wlan0 down\n");
	ret = system("ifconfig wlan0 down");
	if (ret) {
		RKLoge("wlan0 off error %d\n", errno);
		ret = -1;
	}
	return ret;
}

int get_status(void)
{
	if(!access(WIFI_DRIVER_PATH,0))
		return 0;
	else
		return -1;
}

int start_station(void)
{
	int ret = 0;

	RKLogi("wifi station start\n");
	ret = system("wpa_supplicant -D nl80211 -c /data/system/wpa_supplicant.conf -iwlan0 -B -d -d ");
	if (ret < 0) {
		RKLoge("wpa_supplicant start failed\n");
		return -1;
	}
/* Because dhcpcd has been used in join_network
	ret = system("/etc/init.d/dhcpcd stop");
	if (ret < 0) {
		RKLoge("dhcpcd start failed\n");
		return -1;
	}
*/
	RKLogi("wifi station start sucess\n");
	return 0;
}

static int softap_prepare_configure(char *ssid, char *psk) {
    int fd = 0;
    char buf[1024] = {0};
    int ret = 0;
	int channel = 6;

    unlink("/data/system/hostapd.conf");

    system("mkdir -p /data/system");
    fd = open("/data/system/hostapd.conf", O_RDWR | O_CREAT, 0x666);
    if (fd < 0) {
        RKLoge("create configure error %d\n", errno);
        return -1;
    }

#if AP6XXX_USED
	channel = get_best_channel();
	if((channel < 1) || (channel > 13))
		channel = 6;//default
#endif

    if (strlen(psk) > 0) {

        sprintf(buf, "interface=wlan0\ndriver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=%d\nieee80211n=1\nieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\nwpa=3\nwpa_passphrase=%s\nwpa_key_mgmt=WPA-PSK\nwpa_pairwise=TKIP CCMP\nrsn_pairwise=CCMP\n", ssid, channel, psk);
    } else {
        sprintf(buf, "interface=wlan0\ndriver=nl80211\nctrl_interface=/var/run/hostapd\nssid=%s\nchannel=%d\nieee80211n=1\nieee80211ac=1\nhw_mode=g\nignore_broadcast_ssid=0\n#wpa=3\n#wpa_passphrase=\n#wpa_key_mgmt=WPA-PSK\n#wpa_pairwise=TKIP CCMP\n#rsn_pairwise=CCMP\n", ssid, channel);

    }

    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        RKLoge("write hostapd buf errno %d\n", errno);
        return ret;
    }

    close(fd);
    return 0;
}

int stop_station(void)
{
	system("killall wpa_supplicant");
}

int start_ap(char *ssid, char *psk, char *ipaddr)
{
	int fd = 0;
	char buf[1024] = {0};
	int ret = 0;

	int i = 0,dot = 0;
	char ipaddr_pre[20] = {0};

	RKLogi("wifi ap start\n");
	ret = softap_prepare_configure(ssid, psk);
	if (ret) {
		RKLoge("hotapd prepare config error %d\n", errno);
		return -2;
	}

	system("rm -rf /data/hostapd_debug.log");
	ret = system("hostapd -f /data/hostapd_debug.log /data/system/hostapd.conf &");
	if (ret) {
		RKLoge("hostapd start  error %d\n", errno);
		return -2;
	}

	memset(g_ipaddr_ap, 0, sizeof(g_ipaddr_ap));
	snprintf(g_ipaddr_ap, sizeof(g_ipaddr_ap), "%s", ipaddr);

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "ifconfig wlan0 %s", ipaddr);
	ret = system(buf);
	if (ret) {
		RKLoge("ifconfig wlan0  error %d\n", errno);
		return -3;
	}

	snprintf(ipaddr_pre, sizeof(ipaddr_pre), "%s", ipaddr);
	for(i = 0; i < strlen(ipaddr_pre); i++){
		if(ipaddr_pre[i] == '.')
			dot++;
		if(dot == 3){
			ipaddr_pre[i + 1] = 0;
			break;
		}
	}
	if(dot != 3){
		RKLoge("ipaddr(%s) is not legal", ipaddr);
		return -4;
	}


	system("mkdir -p /var/lib/misc");

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "/usr/sbin/dnsmasq -iwlan0  --dhcp-option=3,%s --dhcp-range=%s50,%s200,12h -p100 -d &", ipaddr, ipaddr_pre, ipaddr_pre);
	RKLogi("dnsmasq cmd:%s\n", buf);
	ret = system(buf);
	//ret = system("/usr/sbin/dnsmasq -iwlan0  --dhcp-option=3,192.168.2.1 --dhcp-range=192.168.2.50,192.168.2.200,12h -p100 -d &");
	if (ret) {
		RKLoge("dnsmasq error %d\n", errno);
		return -5;
	}

	RKLogi("wifi ap start sucess\n");

	return 0;
}

int stop_ap(void)
{
	char buf[1024] = {0};
	int ret;

	system("killall hostapd");
	system("killall dnsmasq");

	memset(buf, 0, sizeof(buf));
	sprintf(buf, "ip addr del %s dev wlan0", g_ipaddr_ap);
	ret = system(buf);
	if (ret) {
		RKLoge("ip del old ip  error %d\n", errno);
		return -1;
	}

	return 0;
}


static int wifi_device_close(struct wifi_device_t *device)
{
	struct wifi_device_t *wifi_device = (struct wifi_device_t *)device;

	if (wifi_device) {
		free(wifi_device);
	}

	return 0;

}

static int wifi_device_open(const struct hw_module_t *module, const char *name,
			     struct hw_device_t **device)
{
	struct wifi_device_t *dev;
	dev = (struct wifi_device_t *)malloc(sizeof(struct wifi_device_t));

	if (!dev) {
		RKLoge("Wifi HAL: failed to alloc space\n");
		return -EFAULT;
	}

	memset(dev, 0, sizeof(struct wifi_device_t));
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (hw_module_t *) module;
	dev->common.close = wifi_device_close;

	dev->on = on;
	dev->off = off;
	dev->get_status = get_status;
	dev->start_station = start_station;
	dev->stop_station = stop_station;
	dev->start_ap = start_ap;
	dev->stop_ap = stop_ap;

	*device = &(dev->common);
	RKLogi("Wifi HAL: open successfully.\n");

	return 0;
}

static struct hw_module_methods_t wifi_module_methods = {
	.open = wifi_device_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = WIFI_HARDWARE_MODULE_ID,
	.name = MODULE_NAME,
	.author = MODULE_AUTHOR,
	.methods = &wifi_module_methods,
};

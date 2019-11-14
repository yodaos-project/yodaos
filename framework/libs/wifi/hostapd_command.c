#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <errno.h>
#include <cutils/properties.h>
#include <curl/curl.h>
#include <signal.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/socket.h>
#include <netpacket/packet.h>

#include "hostapd_command.h"

struct wifi_mode global_mode;

int get_wifi_current_mode() {
    return global_mode.mode;
}


//Only worked on AP6XXX(AP mode),but when used for other module(realtek/cypress..), also no error occured
// #define AP6XXX_USED 1
#define AP6XXX_USED  0

#if AP6XXX_USED

#define IFNAME "wlan0"
typedef struct dhd_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} dhd_priv_cmd;

int dhd_priv_func(char *cmd)
{
	struct ifreq ifr;
	dhd_priv_cmd priv_cmd;
	int ret = 0;
	int ioctl_sock; /* socket for ioctl() use */
	char buf[500]={0};
	char ifname[IFNAMSIZ+1];
	char channel[4]={0};
	int i;

	strcpy(ifname, IFNAME);
	ioctl_sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (ioctl_sock < 0) {
		printf("wpa_ctrl err: socket(PF_INET,SOCK_DGRAM)\n");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	memset(&priv_cmd, 0, sizeof(priv_cmd));
	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

	strncpy(buf, cmd, strlen(cmd));
	priv_cmd.buf = buf;
	priv_cmd.used_len = 500;
	priv_cmd.total_len = 500;
	ifr.ifr_data = &priv_cmd;

	if ((ret = ioctl(ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
		printf("wpa_ctrl:failed to issue private commands %d,cmd=%s\n", ret, cmd);
	} else {
		printf("len = %d, ret = %d, buf=%s\n", strlen(buf), ret, buf);
		if(strncmp(buf, "2g=", 3) == 0){
			for(i=0;i<3;i++){
				channel[i] = buf[3+i];
				if(channel[i] == ' ')
					break;
			}
			ret = atoi(channel);
			if((ret < 1) || (ret > 13)){
				printf("wpa_ctrl:Err channel\n");
				ret = -1;
			}
		}

	}

	close(ioctl_sock);
	return ret;
}

static int adjust_radio_frequence(void)
{
	return dhd_priv_func("wl recal 1");
}

static int get_best_channel(void)
{
	int ret;
	int channel;

	ret = dhd_priv_func("autochannel 1");
	if(ret < 0)
		return -1;

	ret = system("iw dev wlan0 scan");
	if(ret != 0)
		return -2;

	ret = dhd_priv_func("autochannel 2");
	if(ret < 0)
		return -3;
	channel = ret;

	ret = dhd_priv_func("autochannel 0");
	if(ret < 0)
		return -4;

	return channel;
}

#endif

void set_wifi_mode(int mode) {
    pthread_mutex_lock(&global_mode.mode_mutex);
    global_mode.mode = mode;
    pthread_mutex_unlock(&global_mode.mode_mutex);
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
        printf("create configure error %d\n", errno);
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
        printf("write hostapd buf errno %d\n", errno);
        return ret;
    }

    close(fd);
    return 0;
}

static int set_current_ip(const char *ipaddr) {
    int sock_set_ip;
    struct sockaddr_in sin_set_ip;
    struct ifreq ifr_set_ip;

    memset(&ifr_set_ip,0,sizeof(ifr_set_ip));
    if( ipaddr == NULL ) {
        return -1;
    }

    sock_set_ip = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_set_ip<0) {
        perror("socket create failse...SetLocalIp!");
        return -1;
    }

    memset(&sin_set_ip, 0, sizeof(sin_set_ip));
    strncpy(ifr_set_ip.ifr_name, "wlan0", sizeof(ifr_set_ip.ifr_name) - 1);

    sin_set_ip.sin_family = AF_INET;
    sin_set_ip.sin_addr.s_addr = inet_addr(ipaddr);
    memcpy( &ifr_set_ip.ifr_addr, &sin_set_ip, sizeof(sin_set_ip));
    printf("ipaddr===%s\n",ipaddr);
    if(ioctl(sock_set_ip, SIOCSIFADDR, &ifr_set_ip) < 0) {
        perror( "Not setup interface");
        return -1;
    }

    ifr_set_ip.ifr_flags |= IFF_UP |IFF_RUNNING;

    if(ioctl(sock_set_ip, SIOCSIFFLAGS, &ifr_set_ip) < 0) {
        perror("SIOCSIFFLAGS");
        return -1;
    }

    close(sock_set_ip);
    return 0;
}


static int set_wifi_adapter_state(int state) {
    int sock_set;
    struct sockaddr_in sin_set;
    struct ifreq ifr_set;

    memset(&ifr_set,0,sizeof(ifr_set));

    sock_set = socket(AF_INET, SOCK_STREAM, 0);
    if(sock_set < 0) {
        perror("socket create failse...SetLocalIp!");
        return -1;
    }

    memset(&sin_set, 0, sizeof(sin_set));
    strncpy(ifr_set.ifr_name, "wlan0", sizeof(ifr_set.ifr_name) - 1);

    ioctl(sock_set, SIOCGIFFLAGS, &ifr_set);

    if (state) {
        ifr_set.ifr_flags |= (IFF_UP | IFF_RUNNING);
    } else {
        ifr_set.ifr_flags &= ~IFF_UP;
    }

    ioctl(sock_set,SIOCSIFFLAGS,&ifr_set);

    return 0;
}

int hostapd_enable(struct hostapd_network *net) {
    int ret = 0;
    int mode = get_wifi_current_mode();

    if (mode == WIFI_SOFTAP_MODE) {
        return 1;
    }

    if (!net) {
        return -1;
    }

    //ret = system("systemctl stop supplicant");
    //ret |= system("systemctl stop dhcpcd");
    ret = system("killall wpa_supplicant");
    ret |= system("/etc/init.d/dhcpcd stop");
    if (ret != 0) {
        printf("kill supplicant & dhcpcd error\n");
    }

    ret = softap_prepare_configure(net->ssid, net->psk);
    if (ret < 0) {
        printf("softap configure error\n");
        return ret;
    }

    ret = set_wifi_adapter_state(0);
    if (ret < 0) {
        printf("stop wifi adapter\n");
        return ret;
    }

    //ret = system("systemctl start hostapd");
    ret = system("hostapd /data/system/hostapd.conf &");
    if (ret) {
        printf("hostapd start  error %d\n", errno);
        return -2;
    }

    ret = softap_set_ip(net->ip);
    if (ret < 0) {
        printf("ip configure  error\n");
        return ret;
    }

    set_wifi_mode(WIFI_SOFTAP_MODE);

#if AP6XXX_USED
	adjust_radio_frequence();//this is not necessary
#endif

    return 0;
}

int hostapd_disable() {
    int ret = 0;
    int mode = get_wifi_current_mode();

    mode = get_wifi_current_mode();
    if (mode != WIFI_SOFTAP_MODE) {
        return -1;
    }

    ret = system("killall dnsmasq");
    //ret |= system("systemctl stop hostapd");
    ret |= system("killall hostapd");
    if (ret < 0) {
        printf("kill error\n");
    }

    set_wifi_mode(WIFI_NONE_MODE);
    return 0;
}

int softap_set_ip(const char *ip) {
    int ret = 0;
    char *p = NULL;
    char cur[36] = {0};
    char cmd[256] = {0};


    ret = set_current_ip(ip);
    if (ret < 0) {
        printf("set current ip error\n");
        return -1;
    }

    p = strrchr(ip, '.');
    if (!p) {
        return -2;
    }

    strncpy(cur, ip, p + 1 - ip);
    printf("host :: %s\n", cur);
    system("mkdir -p /var/lib/misc");
    sprintf(cmd, "dnsmasq -iwlan0  --dhcp-option=3,%s --dhcp-range=%s50,%s200,12h -p100 -D", ip, cur, cur);
    printf("cmd :: %s\n", cmd);

    ret = system(cmd);
    if (ret) {
        printf("%s is error\n", cmd);
        return ret;
    }

    return 0;
}



int supplicant_enable() {
    int ret = 0;
    int mode = get_wifi_current_mode();

    printf("wifi current mode :: %d\n", mode);

    if (mode == WIFI_STATION_MODE) {
        return 1;
    }

    ret = system("killall dnsmasq");
    //ret |= system("systemctl stop hostapd");
    ret |= system("killall hostapd");
    if (ret != 0) {
        printf("killall error\n");
    }

    ret = set_wifi_adapter_state(0);
    if (ret < 0) {
        printf("stop wifi adapter\n");
        return ret;
    }

    ret = set_current_ip("0.0.0.0");
    if (ret < 0) {
        printf("clear wlan0 ip\n");
        return ret;
    }

    //ret = system("systemctl start supplicant");
    ret = system("wpa_supplicant -D nl80211 -c /data/system/wpa_supplicant.conf -iwlan0 -B -d -d -d ");
    if (ret < 0) {
        return -2;
    }

    //ret = system("systemctl start dhcpcd");
    printf("dhcpcd restart\n");
    ret = system("/etc/init.d/dhcpcd restart");
    if (ret < 0) {
        return -3;
    }

    set_wifi_mode(WIFI_STATION_MODE);

    return 0;
}

int wifi_mode_init(int mode) {
    pthread_mutex_init(&global_mode.mode_mutex, NULL);

    set_wifi_mode(mode);

    return 0;
}

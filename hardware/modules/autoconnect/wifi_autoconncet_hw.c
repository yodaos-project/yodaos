#define LOG_TAG "autoconnect_hw"
//#define LOG_NDEBUG 0

#include <hardware/hardware.h>
#include <hardware/autoconnect.h>
#include <cutils/log.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <linux/socket.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <wpa_command.h>

#define RTL8723DF_CONFIG "/proc/net/rtl8723ds/wlan0/cus_config"
#define RTL_ENABLE_MONITOR "echo 1 > /proc/net/rtl8723ds/wlan0/cus_config"
#define RTL_DISABLE_MONITOR "echo 0 > /proc/net/rtl8723ds/wlan0/cus_config"

#define AMPK_ENABLE_MONITOR "/usr/bin/wl monitor 1"
#define AMPK_DISABLE_MONITOR "/usr/bin/wl monitor 0"

pthread_t pthread_auto;
static int action_fd = 0;
static int wl_fd = 0;
char g_ssid[128] = {0};

static int wifi_send_action(uint8_t *buf, int len)
{
    int ret = 0;

    ret = send(action_fd, buf, len, 0);
    if (ret < 0) {
        ALOGE("%s,err\n",__func__);
        return -1;
    }
    return 0;
}

static void start_monitor()
{
    if(!access(RTL8723DF_CONFIG,0)){
        ALOGE("%s,is in rtl_tec\n",__func__);
        system(RTL_ENABLE_MONITOR);
    }else{
        ALOGE("%s,is in ampk\n",__func__);
        system(AMPK_ENABLE_MONITOR);
    }
}

static void stop_monitor()
{
    if(!access(RTL8723DF_CONFIG,0)){
        ALOGE("%s,is in rtl_tec\n",__func__);
        system(RTL_DISABLE_MONITOR);
    }else{
        ALOGE("%s,is in ampk\n",__func__);
        system(AMPK_DISABLE_MONITOR);
    }
}

static void *autoconnect_thread(void (*p_pkt_process)(char *buf, int length))
{
    int ret = 0;
    struct ifreq ifr;
    struct sockaddr_ll ll;
    unsigned short protocol = 0x0003;
    unsigned char local_mac[6];

    wl_fd = socket(PF_PACKET, SOCK_RAW, htons(protocol));
    ALOGE("%s,wl_fd=%d\n", __func__, wl_fd);
    if (wl_fd < 0) {
        ALOGE("raw data socket error\n");
        return NULL;
    }
    
    strncpy(ifr.ifr_name, "wlan0", sizeof(ifr.ifr_name));

    ALOGE("ifr.ifr_name = %s \n", ifr.ifr_name);

    if (ioctl(wl_fd, SIOCGIFHWADDR, &ifr)) {
        ALOGE("failed getting mac addr\n");
        return NULL;
    }

    memcpy(local_mac, ifr.ifr_addr.sa_data, 6);
    ALOGE("local_mac=%02x:%02x:%02x:%02x:%02x:%02x\n", 
           local_mac[0], 
           local_mac[1], 
           local_mac[2], 
           local_mac[3], 
           local_mac[4], 
           local_mac[5]
        );
    
    if (ioctl(wl_fd, SIOCGIFINDEX, &ifr) < 0) {
        close(wl_fd);
        ALOGE("get infr fail\n");
        return NULL;
    }

    memset(&ll, 0, sizeof(ll));
    ll.sll_family = PF_PACKET;
    ll.sll_ifindex = ifr.ifr_ifindex;
    ll.sll_protocol = htons(protocol);
    if (bind(wl_fd, (struct sockaddr *) &ll, sizeof(ll)) < 0) {
        ALOGE("bind fail \n");
        close(wl_fd);
        return NULL;
    }

    while(1) {
        unsigned char buf[2300] = {0};
        int res = 0;
        socklen_t fromlen = 0;
        // int i = 0;

        memset(&ll, 0, sizeof(ll));
        fromlen = sizeof(ll);
        pthread_testcancel();
        res = recvfrom(wl_fd, buf, sizeof(buf), 0, (struct sockaddr *) &ll,
                       &fromlen);
        if (res < 0) {
            ALOGE("recvfrom err,maybe the pthread be cancel,res=%d\n", res);
            return NULL;
        } else {
            if (!memcmp(local_mac, buf+0, sizeof(local_mac)) ||
                !memcmp(local_mac, buf+6, sizeof(local_mac))) {
                /* ignore 802.3 packet */
                // ALOGE("ignore 802.3 packet\n");
                continue;
            }

            if(!access(RTL8723DF_CONFIG,0)){
                (*p_pkt_process)(buf + 18, res - 18);
            } else {
                (*p_pkt_process)(buf, res);
            }
        }
    }

    //close(wl_fd);//close it in func autoconnect_stop
    return NULL;
}

static int wifi_get_info(int *freq, char *bssid, char *mac, char *ssid)
{
    int ret;

    ret = wifi_get_current_channel(freq);
    if (ret < 0) {
        ALOGE("get signal error\n");
        return -1;
    }
    ALOGE("freq=%d\n",*freq);

    ret = wifi_get_ap_bssid(bssid);
    if (ret < 0) {
        ALOGE("get ap bssid\n");
        return -1;
    }
    ALOGE("bssid=%s\n",bssid);

    ret = wifi_get_device_mac(mac);
    if (ret < 0) {
        ALOGE("get device mac error\n");
        return -1;
    }
    ALOGE("mac=%s\n",mac);

    ret = wifi_get_current_ssid(ssid);
    if (ret < 0) {
        ALOGE("%s,get current ssid err\n", __func__);
        return -1;
    }
    ALOGE("ssid=%s\n",ssid);

    return 0;
}

static int action_sock_init()
{
    unsigned int len = 0;
    struct sockaddr_un addr;
    char *path = "/tmp/action_pipe";

    memset(&addr, 0, sizeof(addr));

    action_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (action_fd < 0) {
        ALOGE("wifi event client create error %d\n", errno);
        return -1;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, strlen(path));
    len = strlen(addr.sun_path) + sizeof(addr.sun_family);

    if (connect(action_fd, (struct sockaddr *)&addr, len) < 0)  {
        ALOGE("connect error,errno=%d\n", errno);
        close(action_fd);
        return -2;
    }

    return 0;
}

static int autoconnect_init(void)
{
    int ret;

    ret = action_sock_init();
    if (ret < 0) {
        ALOGE("action init error\n");
        return ret;
    }

    start_monitor();
    return 0;
}

static int autoconnect_deinit()
{
    int ret;
    ret = close(action_fd);
    if(ret != 0){
        ALOGE("%s,close fail, errno = %d\n",__func__, errno);
    }

    stop_monitor();

    return 0;
}

static int autoconnect_recv_start(void (*p_pkt_process)(char *buf, int length))
{
    ALOGE("%s\n", __func__); 

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pthread_auto, &attr, autoconnect_thread, (void *)p_pkt_process);

    return 0;
}

static int autoconnect_recv_stop()
{
    int ret;
    void *data=NULL;

    ret = close(wl_fd);
    if(ret != 0){
        ALOGE("%s,close wl_fd fail, errno = %d\n",__func__, errno);
    }
    ret = pthread_cancel(pthread_auto);
    if(ret != 0){
        ALOGE("%s,pthread_cancel fail, ret=%d,errno = %d\n",__func__, ret, errno);
    }
    pthread_join(pthread_auto, &data);
    printf("pthread_join exit code %d\n", (int)data);
    return 0;
}

static int autoconnect_dev_close(struct autoconnect_device_t *device)
{
    ALOGE("%s\n", __func__);
    if(!device)
        free(device);
    return 0;
}

static int autoconnect_dev_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device)
{

    struct autoconnect_device_t* autoconnectdev;
    autoconnectdev = (struct autoconnect_device_t*)malloc(sizeof(struct autoconnect_device_t));

    if (!autoconnectdev) {
        ALOGE("Can not allocate memory for the leds device");
        return -ENOMEM;
    }

    ALOGE("%s\n", __func__);

    autoconnectdev->common.tag = HARDWARE_DEVICE_TAG;
    autoconnectdev->common.module = (hw_module_t *) module;
    autoconnectdev->common.version = HARDWARE_DEVICE_API_VERSION(1, 0);

    autoconnectdev->dev_close = autoconnect_dev_close;

    autoconnectdev->dev_init = autoconnect_init;
    autoconnectdev->dev_deinit = autoconnect_deinit;

    autoconnectdev->recv_start = autoconnect_recv_start;
    autoconnectdev->recv_stop = autoconnect_recv_stop;
    autoconnectdev->send_action = wifi_send_action;
    autoconnectdev->get_info = wifi_get_info;

    *device = (struct hw_device_t *) autoconnectdev;
    return 0;
}

static struct hw_module_methods_t autoconnect_module_methods = {
    .open = autoconnect_dev_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = AUTOCONNECT_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = AUTOCONNECT_HW_ID,
    .name = "ROKID AUTOCONNECT HAL: The Coral solution.",
    .author = "Eric.yang@rokid",
    .methods = &autoconnect_module_methods,
};

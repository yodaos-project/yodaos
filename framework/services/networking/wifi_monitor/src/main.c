#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/route.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/prctl.h>

#include <wpa_command.h>

#include <json-c/json.h>

#define WIFI_AMS_CONNECT "{\"Wifi\":true}"

#define WIFI_AMS_UNCONNECTED "{\"Wifi\":false}"

#define WIFI_EVENT_PATH "/tmp/wifi_monitor_event"

//#define WIFI_ROAMING

#ifdef WIFI_ROAMING
#define WIFI_FAILED_TIMES 3
#define WIFI_ROAM_LIMIT_SIGNAL -65
#define WIFI_SWITCH_LIMIT_SIGNAL -80
#endif

static int is_report_unconnect = 0;
static int is_report_connect = 0;
int buflen = 2047;
char monitor_buf[2048] = {0};

static int sock_event_fd = 0;

#ifdef WIFI_ROAMING
static int channel[26] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320, 5745, 5765, 5785, 5805, 5825};

static int scan_index[6] = {4, 4, 5, 4, 4, 5};
static pthread_t p_wifi_roam;
#endif

/**
 * trans buf to net manager
 *
 * @param sock net manager socket fd
 * @param buf send buf
 * @param buflen
 *
 * @return 0 OK, other ERROR
 */
static int json_transmit_buf(int sock, char *buf, int buflen)
{
    json_object *root = json_object_new_object();
    char *trans = NULL;
    char *p_index = NULL;
    char msg[48] = {0};
    int ret = 0;

    p_index = strchr(buf + 4, ' ');

    // strncpy(msg, buf + 3, p_index - buf + 3);
    strncpy(msg, buf + 3, p_index - buf - 3);
    json_object_object_add(root, "msg", json_object_new_string(msg));

    if (p_index != NULL) {
        json_object_object_add(root, "data", json_object_new_string(p_index + 1));
    }

    trans = json_object_to_json_string(root);
    if (send(sock, trans, strlen(trans), 0) < 0) {
        printf("send sock error : %d\n", errno);
        ret = -errno;
    }

    json_object_put(root);

    return ret;
}

int judge_report_connect()
{
    int netpingtime = 0;
    int ret = 0;
    int status = 0;

    while (netpingtime++ < 3) {
        ret = network_get_status(&status);
        if (ret < 0) {
            printf("wpa connect get status error\n");
        }
        printf("current net status:: %d\n", status);

        if ((status == NETSERVER_CONNECTED) && (is_report_connect == 0)) {
            printf("wifi ams report connect\n");
            if (ReportSysStatus(WIFI_AMS_CONNECT) != 0) {
                continue;
            }
            is_report_unconnect = 0;
            is_report_connect = 1;
            return status;
        } else if (status == NETSERVER_UNCONNECTED) {
            sleep(3);
        } else {
            return status;
        }
    }
    return status;
}

void judge_report_unconnect(){
    printf("wifi ams report unconnect\n");
    if (ReportSysStatus(WIFI_AMS_UNCONNECTED) != 0) {
        return;
    }

    is_report_unconnect = 1;
    is_report_connect = 0;
}

int wifi_get_monitor_event() {
    int ret = 0;
    int network_num = 0;
    int size = 0;

    ret =  wifi_get_listnetwork(&network_num);
    if ((network_num == 0) && (is_report_unconnect == 0)) {
        judge_report_unconnect();
    } else {
        judge_report_connect();
    }

    ret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
    if (ret < 0) {
        printf("monitor socket create error %d\n", ret);
        return -1;
    }

    while (1) {
        buflen = 2047;

        memset(monitor_buf, 0, sizeof(monitor_buf));
        ret = wifi_ctrl_recv(monitor_buf, &buflen);

        if (ret == 0) {
            if (strstr(monitor_buf, "CTRL-EVENT-")) {
                if (strstr(monitor_buf, "CTRL-EVENT-BSS-") == NULL) {
                    // size = send(sock_event_fd, monitor_buf, buflen, 0);
                    ret = json_transmit_buf(sock_event_fd, monitor_buf, buflen);
                    printf("ret %d buf: %s len  %d size %d\n", ret, monitor_buf, buflen, size);
                }
            }
        } else if (ret == 1) {

        } else {
            printf("recv error :: %d\n", errno);
            wifi_monitor_release();
            ret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
            if (ret < 0) {
                printf("monitor socket create error %d\n", errno);
                return -1;
            }
        }
    }

    return 0;
}

#ifdef WIFI_ROAMING
static int find_index(int val, int *channel, int len)
{
    int i = 0;

    for (i = 0; i < len; i++) {
        if (channel[i] == val) {
            return i;
        }
    }

    return -1;
}

static int find_flag(int index, int *scan_index, int len)
{
    int i = 0;
    int sum = 0;

    for (i = 0; i < len; i++) {
        if (scan_index[i] + sum > index) {
            return i;
        }
        sum += scan_index[i];
    }
    return -1;
}

static int calc_scan_sum(int index)
{
    int i = 0;
    int sum = 0;

    for (i = 0; i < index; i++) {
        sum += scan_index[i];
    }
    return sum;
}

void *wifi_roam_scan_event(void *arg) {
    int ret = 0;
    int signal = 0;
    int first_time = 0;
    int index = 0;
    int current_freq = 0;
    int freq[6] = {0};
    static unsigned long time_sum = 0;
    int time = 1;
    // int size = 0;
    // char signalbuf[128] = {0};
    prctl(PR_SET_NAME,"wifi_roam");
    while (1) {
        ret = wifi_get_signal(&signal);
        if (ret == 0) {
            // sprintf(signalbuf, "<3>CTRL-EVENT-SIGNAL signal: %d", signal);
            // size = send(sock_event_fd, signalbuf, 128, 0);

            if (signal < WIFI_SWITCH_LIMIT_SIGNAL) {
                sleep(10);
            } else {
                if (signal < WIFI_ROAM_LIMIT_SIGNAL) {
                    if (first_time == 0) {
                        ret = wifi_get_current_channel(&current_freq);
                        if (ret < 0) {
                            continue;
                        }

                        index = find_index(current_freq, channel, sizeof(channel) / sizeof(int));
                        index = find_flag(index, scan_index, sizeof(scan_index) / sizeof(int));

                        first_time = 1;
                    }
                    else {
                        index++;

                        if (index == sizeof(scan_index) / sizeof(int)) {
                            index = 0;
                        }
                    }
                    memcpy(freq, &channel[calc_scan_sum(index)], scan_index[index] * sizeof(int));

                    wifi_scan_channel(scan_index[index], freq);

                    time_sum++;

                } else {
                    first_time = 0;
                    time = 1;
                    time_sum = 0;
                }

                if ((time_sum > 3 * sizeof(scan_index) / sizeof(int)) &&
                    (time_sum < 5 * sizeof(scan_index) / sizeof(int))) {
                    time = 60;
                }
                else if (time_sum >= 5 * sizeof(scan_index) / sizeof(int)) {
                    time = 60 * 3;
                }
                else {
                    time = 1;
                }

                sleep(time);
            }
        }
    }
}
#endif

static int wifi_sock_init() {
    int sock_fd = 0;
    unsigned int len = 0;
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));

    sock_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        printf("wifi event client create error %d\n", errno);
        return -1;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, WIFI_EVENT_PATH, strlen(WIFI_EVENT_PATH));
    len = strlen(addr.sun_path) + sizeof(addr.sun_family);

    if (connect(sock_fd, (struct sockaddr *)&addr, len) < 0)  {
        perror("connect error");
        close(sock_fd);
        return -3;
    }

    return sock_fd;
}

int main(int argc, char **argv)
{
#ifdef WIFI_ROAMING
    int ret = 0;
#endif   
    while (1) {
        if ((access(WIFI_WPA_CTRL_PATH, F_OK)) != -1) {
            break;
        } else {
            sleep(1);
            continue;
        }
    }
    printf("wpa_supplicant start ok\n");

    while (1) {
        if (AmsExInit() == 0) {
            break;
        } else {
            sleep(1);
            continue;
        }
    }
    printf("ams start ok\n");

    sock_event_fd = wifi_sock_init();
    if (sock_event_fd < 0) {
        perror("wifi socket init error");
        return -1;
    }

#ifdef WIFI_ROAMING
    ret = pthread_create(&p_wifi_roam, NULL, wifi_roam_scan_event, NULL);
    if (ret != 0) {
        printf("can't create thread: %s\n", strerror(ret));
        return -2;
    }
#endif
    wifi_get_monitor_event();

#ifdef WIFI_ROAMING
    pthread_join(p_wifi_roam, NULL);
#endif   
    return 0;
}

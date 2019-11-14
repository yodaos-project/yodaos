#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <asm/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <sys/un.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <common.h>
#include <utils/Log.h>
#include <json-c/json.h>
#include <eloop_mini/eloop.h>
#include <wifi_event.h>
#include <network_switch.h>
#include <wpa_command.h>
#include <hostapd_command.h>
#include <flora_upload.h>
#include <network_report.h>

//extern int wifi_init_mode_flag;

static pthread_t p_network_moniter;
static tNETWORK_STATUS g_network_status = {0};
static pthread_mutex_t report_mutex  = PTHREAD_MUTEX_INITIALIZER;

int monitor_get_status(tNETWORK_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(status,&g_network_status,sizeof(tNETWORK_STATUS));
    pthread_mutex_unlock(&report_mutex);

    return 0;
}

static int monitor_set_status(tNETWORK_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(&g_network_status,status,sizeof(tNETWORK_STATUS));
    pthread_mutex_unlock(&report_mutex);
    return 0;
}

void monitor_report_status(void)
{
   tNETWORK_STATUS status;

   memset(&status,0,sizeof(tNETWORK_STATUS));
   monitor_get_status(&status);
   report_network_status(&status);
}

int monitor_respond_status(flora_call_reply_t reply)
{
   tNETWORK_STATUS status;

   memset(&status,0,sizeof(tNETWORK_STATUS));
   monitor_get_status(&status);

   respond_network_status(reply,&status);
   return 0;
} 



// void judge_network_link(void *eloop_ctx) 
static void* judge_network_link(void *arg) 
{
    int ret = 0;
    tNETWORK_STATUS status;
    static int unconnect_times = 0;
    struct network_service *p_net = get_network_service();

   NM_LOGI("judge_network_link starting\n");
   while(1){
    ret = ping_net_address(SERVER_ADDRESS_PUB_DNS);
    if (ret) {
        NM_LOGI("network unconnect %s %d\n", SERVER_ADDRESS_PUB_DNS, ret);
        ret = ping_net_address(SERVER_ADDRESS);
        if (ret) {
           NM_LOGI("network unconnect %s %d\n", SERVER_ADDRESS, ret);
            unconnect_times++;
            if (unconnect_times >= 3) {

                monitor_get_status(&status);
                status.state = REPORT_UNCONNECT;
                monitor_set_status(&status);

                report_network_status(&status);
            }
        } else {
            NM_LOGI("network connect\n");
            unconnect_times = 0;

           monitor_get_status(&status);
           status.state = REPORT_CONNECT;
           monitor_set_status(&status);

            report_network_status(&status);
        }
    } else {
        NM_LOGI("network connect\n");
        unconnect_times = 0;

        monitor_get_status(&status);
        status.state = REPORT_CONNECT;
        monitor_set_status(&status);


        report_network_status(&status);
    }
    sleep(2);
 }

 //  eloop_timer_start(p_net->network_monitor);
}

int network_start_monitor(void)
{
    pthread_attr_t attr;
   struct   sched_param   param;  

	if(pthread_attr_init(&attr) != 0)
	{
		NM_LOGE("Attribute create failed\n");
		return -1;
	}

    pthread_attr_setschedpolicy(&attr, SCHED_RR);  
    param.sched_priority  = 80;  
    pthread_attr_setschedparam(&attr,   &param);  
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);

	if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) != 0)
	{
		NM_LOGE("Setting detached attribute failed\n");
		return -1;
	}

    if (pthread_create(&p_network_moniter,&attr,judge_network_link,NULL) != 0) 
    {
        NM_LOGE("Can't create thread wifi event \n");
        return -1;
    }

	(void)pthread_attr_destroy(&attr);

    return 0;
}

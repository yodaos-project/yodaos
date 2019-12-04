#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <sys/un.h>

#include <cutils/properties.h>
#include <utils/Log.h>
#include <eloop_mini/eloop.h>
#include <wifi_event.h>
#include <flora_upload.h>
#include <hostapd_command.h>
#include <wpa_command.h>
#include <ipc.h>
#include <network_switch.h>
#include <flora_upload.h>
#include <sys/prctl.h>
#include <wpa_command.h>
#include <pthread.h>
#include <network_report.h>

//#define WIFI_ROAMING 
int buflen = 2047;

int g_wifi_monitor_open = 0;
char monitor_buf[2048] = {0};
static tWIFI_STATUS g_wifi_status= {0};

#ifdef WIFI_ROAMING 
static int channel[26] = {2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 5180, 5200, 5220, 5240, 5260, 5280, 5300, 5320, 5745, 5765, 5785, 5805, 5825};
static int scan_index[6] = {4, 4, 5, 4, 4, 5};
#endif
static pthread_t p_wifi_roam;
static pthread_t p_wifi_event;

static pthread_mutex_t wifi_mutex  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  wifi_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t report_mutex  = PTHREAD_MUTEX_INITIALIZER;

static tRESULT g_result;

static uint32_t g_wifi_thread_start = 1;

const char wpa_event_msg[][48] = {
    "CTRL-EVENT-CONNECTED",
    "CTRL-EVENT-DISCONNECTED",
    "CTRL-EVENT-ASSOC-REJECT",
    "CTRL-EVENT-AUTH-REJECT",
    "CTRL-EVENT-TERMINATING",
    "CTRL-EVENT-PASSWORD-CHANGED",
    "CTRL-EVENT-EAP-NOTIFICATION",
    "CTRL-EVENT-EAP-STARTED",
    "CTRL-EVENT-EAP-PROPOSED-METHOD",
    "CTRL-EVENT-EAP-METHOD",
    "CTRL-EVENT-EAP-PEER-CERT",
    "CTRL-EVENT-EAP-PEER-ALT",
    "CTRL-EVENT-EAP-TLS-CERT-ERROR",
    "CTRL-EVENT-EAP-STATUS",
    "CTRL-EVENT-EAP-SUCCESS",
    "CTRL-EVENT-EAP-FAILURE",
    "CTRL-EVENT-SSID-TEMP-DISABLED",
    "CTRL-EVENT-SSID-REENABLED",
    "CTRL-EVENT-SCAN-STARTED",
    "CTRL-EVENT-SCAN-RESULTS",
    "CTRL-EVENT-SCAN-FAILED",
    "CTRL-EVENT-STATE-CHANGE",
    "CTRL-EVENT-BSS-ADDED",
    "CTRL-EVENT-BSS-REMOVED",
    "CTRL-EVENT-NETWORK-NOT-FOUND",
    "CTRL-EVENT-SIGNAL-CHANGE",
    "CTRL-EVENT-REGDOM-CHANGE",
    "CTRL-EVENT-CHANNEL-SWITCH",
    "CTRL-EVENT-SUBNET-STATUS-UPDATE",
};

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
#endif


static int get_wifi_msg_index(char *msgbuf) 
{
    int i = 0;

    for (i = 0; i < sizeof(wpa_event_msg) / sizeof(wpa_event_msg[0]); i++) 
    {
        if (strcmp(msgbuf, wpa_event_msg[i]) == 0) {
            return i;
        }
     }

    return -1;
}

static int get_msg_and_data(char *buf, char *msg,char *data)
{
    char *p_index = NULL;

    p_index = strchr(buf + 4, ' ');

    strncpy(msg, buf + 3, p_index - buf - 3);
    strncpy(data,p_index+1,strlen(p_index+1));

    return 0;
}


int wifi_event_get_status(tWIFI_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(status,&g_wifi_status,sizeof(tWIFI_STATUS));
    pthread_mutex_unlock(&report_mutex);

    return 0;
}

int wifi_event_set_status(tWIFI_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(&g_wifi_status,status,sizeof(tWIFI_STATUS));
    pthread_mutex_unlock(&report_mutex);
    return 0;
}

void wifi_set_state(int state)
{
   tWIFI_STATUS status;

   memset(&status,0,sizeof(tWIFI_STATUS));
   wifi_event_get_status(&status);
   status.state = state;
   wifi_event_set_status(&status);
}

static void wifi_report_connect_state(int state)
{
     tWIFI_STATUS status;

  //   NM_LOGE("########p_net->wifi_state = %d\n",state);
     memset(&status,0,sizeof(tWIFI_STATUS));
     wifi_event_get_status(&status);
     status.state = state;
     wifi_event_set_status(&status);
     report_wifi_status(&status);
}

static void wifi_report_signal_strength(int strength)
{
     tWIFI_STATUS status;

     memset(&status,0,sizeof(tWIFI_STATUS));
     wifi_event_get_status(&status);
     status.signal = strength;
     wifi_event_set_status(&status);
     report_wifi_status(&status);
}


void wifi_report_status(void)
{
     tWIFI_STATUS status;

     memset(&status,0,sizeof(tWIFI_STATUS));
     wifi_event_get_status(&status);
     report_wifi_status(&status);
}

int wifi_respond_status(flora_call_reply_t reply)
{
     tWIFI_STATUS status;

     memset(&status,0,sizeof(tWIFI_STATUS));
     wifi_event_get_status(&status);
     respond_wifi_status(reply,&status);

     return 0;
} 

static int process_recv_info(char * monitor_buf)
{
    int index = 0;
    tRESULT result;
    char msg[48];
    char data[256];
    static int wifi_faild_time   = 0;
    struct network_service *p_net =  get_network_service();

    memset(msg,0,48);
    memset(data,0,256);

    get_msg_and_data(monitor_buf,msg,data);

    memset(&result,0,sizeof(tRESULT));

    index = get_wifi_msg_index(msg);
 
    NM_LOGI("%s msg:%s,data:%s,index:%d,wifi_state:%d\n",__func__,msg,data,index,p_net->wifi_state);

    switch(index) {
    case WPA_EVENT_CONNECTED:
    case WPA_EVENT_REENABLED:
 
//         NM_LOGI("%s: msg:WPA_EVENT_CONNECTED\n",__func__);
         if(p_net->wifi_state ==WIFI_STATION_JOINING)
         {
            result.flag = 1;
            result.result = COMMAND_OK;
            result.reason = eREASON_NONE;

        //    wifi_report_connect_state(REPORT_CONNECT);
         }

         if(p_net->wifi_state == WIFI_STATION_UNCONNECTED)
         {
             NM_LOGI("%s: msg:WPA_EVENT_CONNECTED\n",__func__);
         //    wifi_report_connect_state(REPORT_CONNECT);

             p_net->wifi_state = WIFI_STATION_CONNECTED;
         }

          wifi_report_connect_state(REPORT_CONNECT);
         wifi_faild_time  = 0;
         break;

    case WPA_EVENT_ASSOC_REJECT:
    case WPA_EVENT_DISCONNECTED:
        
 //        NM_LOGI("%s: msg:WPA_EVENT_DISCONNECTED\n",__func__);
        if(++wifi_faild_time > WIFI_FAILED_TIMES) 
        {
         if(p_net->wifi_state ==WIFI_STATION_JOINING)
         {
            result.flag = 1;
            result.result = COMMAND_NOK;
            result.reason = eREASON_WIFI_NOT_CONNECT;

         //    wifi_report_connect_state(REPORT_UNCONNECT);
         }
       }
         if(p_net->wifi_state == WIFI_STATION_CONNECTED)
         {
             NM_LOGI("%s: msg:WPA_EVENT_DISCONNECTED\n",__func__);
        //     wifi_report_connect_state(REPORT_UNCONNECT);
             p_net->wifi_state = WIFI_STATION_UNCONNECTED;
//  ADD BY SJM
        //     wifi_enable_all_network();
         }
         wifi_report_connect_state(REPORT_UNCONNECT);
         break;

    case WPA_EVENT_NETWORK_NOT_FOUND:

//         NM_LOGI("%s: msg:WPA_EVENT_NETWORK_NOT_FOUND\n",__func__);
         if(p_net->wifi_state ==WIFI_STATION_JOINING)
         {
            result.flag = 1;
            result.result = COMMAND_NOK;
            result.reason = eREASON_WIFI_NOT_FOUND;

        //    wifi_report_connect_state(REPORT_UNCONNECT);
         }

        if(p_net->wifi_state == WIFI_STATION_CONNECTED)
        {

         //    wifi_report_connect_state(REPORT_UNCONNECT);
             p_net->wifi_state = WIFI_STATION_UNCONNECTED;

//  ADD BY SJM
          //   wifi_enable_all_network();
        }

     /** DEL to avoid app process the commnad when wifi signal week**/
    //     wifi_report_connect_state(REPORT_UNCONNECT);
         break;
    case WPA_EVENT_TEMP_DISABLED:

 //        NM_LOGI("%s: msg:WPA_EVENT_TEMP_DISABLED\n",__func__);
        if (strstr(data, "WRONG_KEY") != NULL) 
        {
         if(p_net->wifi_state ==WIFI_STATION_JOINING)
         {
            result.flag = 1;
            result.result = COMMAND_NOK;
            result.reason = eREASON_WIFI_WRONG_KEY;
       //     wifi_report_connect_state(REPORT_UNCONNECT);
         }
        }

        if(p_net->wifi_state == WIFI_STATION_CONNECTED)
        {
      //       wifi_report_connect_state(REPORT_UNCONNECT);
             p_net->wifi_state = WIFI_STATION_UNCONNECTED;

//  ADD BY SJM
         //    wifi_enable_all_network();
        }
        wifi_report_connect_state(REPORT_UNCONNECT);
/* 
        if(g_wifi_status.state == REPORT_CONNECT)
        {
             g_wifi_status.state = REPORT_UNCONNECT;
             report_wifi_status(&g_wifi_status);
        }
*/
        break;

    default:
        break;
    }

    if(result.flag ==1)
    {
       NM_LOGI("wifi_wakeup_wait \n");
       wifi_wakeup_wait(&result);
    }

    return 0;
}

static void * wifi_get_monitor_event(void *arg) {

    int iret = -1;
    int cnts = 5;
    int status= -100; 

   do{
     NM_LOGW("wifi_get_monitor_event enter\n");
     sleep(1);
     iret = wifi_get_status(&status);;
     NM_LOGW("wifi get status  %d\n",status);
    }while( --cnts && iret <0);

   if(iret < 0){
     NM_LOGE("wifi_get_status error %d\n",iret);
     return NULL;
   }

   if((access(WIFI_WPA_CTRL_PATH, F_OK))) 
   {
      NM_LOGE("WIFI WPA CTRL PATH:%s is not exist\n",WIFI_WPA_CTRL_PATH);
      return NULL;
      // sleep(1);
    }else{
      NM_LOGI("wifi_connect_moni_socket\n");
      iret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
      if (iret < 0) {
       NM_LOGW("monitor socket create fail %d,retry\n",iret);
      //  sleep(1);
       return NULL;
     }
   }

    g_wifi_thread_start  = 1;

    NM_LOGI("wifi_get_monitor_event enter to loop\n");

    while (g_wifi_thread_start) {
        buflen = 2047;

        memset(monitor_buf, 0, sizeof(monitor_buf));
      //  NM_LOGI("wifi_ctrl_recv wait\n");
       if(g_wifi_monitor_open == 0){
          g_wifi_monitor_open = 1;
         NM_LOGW("g_wifi_monitor_open:%d\n",g_wifi_monitor_open);
       }
        iret = wifi_ctrl_recv(monitor_buf, &buflen);

 //    NM_LOGI("*(wifi_ctrl_recv)=%d:%s\n",iret,monitor_buf);

        if (iret == 0) {
            if (strstr(monitor_buf, "CTRL-EVENT-")) {
                if (strstr(monitor_buf, "CTRL-EVENT-BSS-") == NULL) {

                    iret = process_recv_info(monitor_buf);
                }
            }
        } else if (iret == 1) {

        } else {
            NM_LOGW("recv error :: %d\n", errno);
            wifi_monitor_release();
            iret = wifi_connect_moni_socket(WIFI_WPA_CTRL_PATH);
            if (iret < 0) {
                NM_LOGE("monitor socket create error %d\n", errno);
                return NULL; 
            }
        }

       if(g_wifi_thread_start == 0)
              wifi_monitor_release();
    }

    return NULL;
}

static int report_signal_strength(int signal)
{

 #if 0
   int strength;

   if(signal >WIFI_ROAM_LIMIT_SIGNAL)
      strength = REPORT_SIGNAL_STRONG;
   else if(signal < WIFI_SWITCH_LIMIT_SIGNAL)
      strength = REPORT_SIGNAL_WEAK;
   else
      strength = REPORT_SIGNAL_MIDDLE;
    wifi_report_signal_strength(strength);
#endif

    wifi_report_signal_strength(signal);

    return 0;
}

static void* wifi_roam_scan_event(void *arg) 
{
    int iret = 0;
    int signal = 0;
    int first_time = 0;
    int index = 0;
    int current_freq = 0;
    int freq[6] = {0};
    static unsigned long time_sum = 0;
    int time = 5;

    NM_LOGW("wifi_roam_scan_event enter\n");
    g_wifi_thread_start  = 1;

    while (g_wifi_thread_start) {
        iret = wifi_get_signal(&signal);
        if (iret == 0) {
              report_signal_strength(signal);

         #ifdef WIFI_ROAMING 
            if (signal < WIFI_SWITCH_LIMIT_SIGNAL) {
                sleep(10);
            } else {
                if (signal < WIFI_ROAM_LIMIT_SIGNAL) {
                    if (first_time == 0) {
                        iret = wifi_get_current_channel(&current_freq);
                        if (iret < 0) {
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
                    time = 5;
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
                    time = 5;
                }

                sleep(time);
            }
          #else
                sleep(3);
          #endif
        }
    }
  return NULL;
}

void wifi_trigger_scan(void *eloop_ctx) 
{
    wifi_scan();
}

int wifi_wait_for_result(tRESULT *result,uint32_t ms)
{
    int iret = 0;

    struct timespec timeout;
    timeout.tv_sec = time(NULL) + ms /1000;
    timeout.tv_nsec = (ms %1000) *1000;

    NM_LOGI("wifi_wait_for_result\n");
    pthread_mutex_lock(&wifi_mutex);
    if(pthread_cond_timedwait(&wifi_cond,&wifi_mutex,&timeout) ==ETIMEDOUT)
    {
      
        g_result.result = COMMAND_NOK;
        g_result.reason = eREASON_WIFI_TIMEOUT;

        iret = -1;
    }
   
    memcpy(result,&g_result,sizeof(tRESULT));

    pthread_mutex_unlock(&wifi_mutex);


    return iret;
}

void wifi_wakeup_wait(tRESULT *src)
{
	pthread_mutex_lock(&wifi_mutex);
    memcpy(&g_result,src,sizeof(tRESULT));
	pthread_cond_signal(&wifi_cond);
	pthread_mutex_unlock(&wifi_mutex);
}

static int g_wifi_monitor_flag = 0;

int wifi_start_monitor(void)
{
 wifi_set_state(0);

 if(g_wifi_monitor_flag==0)
 {  
    pthread_attr_t attr;
    struct   sched_param   param;  

    NM_LOGI("wifi_start_monitor \n");
	if(pthread_attr_init(&attr) != 0)
	{
		NM_LOGE("Attribute create failed\n");
		return -1;
	}
/*
	if(pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED) != 0)
	{
		NM_LOGE("Setting detached attribute failed\n");
		return -1;
	}
*/

    memset(&param,0,sizeof(struct sched_param));
    pthread_attr_setschedpolicy(&attr, SCHED_RR);  
    param.sched_priority  = 81;  
    pthread_attr_setschedparam(&attr,   &param);  
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);

    NM_LOGI("get_monitor_event thread create\n");
    if (pthread_create(&p_wifi_event,&attr,wifi_get_monitor_event,NULL) != 0) 
    {
        NM_LOGE("Can't create thread wifi event \n");
        return -1;
    }
	(void)pthread_attr_destroy(&attr);

    NM_LOGI("wifi_roam_scan_event thread create\n");
    if (pthread_create(&p_wifi_roam,NULL, wifi_roam_scan_event, NULL) != 0) 
    {
        NM_LOGE("Can't create thread wifi roam\n");
        g_wifi_thread_start = 0;
        pthread_cancel(p_wifi_event);
        pthread_join(p_wifi_event,NULL);
        return -1;
    }

    g_wifi_monitor_flag  = 1;
  }
    return 0;
}

void wifi_stop_monitor(void)
{
  if(g_wifi_monitor_flag == 1)
  {
    g_wifi_monitor_flag = 0;
    g_wifi_thread_start = 0;
    g_wifi_monitor_open = 0;
    pthread_cancel(p_wifi_roam);
    pthread_cancel(p_wifi_event);
    pthread_join(p_wifi_roam,NULL);
    pthread_join(p_wifi_event,NULL);
// del by SJM
//    wifi_monitor_release();
  }
}
 


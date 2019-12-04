#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <cutils/properties.h>
#include <eloop_mini/eloop.h>
#include <wpa_command.h>
#include <hostapd_command.h>
#include <common.h>
#include <signal.h>
#include <wifi_event.h>
#include <network_switch.h>
#include <network_ctrl.h>
#include <ipc.h>
#include <wifi_hal.h>
#include <network_mode.h>
#include <modem_hal.h>
#include <etherent_hal.h>
#include <network_monitor.h>
#include <modem_event.h>
#include <etherent_event.h>
#include <network_sched.h>

//extern int g_wifi_monitor_open;
static uint32_t g_capacity = NET_DEFAULT_CAPACITY; 
static struct network_service g_network_service;
static struct wifi_network g_wifi_network;

int nm_log_level = LEVEL_OVER_LOGI;

struct wifi_network * get_wifi_network(void)
{
  return &g_wifi_network;
}

static void wifi_connecting_network(void *eloop_ctx) 
{
//   int cnts=10;
   struct wifi_network* net = (struct wifi_network*)eloop_ctx;

//    NM_LOGW("Prepare join ......\n");
//   while(--cnts && g_wifi_monitor_open == 0){
//     NM_LOGW("sleep 1s have %d cnts\n");
//     sleep(1);
//  }
//  if(g_wifi_monitor_open == 1)
//  {
      NM_LOGW("wifi_join_network\n");
      wifi_join_network(net);
//   } 
}


uint32_t get_network_capacity(void)
{
   return g_capacity; 
}

struct network_service *get_network_service(void)
{
   return &g_network_service;
}

static int nm_set_bug_level(void)
{
      char value[64] = {0};

      property_get("persist.rokid.debug.netmanager", value, "0");
      if (atoi(value) > 0)
          nm_log_level = atoi(value);

      return 0;
 }

extern int network_start_monitor(void);
static int network_parm_init(void) 
{
    struct network_service *p_net = get_network_service();

    memset(p_net,0,sizeof(struct network_service));

    p_net->wpa_network_id = -1;
    p_net->modem_state = MODEM_UNCONNECTED;
    p_net->etherent_state = ETHERENT_UNCONNECTED;
    p_net->wifi_state = WIFI_STATION_UNCONNECTED;

    p_net->wifi_scan = eloop_timer_add(wifi_trigger_scan, NULL, 1*1000, 5*1000);
    p_net->wifi_connect = eloop_timer_add(wifi_connecting_network,&g_wifi_network,10,0);
 //   p_net->network_sched = eloop_timer_add(network_sched_net,NULL,3000,5*1000);
//  p_net->etherent_monitor = eloop_timer_add(etherent_event_monitor,NULL,0,5000);
    //p_net->modem_monitor = eloop_timer_add(modem_event_monitor,NULL,1000,5*1000);
    p_net->modem_monitor = eloop_timer_add(modem_event_monitor,NULL,100,5*1000);
//    p_net->network_monitor = eloop_timer_add(judge_network_link,&g_wifi_network,2 * 1000, 0);

//    eloop_timer_start(p_net->network_monitor);
//    eloop_timer_start(p_net->network_sched);

    network_start_monitor();

    return 0;
}

#if 0 

static void sig_handler (int sig, siginfo_t *si, void *uc)
{
  if (sig == SIGINT) 
  {
     NM_LOGI("SIGINT system exit\n");

     eloop_exit();
 //  network_ipc_exit();
  }
}

int initSignal(void)
{
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO /*| SA_RESETHAND*/;
        sa.sa_sigaction = sig_handler;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == -1)
        {
                NM_LOGE("sigaction\n");
                return -1;
        }

        return 0;
}
#endif

int main(int argc, char *argv[]) 
{
    int wifi_hal_flag=0;
    int modem_hal_flag=0;
    int etherent_hal_flag=0;
/*
    if(initSignal())
    {
       NM_LOGE("signal init fail\n");
       return -1;
    }
\*/

    nm_set_bug_level(); 

    if (network_ipc_init() < 0) 
    {
        NM_LOGE("ipc init fail\n");
        return -1;
    }


   if(wifi_hal_init()  == 0)
   {
      wifi_hal_flag = 1;
      g_capacity |= get_wifi_capacity();
   }
   else
    NM_LOGW("wifi_hal_init fail\n");
   

   if(modem_hal_init()==0)
   {
      modem_hal_flag = 1;
      g_capacity |= get_modem_capacity();
   }
   else
   {
      NM_LOGW("modem_hal_init fail\n");

     if(etherent_hal_init()==0)
     {
      etherent_hal_flag = 1;
      g_capacity |= get_etherent_capacity();
     }
   else
    NM_LOGW("etherent_hal_init fail\n");

   }

   NM_LOGI("Network Capacity is:%d \n",g_capacity);

   eloop_init();

   if (network_parm_init()< 0) 
   {
        NM_LOGE("network timer init fail\n");
        return -1;
   }

   if(network_set_initmode())
   {
     NM_LOGE("network_set_initmode fail\n");
   }

   eloop_run();

   if(etherent_hal_flag)
     etherent_hal_exit();

   if(modem_hal_flag)
     modem_hal_exit();
   
   if(wifi_hal_flag)
      wifi_hal_exit();

    network_ipc_exit();

    return 0;
}

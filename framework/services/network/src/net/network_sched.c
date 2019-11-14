#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <common.h>
#include <wifi_event.h> 
#include <wpa_command.h>
#include <ipc.h>
#include <network_switch.h>
#include <hostapd_command.h>
#include <network_mode.h>
#include <wifi_hal.h>
#include <modem_hal.h>
#include <etherent_hal.h>
#include <network_monitor.h>
#include <network_mode.h>
#include <etherent_event.h>
#include <wifi_event.h>
#include <modem_event.h>
#include <cutils/properties.h>

#define NETWORK_SCHED_ENABLE_KEY                  ("persist.netmanager.sched.enable")
#define NETWORK_SCHED_WIFI2MODEM_THRESHOLD_KEY    ("persist.netmanager.w2m.threshld")
#define NETWORK_SCHED_MODEM2WIFI_THRESHOLD_KEY    ("persist.netmanager.m2w.threshld")

extern int wifi_init_mode_flag;
static int  network_sched_enable = 0;
int g_wifi_enable_flag = 0;

static int  network_wifi2modem_threshold = WIFI_SWITCH_LIMIT_SIGNAL;/* -80 */
static int  network_modem2wifi_threshold = WIFI_ROAM_LIMIT_SIGNAL;  /*-65 */

static int network_get_sched_parms(void)
{
    int val;
    int iret ;
    char value[64] = {0};

    property_get(NETWORK_SCHED_ENABLE_KEY, value, "0");

    if ((val =  atoi(value)) > 0)
        network_sched_enable  = val;

   iret = property_get(NETWORK_SCHED_WIFI2MODEM_THRESHOLD_KEY, value, NULL);

   if(iret > 0 && (val = atoi(value)) < WIFI_SWITCH_LIMIT_SIGNAL)
   {
      network_wifi2modem_threshold  = val;
   }

   iret = property_get(NETWORK_SCHED_MODEM2WIFI_THRESHOLD_KEY, value, NULL);

   if(iret > 0 && (val = atoi(value)) > WIFI_ROAM_LIMIT_SIGNAL)
   {
      network_modem2wifi_threshold  = val;
   }
        
   return 0;
}

int get_current_wifi_ssid_id(void)
{
  int iret = -1;
  int id;
  char ssid[128];
  struct network_service *p_net = get_network_service();

  memset(ssid,0,128);

  p_net->wpa_network_id = -1;
   
  if(wifi_get_current_ssid(ssid) == 0 &&  wifi_get_current_id(&id)  == 0)
   {
        if(id >= 0)
        {
          p_net->wpa_network_id = id;
          strncpy(p_net->ssid,ssid,128);
          iret  = 0;
        }
   }
   NM_LOGW("ssid:%s,id:%d\n",p_net->ssid,p_net->wpa_network_id); 
   return iret;
}

static int network_change_to_etherent(void)
{
  uint32_t  mode = get_network_mode();

   if(mode  & WIFI)
   {
      tWIFI_STATUS status;
      struct network_service *p_net = get_network_service();

      if(p_net->wpa_network_id >=0)
      {
       wifi_disable_network(&p_net->wpa_network_id); 
      }
       memset(&status,0,sizeof(tWIFI_STATUS));
       wifi_event_get_status(&status);
       status.state  = REPORT_UNCONNECT;
       wifi_event_set_status(&status);

       g_wifi_enable_flag = 0;
   }

   if(mode & MODEM)
   {
     network_configure_net(MODEM,STOP,NULL);
     network_set_mode(MODEM,STOP,NULL);
   }

   return 0;
}

static int network_change_to_modem(void)
{
  uint32_t  mode = get_network_mode();
  tNETWORK_MODE_INFO info, *p_info = &info;
  struct network_service *p_net = get_network_service();

   if(mode  & WIFI)
   {
        tWIFI_STATUS status;
        if(p_net->wpa_network_id >=0)
          wifi_disable_network(&p_net->wpa_network_id); 

        memset(&status,0,sizeof(tWIFI_STATUS));
        wifi_event_get_status(&status);
        status.state  = REPORT_UNCONNECT;
        wifi_event_set_status(&status);
    
       g_wifi_enable_flag = 0;
   }

   if(mode & MODEM)
   {
     network_configure_net(MODEM,STOP,NULL);
     network_set_mode(MODEM,STOP,NULL);
   }
 
   network_configure_net(MODEM,START,p_info);
   network_set_mode(MODEM,START,NULL);

  return 0;
}

static int network_change_to_wifi(void)
{
   uint32_t  mode = get_network_mode();
   struct network_service *p_net = get_network_service();

   if(mode & MODEM)
   {
     network_configure_net(MODEM,STOP,NULL);
     network_set_mode(MODEM,STOP,NULL);
   }

   if(mode & WIFI)
   {

    if(p_net->wpa_network_id >=0)
     {
        tWIFI_STATUS status;

        NM_LOGW("ssid:%s,id:%d\n",p_net->ssid,p_net->wpa_network_id); 
        wifi_enable_network(&p_net->wpa_network_id); 
        memset(&status,0,sizeof(tWIFI_STATUS));
        wifi_event_get_status(&status);
        status.state  = REPORT_CONNECT;
       wifi_event_set_status(&status);
       g_wifi_enable_flag = 1;
     }
   }
   else 
   {
     if(mode & WIFI_AP)
     {
       network_configure_net(WIFI_AP,STOP,NULL);
       network_set_mode(WIFI_AP,STOP,NULL);
     }

     if( wifi_init_start()==0)
     {
       NM_LOGW("wifi init start ok\n");
     }
     else
     {
       NM_LOGW("wifi init start fail\n");
     }
   }


   return 0;
}

static int get_wifi_ssid_signal(char *ssid,int* signal) 
{
    int i = 0;
    int iret = -1;
    int sig=0;
    struct wifi_scan_list list;
    
    memset(&list, 0, sizeof(struct wifi_scan_list));

    wifi_get_block_scan_result(&list,3);

   for (i = 0; i < list.num; i++) 
   {
     if(strcmp(list.ssid[i].ssid,ssid) ==  0)
     {
        sig = list.ssid[i].sig;
        iret = 0 ;
     }
   }

   if(iret == 0 && signal)
     *signal = sig; 

//    NM_LOGW("get_wifi_ssid_signal:ssid %s,signal:%d\n",ssid,sig);

    return iret;
}

void network_sched_net(void *eloop_ctx) 
{
  static int wait_wifi_init_cnt = 0;
   tWIFI_STATUS wifi_status;
#if 0
   tNETWORK_STATUS network_status;
#endif
   int wifi_state = 0;
   static int state = REPORT_UNCONNECT;
   uint32_t capacity = get_network_capacity();
   struct network_service *p_net = get_network_service();
   uint32_t  mode = get_network_mode();


    network_get_sched_parms();

//    NM_LOGI("sched_enable:%d,wifi2modem_threshold:%d,_modem2wifi_threshold:%d,mode:%d\n",network_sched_enable,network_wifi2modem_threshold,network_modem2wifi_threshold,mode);

   if(capacity & NET_CAPACITY_WIFI )
   {
     memset(&wifi_status,0,sizeof(tWIFI_STATUS));
     wifi_event_get_status(&wifi_status);

    if(wifi_status.state == REPORT_CONNECT)
          g_wifi_enable_flag = 1;
    else
         g_wifi_enable_flag = 0;

   
    if(p_net->wifi_state != WIFI_STATION_CONNECTED)
          g_wifi_enable_flag = 0;
       
//     wifi_get_status(&wifi_status); 


     //  wifi_event_get_status(&wifi_status);
#if 0
       monitor_get_status(&network_status);

      if(network_status.state == REPORT_UNCONNECT && wifi_status.state != REPORT_UNCONNECT)
          wifi_set_state(REPORT_UNCONNECT);
      else if (network_status.state == REPORT_CONNECT && wifi_status.state != REPORT_CONNECT)
          wifi_set_state(REPORT_CONNECT);
#endif
    }

     if(wifi_init_mode_flag == 1)
     {

         get_current_wifi_ssid_id();
         wifi_get_status(&wifi_state); 
 //        NM_LOGW("status:%d,g_wifi_enable_flag:%d,wifi_state:%d,WIFI_STATION_CONNECTED:%d\n",wifi_state,g_wifi_enable_flag,p_net->wifi_state,WIFI_STATION_CONNECTED);

         if( p_net->wpa_network_id >=0 &&wifi_state == 2)
         { 
  //          NM_LOGW(" wifi init ok\n");
            wifi_init_mode_flag = 0;
            wifi_set_state(REPORT_CONNECT);
            p_net->wifi_state  = WIFI_STATION_CONNECTED;
            g_wifi_enable_flag  = 1;
            network_set_mode(WIFI,START,NULL);
            wait_wifi_init_cnt = 0;

          if(mode & MODEM)
          {
               network_configure_net(MODEM,STOP,NULL);
               network_set_mode(MODEM,STOP,NULL);
          }
         }
         else
         {
  
           wait_wifi_init_cnt ++;
   //        NM_LOGW("wait wifi cnts:%d\n",wait_wifi_init_cnt);

          if(wait_wifi_init_cnt >= 30)
          {
            wait_wifi_init_cnt = 0;
            wifi_init_mode_flag = 0;
          }
          return ;
         }
     }


    if(capacity & NET_CAPACITY_ETHERENT)
    {
      tETHERENT_STATUS eth_status;

      memset(&eth_status,0,sizeof(tETHERENT_STATUS));

      if(etherent_get_start_flag() == 0)
      {
        if(etherent_hal_start_eth() == 0 && etherent_start_monitor() == 0)
        {
           NM_LOGW("etherent start ok\n"); 
           network_set_mode(ETHERENT,START,NULL);
        }
        else
           NM_LOGW("etherent start fail\n"); 
      }
      
      if(etherent_get_start_flag() ==1)
      {
        etherent_get_rt_status(&eth_status);

        report_etherent_status(&eth_status);

        if(network_sched_enable == 1 && eth_status.state != state)
        {

          if(eth_status.state == REPORT_CONNECT)
          {
             if(network_change_to_etherent() == 0)
             {
               NM_LOGW("network_change_to_etherent()\n");
               state = eth_status.state;
             }
          }
          else if(eth_status.state == REPORT_UNCONNECT)
          {
           if(capacity & NET_CAPACITY_WIFI)
           {
             if(network_change_to_wifi() == 0)
             {
               NM_LOGW("network_change_to_wifi()\n");
               state = eth_status.state;
             } 
           }
           else if(capacity & NET_CAPACITY_MODEM)
           {
             if(network_change_to_modem() == 0)
             {
               NM_LOGW("network_change_to_modem()\n");
               state = eth_status.state;

             }
           }
        }
         //  state = eth_status.state;
           return ;
      }
    }
  }
   if(network_sched_enable == 1 &&(capacity & NET_CAPACITY_WIFI) &&(capacity & NET_CAPACITY_MODEM))
   {
      //uint32_t  mode = get_network_mode();

      if((mode & WIFI) && g_wifi_enable_flag) 
      {
        int signal;
         
    //    NM_LOGI("here\n");
        if(wifi_get_signal(&signal))
          return;

    //    NM_LOGI("wifi.status.state:%d,wifi.signal:%d\n",wifi_status.state,signal);

       #if 0
        do{
          static int test = 1;

           NM_LOGI("for test\n");
          if(test ==1)
          {
            signal = 0;
            test = 0;
          }
        
         }while(0);
       #endif

        if((signal <  WIFI_SWITCH_LIMIT_SIGNAL) || signal == 0)
        {
          //  NM_LOGI("here\n");
           // NM_LOGI("begin to  change to mdemg _wifi_enable_flag:%d\n",g_wifi_enable_flag);
             if(network_change_to_modem() == 0)
             {
                NM_LOGW("network_change_to_modem() \n");
             }
             return;
        }
      }
      
      if((mode& MODEM)  && !(mode& WIFI_AP) && g_wifi_enable_flag == 0 )
      {   
         int signal=0;

//         NM_LOGW("modem sched 1\n");
         if(!(mode & WIFI) && wifi_init_mode_flag == 0)
         {
//            NM_LOGW("modem sched 2\n");
            if(wifi_init_start() ==0)
            {
               NM_LOGW("wifi_init_start ok\n");
            }
            else
            {
               NM_LOGW("wifi_init_start fail\n");
            }
            
            return;
         }
        
//          NM_LOGW("modem sched 3\n");
         if((mode & WIFI) && get_wifi_ssid_signal(p_net->ssid,&signal) == 0 && 
         (signal > WIFI_ROAM_LIMIT_SIGNAL && signal < 0 ))
         {
 //          NM_LOGW("modem sched 4\n");
           if(network_change_to_wifi() == 0)
           {
              NM_LOGW("network_change_to_wifi()\n");
           }
            return;
         }
      }
   }
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <common.h>
#include <wifi_event.h>
#include <network_ctrl.h>
#include <network_switch.h>
#include <hostapd_command.h>
#include <wpa_command.h>
#include <wifi_hal.h>
#include <modem_hal.h>
#include <etherent_hal.h>
#include <modem_event.h>
#include <etherent_event.h>

extern int g_wifi_monitor_open;
#define PTHREAD_WAIT_DELAY (20000)

static pthread_mutex_t switch_mutex  = PTHREAD_MUTEX_INITIALIZER;

static int network_start_wifi_ap(tNETWORK_MODE_INFO * info) 
{
    int iret  = 0;
   info->reason = eREASON_NONE;

   if(wifi_hal_start_ap(info->ssid,info->psk,info->ip))
   {
      info->reason = eREASON_HOSTAPD_ENABLE_FAIL;
      iret = -1;
   }
    
   NM_LOGI("wifi ap start\n");

    return iret;
}

static int network_stop_wifi_ap(void)
{
  wifi_hal_stop_ap();
  return 0;
}


static int network_start_wifi(tNETWORK_MODE_INFO * info) 
{
    int iret = 0;
    int cnts=10;
    tRESULT result;
    struct wifi_network *net = get_wifi_network();
    struct network_service *p_net  = get_network_service();

    info->reason = eREASON_NONE;

    if(wifi_hal_start_station())
    {
       info->reason = eREASON_WPA_SUPLICANT_ENABLE_FAIL;
       NM_LOGE("wifi_hal_start_station fail\n");
       return -1;
    }

    if(wifi_start_monitor())
    {
       NM_LOGE("wifi_start_monitor fail\n");
       info->reason = eREASON_WIFI_MONITOR_ENABLE_FAIL;
       return -1;
    }

    NM_LOGW("wait for wpa_supplicat init over ...\n");
    while(--cnts && g_wifi_monitor_open == 0){
     NM_LOGW("sleep 1s have %d cnts\n",cnts);
     sleep(1);
  }

  if(g_wifi_monitor_open == 0)
  {
      NM_LOGE("wpa_supplicant open fail\n");
      info->reason = eREASON_WPA_SUPLICANT_ENABLE_FAIL;
      return -1;
  }

    NM_LOGI("wifi_disable_all_network init\n");
    if(wifi_disable_all_network())
    {
      NM_LOGE("wifi_disable_all_network fail\n");
      info->reason = eREASON_COMMAND_RESPOND_ERROR;
      return -1;
    }
    NM_LOGI("wifi_disable_all_network over\n");

    memset(net,0,sizeof(struct wifi_network));
    strncpy(net->ssid, info->ssid, strlen(info->ssid)+1);
    strncpy(net->psk, info->psk, strlen(info->psk)+1);

    if(strlen(info->psk) == 0)
      net->key_mgmt = WIFI_KEY_NONE;

    NM_LOGI("eloop start wifi_join_network\n");
    eloop_timer_start(p_net->wifi_connect);

   if(wifi_wait_for_result(&result,PTHREAD_WAIT_DELAY))
   {
        info->reason = eREASON_WIFI_TIMEOUT;

        NM_LOGW("wait for result time out\n");
      //  return -1;
   }
   else
   {
     NM_LOGI("wait for result correct return\n");
     info->reason = result.reason;
   }

  if(result.result != COMMAND_OK)
  {
     int id;
 
     if(wifi_get_current_id(&id) == 0)
     {
       NM_LOGI("##wiif current id:%d\n",id); 
       if(id != -1)
         wifi_remove_network(&id); 
     } 

    iret =  -1;
/*
    wifi_stop_monitor();
    wifi_hal_stop_station();
*/
  }
  else 
    wifi_save_network();

  return iret;
}

static int network_stop_wifi(void) 
{
    wifi_stop_monitor();
    wifi_hal_stop_station();

    return 0;
}

static int network_start_modem(tNETWORK_MODE_INFO * info) 
{
  int iret  = 0;
 
  info->reason = eREASON_NONE;

  if(modem_hal_start_modem() || modem_start_monitor())
  {
      info->reason = eREASON_COMMAND_RESPOND_ERROR;
      iret = -1;
  }

  return iret;
}

static int network_stop_modem(void) 
{
    modem_hal_stop_modem();
    modem_stop_monitor();

    return 0;
}

static int network_start_etherent(tNETWORK_MODE_INFO * info)
{
  int iret = 0;

  info->reason = eREASON_NONE;

  if(etherent_hal_start_eth() || etherent_start_monitor())
  {
      info->reason = eREASON_COMMAND_RESPOND_ERROR;
      iret = -1;
  }

    return iret;
}

static int network_stop_etherent(void)
{
   etherent_stop_monitor();
   etherent_hal_stop_eth();

   return 0;
}

int is_mode_in_capacity(int mode)
{
    int iret  = -1;
    int capacity = get_network_capacity();

    switch(mode)
    {
      case WIFI:
      case WIFI_AP:
       if(capacity & NET_CAPACITY_WIFI)
         iret  = 0;
        break;

      case MODEM:
       if(capacity & NET_CAPACITY_MODEM)
           iret = 0;
        break;

      case ETHERENT: 
       if(capacity & NET_CAPACITY_ETHERENT)
           iret = 0;
        break;
      default:
        break; 
    }
     
    return iret; 
}
 
int  network_configure_net(int mode,int start,tNETWORK_MODE_INFO*info)
{
    int iret = 0;

    if(info)
      info->reason  = 0;

    if(is_mode_in_capacity(mode))
    {
       NM_LOGE("mode(%d) out of capacity\n",mode);
       info->reason = eREASON_OUT_OF_CAPACITY;
       return -1;
    }

    pthread_mutex_lock(&switch_mutex);

     switch (mode)  {
        case WIFI:
            if(start == START)
            iret =  network_start_wifi(info); 
            else 
            iret = network_stop_wifi();
            break;
        case WIFI_AP:
          if(start == START)
            iret = network_start_wifi_ap(info);
          else  
          {
            iret = network_stop_wifi_ap();
          }
            break;
        case MODEM:
           if(start == START)
           iret  = network_start_modem(info); 
           else
           iret = network_stop_modem();
           break;
        case ETHERENT:
           if(start == START)
           iret  = network_start_etherent(info);
           else
           iret = network_stop_etherent();
            break;
        default:
            break;
        }

      pthread_mutex_unlock(&switch_mutex);

     return iret;
 }


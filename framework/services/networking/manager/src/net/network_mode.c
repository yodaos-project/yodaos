#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <common.h>
#include <network_ctrl.h>
#include <wifi_hal.h>
#include <wifi_event.h>
#include <network_switch.h>
#include <cutils/properties.h>
#include <modem_hal.h>
#include <modem_event.h>
#include <etherent_hal.h>
#include <etherent_event.h>
#include <wpa_command.h>

#define NETWORK_DEVICE_OFF      ("0")
#define NETWORK_DEVICE_ON       ("1")

#define NETWORK_WIFI_MODE_KEY     ("persist.netmanager.wifi")
#define NETWORK_WIFI_AP_MODE_KEY  ("persist.netmanager.wifi.ap")
#define NETWORK_MODEM_MODE_KEY    ("persist.netmanager.modem")
#define NETWORK_ETHERENT_MODE_KEY ("persist.netmanager.etherent")

#define NETWORK_WIFI_AP_SSID      ("persist.netmanager.ap.ssid")
#define NETWORK_WIFI_AP_PSK       ("persist.netmanager.ap.psk")
#define NETWORK_WIFI_AP_HOSTIP    ("persist.netmanager.ap.hostip")

#define NETWORK_WIFI_DEFAULT_SSID   ("ROKID.TC")
#define NETWORK_WIFI_DEFAULT_PSK    ("rokidguys")
#define NETWORK_WIFI_DEFAULT_IP   ("192.168.2.1")

int wifi_init_mode_flag=0;
static volatile uint32_t  g_network_mode=0; 

static pthread_mutex_t mode_mutex  = PTHREAD_MUTEX_INITIALIZER;

uint32_t  get_network_mode(void)
{
    uint32_t mode;
   
    pthread_mutex_lock(&mode_mutex);
    mode = g_network_mode;
    pthread_mutex_unlock(&mode_mutex);

   return mode;
}

static void set_wifi_station_mode(int start)
{
  if(start == START)
  {
    g_network_mode |= WIFI;
    g_network_mode &= ~WIFI_AP;
  }
  else
    g_network_mode &= ~WIFI;

}

static void set_wifi_ap_mode(int start)
{
  if(start == START)
  {
    g_network_mode &=~ WIFI;
    g_network_mode |= WIFI_AP;
  }
  else
    g_network_mode &= ~WIFI_AP;

}

static void set_modem_mode(int start)
{
  if(start == START)
   g_network_mode |= MODEM;
  else
   g_network_mode &= ~MODEM;
}

static void set_etherent_mode(int start)
{
 if(start== START)
    g_network_mode |= ETHERENT;
 else
    g_network_mode &= ~ETHERENT;
 
}

int network_set_mode(uint32_t mode,uint32_t start,tNETWORK_MODE_INFO*p_info)
{

  pthread_mutex_lock(&mode_mutex);

  switch(mode)
  {
    case WIFI:
         set_wifi_station_mode(start);
#if 0
         if(start == START)
         {
           property_set(NETWORK_WIFI_MODE_KEY,NETWORK_DEVICE_ON);
           property_set(NETWORK_WIFI_AP_MODE_KEY,NETWORK_DEVICE_OFF);
         }
         else 
           property_set(NETWORK_WIFI_MODE_KEY,NETWORK_DEVICE_OFF);
#endif

         break;
    case WIFI_AP:
         set_wifi_ap_mode(start);
#if 0
         if(start == START)
         {
           property_set(NETWORK_WIFI_MODE_KEY,NETWORK_DEVICE_OFF);
           property_set(NETWORK_WIFI_AP_MODE_KEY,NETWORK_DEVICE_ON);

           property_set(NETWORK_WIFI_AP_SSID,p_info->ssid);
           property_set(NETWORK_WIFI_AP_PSK,p_info->psk);
           property_set(NETWORK_WIFI_AP_HOSTIP,p_info->ip);
         }
         else
           property_set(NETWORK_WIFI_AP_MODE_KEY,NETWORK_DEVICE_OFF);
#endif
 
         break;
    case MODEM:
         set_modem_mode(start);
#if 0
         if(start == START)
           property_set(NETWORK_MODEM_MODE_KEY,NETWORK_DEVICE_ON);
         else
           property_set(NETWORK_MODEM_MODE_KEY,NETWORK_DEVICE_OFF);
#endif
        break;
    case ETHERENT:
         set_etherent_mode(start);
#if 0
         if(start == START)
           property_set(NETWORK_ETHERENT_MODE_KEY,NETWORK_DEVICE_ON);
         else
           property_set(NETWORK_ETHERENT_MODE_KEY,NETWORK_DEVICE_OFF);
#endif
 
        break;
     default:
        break;
  }

  pthread_mutex_unlock(&mode_mutex);
  return 0;     
}

static int network_get_mode(uint32_t *cur_mode)
{
  uint32_t mode=0;
  char val[64]={0};  

  property_get(NETWORK_WIFI_MODE_KEY,val,"0");

  if(atoi(val))
    mode  |= WIFI;

  property_get(NETWORK_WIFI_AP_MODE_KEY,val,"0");

  if(atoi(val))
    mode  |= WIFI_AP;

  property_get(NETWORK_MODEM_MODE_KEY,val,"0");

  if(atoi(val))
    mode  |= MODEM;

  property_get(NETWORK_ETHERENT_MODE_KEY,val,"0");

  if(atoi(val))
    mode  |= ETHERENT;

  *cur_mode = mode;
 
  return 0;
}

int wifi_init_start(void)
{
   int iret = -1;

   if(wifi_hal_start_station() == 0 && wifi_start_monitor()==0)
    {
       NM_LOGW(" wifi station start ok\n");
       wifi_init_mode_flag = 1;
       iret = 0;
    }
    else
    {
       NM_LOGW(" wifi station start fail\n");
    }

   return iret; 
}

static int wifi_ap_init_start(void)
{
     int iret = -1;
     tNETWORK_MODE_INFO info,*p_info = &info;
     memset(p_info,0,sizeof(tNETWORK_MODE_INFO));

     property_get(NETWORK_WIFI_AP_SSID,p_info->ssid,NETWORK_WIFI_DEFAULT_SSID);
     property_get(NETWORK_WIFI_AP_PSK,p_info->psk,NETWORK_WIFI_DEFAULT_PSK );
     property_get(NETWORK_WIFI_AP_HOSTIP,p_info->ip,NETWORK_WIFI_DEFAULT_IP);

    if(wifi_hal_start_ap(p_info->ssid,p_info->psk,p_info->ip)==0)
    {
       network_set_mode(WIFI_AP,START,NULL);
       NM_LOGW("wifi ap start ok\n");
       iret = 0;
    }
    else
    {
       NM_LOGW("wifi ap start fail\n");
    }
    return iret;
}

#define WIFI_FLAG "ssid"
#define WIFI_CFG_FILE_PATH "/data/system/wpa_supplicant.conf"

int is_wifi_configured(void){
   FILE *fp;
   char *str;
   int flag = 0;
   char buf[256]= {0};

   if((fp = fopen(WIFI_CFG_FILE_PATH,"r"))== NULL){
     printf("error:%s open fail\n",WIFI_CFG_FILE_PATH);
     return 0;
   } 
     
    while(!feof(fp)){
      memset(buf,0,256);
      if((str=fgets(buf,256,fp)) != NULL){
        if(strstr(str,WIFI_FLAG) != NULL){
           flag = 1;
         }
      }
    }
    fclose(fp);

    NM_LOGI("wifi config status:%d\n",flag);
    return flag;
}

int network_set_initmode(void)
{
  uint32_t mode;

  network_get_mode(&mode);

  NM_LOGW("network mode:%d\n",mode);

  if((mode&MODEM) && is_mode_in_capacity(MODEM)==0)
  {
    NM_LOGW("network mode:%d\n",mode);
    if(modem_hal_start_modem()==0 && modem_start_monitor()==0)
    {
      sleep(1);
      NM_LOGW("modem start ok \n");
      network_set_mode(MODEM,START,NULL);
    }
    else
    {
      NM_LOGW("modem start fail\n");
    }
  }

  if((mode&ETHERENT) && is_mode_in_capacity(ETHERENT)==0)
  {
     if(etherent_hal_start_eth()==0 && etherent_start_monitor()==0)
     {
       NM_LOGW("etherent start ok \n");
       network_set_mode(ETHERENT,START,NULL);
     }
     else
     {
       NM_LOGW("etherent start fail\n");
     }
  }

  if((mode&WIFI)  && !(mode &WIFI_AP) && is_mode_in_capacity(WIFI)==0)
  {
    wifi_init_start();
  }
  else if(!(mode&WIFI)  && (mode&WIFI_AP) && is_mode_in_capacity(WIFI_AP)==0)
  {
     wifi_ap_init_start();
  }
  else if(is_wifi_configured())
  {
    wifi_init_start();
    wifi_set_state(1);
  }
  
  NM_LOGI("current network work mode :%d\n",g_network_mode);

  return 0;
}


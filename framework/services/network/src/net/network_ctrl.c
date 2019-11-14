#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cutils/properties.h>
#include <json-c/json.h>
#include <common.h>
#include <wifi_event.h> 
#include <wpa_command.h>
#include <ipc.h>
#include <network_switch.h>
#include <hostapd_command.h>
#include <wifi_hal.h>
#include <network_mode.h>
#include <modem_hal.h>
#include <etherent_hal.h>
#include <network_monitor.h>
#include <etherent_event.h>
#include <wifi_event.h>
#include <modem_event.h>
#include <wpa_ctrl.h>
#include <resolv.h>


#define CMD_DEVICE_KEY  "device"
#define CMD_COMMAND_KEY "command"

#define NET_MODEM_S     ("modem")
#define NET_WIFI_S      ("wifi")
#define NET_WIFI_AP_S   ("wifi_ap")
#define NET_ETHERENT_S  ("etherent")

#define WIFI_SSID_KEY          ("SSID")
#define WIFI_PASSWD_KEY        ("PASSWD")
#define WIFI_AP_IP_KEY         ("IP") 
#define WIFI_AP_TIMEOUT        ("TIMEOUT")

#define NET_PARMS_KEY   "params"
#define NET_MAX_CAPACITY       (4)

#define CMD_MAX_LEN (64)

tNETWORK_MODE_INFO  g_ap_info = {0};

typedef struct ctrl_cmd_format
{
   char *device;
   char *command;
   int (*respond_func)(flora_call_reply_t reply,void *data);
}tCTRL_CMD_FMT;

typedef struct ctr_cmd_string
{
   char device[CMD_MAX_LEN];
   char command[CMD_MAX_LEN];
}tCTRL_CMD_STR;

static int g_ap_timeout_flag = 0;
static pthread_mutex_t func_mutex  = PTHREAD_MUTEX_INITIALIZER;

static const char *cmd_fail_reason[] = 
{
  "NONE",
  "WIFI_NOT_CONNECT",
  "WIFI_NOT_FOUND",
  "WIFI_WRONG_KEY",
  "WIFI_TIMEOUT",
  "WIFI_CFGNET_TIMEOUT",
  "NETWORK_OUT_OF_CAPACITY",
  "WIFI_WPA_SUPLICANT_ENABLE_FAIL",
  "WIFI_HOSTAPD_ENABLE_FAIL",
  "COMMAND_FORMAT_ERROR",
  "COMMAND_RESPOND_ERROR",
  "WIFI_MONITOR_ENABLE_FAIL",
  "WIFI_SSID_PSK_SET_FAIL"
};

static int netmanger_response_command(flora_call_reply_t reply, int state,int reason) 
{
    char *result = NULL;
    char *respond = NULL;
    json_object *obj = json_object_new_object();
    
    switch (state) 
    {
      case COMMAND_OK:
        result = "OK";
        break;
      case COMMAND_NOK:
        result = "NOK";
        break;
      default:
        break;
    }

    json_object_object_add(obj, "result", json_object_new_string(result));

   if(state == COMMAND_NOK)
      json_object_object_add(obj, "reason", json_object_new_string(cmd_fail_reason[reason]));

    respond = (char *)json_object_to_json_string(obj);
    network_ipc_return(reply,(uint8_t *)respond);
    json_object_put(obj);

    return 0;
}


int get_wifi_capacity(void)
{
  int capacity = 0;
  if(wifi_hal_get_capacity() == 0)
     capacity |= NET_CAPACITY_WIFI;
 
   return capacity;
}

int get_modem_capacity(void)
{
  int capacity = 0;

  if(modem_hal_get_capacity() == 0)
     capacity |= NET_CAPACITY_MODEM;
 
   return capacity;
}

int get_etherent_capacity(void)
{
  int capacity = 0;

  if(etherent_hal_get_capacity() == 0)
     capacity |= NET_CAPACITY_ETHERENT;
 
   return capacity;
}

static int netmanager_get_net_capacity(flora_call_reply_t reply,void *data)
{
    const char * respond;
    uint32_t capacity;

    json_object *root = json_object_new_object();
    json_object *capacity_array  = json_object_new_array();

    NM_LOGI("%s->%d\n",__func__,__LINE__);

    capacity = get_network_capacity();

    json_object_object_add(root, "result", json_object_new_string("OK"));

    if(capacity& NET_CAPACITY_WIFI)
    {
       json_object_array_add(capacity_array, json_object_new_string(NET_WIFI_S));
    }

    if(capacity & NET_CAPACITY_MODEM)
    {
       json_object_array_add(capacity_array, json_object_new_string(NET_MODEM_S));
    }

    if(capacity &NET_CAPACITY_ETHERENT)
    {
       json_object_array_add(capacity_array, json_object_new_string(NET_ETHERENT_S));
    }

    json_object_object_add(root,"net_capacities",capacity_array);
     
    respond = (char *)json_object_to_json_string(root);

    network_ipc_return(reply,(uint8_t *)respond);

//    json_object_put(capacity_array);
    json_object_put(root);

    return 0;
}

static int netmanager_get_net_mode(flora_call_reply_t reply,void *data)
{
    const char * respond;
    uint32_t mode;

    json_object *root = json_object_new_object();
    json_object *mode_array  = json_object_new_array();

    NM_LOGI("%s->%d\n",__func__,__LINE__);

    mode = get_network_mode();

    json_object_object_add(root, "result", json_object_new_string("OK"));

    if(mode & WIFI)
    {
       json_object_array_add(mode_array, json_object_new_string(NET_WIFI_S));
    }

    if(mode & WIFI_AP)
    {
       json_object_array_add(mode_array, json_object_new_string(NET_WIFI_AP_S));
    }

    if(mode & MODEM)
    {
       json_object_array_add(mode_array, json_object_new_string(NET_MODEM_S));
    }

    if(mode & ETHERENT)
    {
       json_object_array_add(mode_array, json_object_new_string(NET_ETHERENT_S));
    }

    json_object_object_add(root,"net_modes",mode_array);
     
    respond = (char *)json_object_to_json_string(root);

    network_ipc_return(reply,(uint8_t *)respond);

//    json_object_put(mode_array);
    json_object_put(root);

    return 0;
}


static int netmanager_report_all_status(flora_call_reply_t reply,void *data)
{
   int reason = eREASON_NONE;
   int state = COMMAND_OK;
   int mode = get_network_mode(); 
    
   monitor_report_status();   
  
   if(mode&WIFI) 
     wifi_report_status();

   if(mode&ETHERENT) 
     etherent_report_status();
  
   if(mode & MODEM)
     modem_report_status();

   netmanger_response_command(reply,state,reason); 

   return 0; 
}



int get_wifi_station_info(json_object *base,tNETWORK_MODE_INFO *mode)
{
   const char *str;
   json_object  *obj,*elem;

   NM_LOGI("%s->%d\n",__func__,__LINE__);
   if (json_object_object_get_ex(base,NET_PARMS_KEY,&obj) != TRUE) 
   {
     NM_LOGE("%s error\n",NET_PARMS_KEY);
     return -1;
   }

   if (json_object_object_get_ex(obj,WIFI_SSID_KEY, &elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_SSID_KEY);
      return -1;
   }

   if((str=json_object_get_string(elem)) == NULL)
   {
      NM_LOGE("SSID:%s error\n",WIFI_SSID_KEY);
     return -1;
   }

    strncpy(mode->ssid ,json_object_get_string(elem),MAX_SIZE-1);

   if (json_object_object_get_ex(obj,WIFI_PASSWD_KEY,&elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_PASSWD_KEY);
      return -1;
   }

   if((str= json_object_get_string(elem)) == NULL)
       strncpy(mode->psk,"",MAX_SIZE-1);
   else
     strncpy(mode->psk,json_object_get_string(elem),MAX_SIZE-1);

   return 0;
}

int  get_wifi_ap_info(json_object *base,tNETWORK_MODE_INFO*mode)
{
   json_object  *obj,*elem;
   
   if (json_object_object_get_ex(base,NET_PARMS_KEY,&obj) != TRUE) 
   {
      NM_LOGE("%s error\n",NET_PARMS_KEY);
      return -1;
   }

   if (json_object_object_get_ex(obj,WIFI_SSID_KEY, &elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_SSID_KEY);
      return -1;
   }
    strncpy(mode->ssid ,json_object_get_string(elem),MAX_SIZE-1);

   if (json_object_object_get_ex(obj,WIFI_PASSWD_KEY,&elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_PASSWD_KEY);
      return -1;
   }
   strncpy(mode->psk ,json_object_get_string(elem),MAX_SIZE-1);

   if (json_object_object_get_ex(obj,WIFI_AP_IP_KEY,&elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_AP_IP_KEY);
      return -1;
   }

   strncpy(mode->ip ,json_object_get_string(elem),MAX_SIZE-1);

   if (json_object_object_get_ex(obj,WIFI_AP_TIMEOUT,&elem) != TRUE) 
   {
      NM_LOGE("%s error\n",WIFI_AP_TIMEOUT);
      return -1;
   }

   NM_LOGW("p_info->timeout:%s\n",json_object_get_string(elem));
   mode->timeout = strtol(json_object_get_string(elem),NULL,0);
   NM_LOGW("p_info->timeout:%d\n",mode->timeout);

   return 0;
}

static int netmanager_start_wifi_scan(flora_call_reply_t reply,void *data)
{
  struct network_service *p_net = get_network_service();

  if(get_network_mode() & WIFI_AP)
  {
    network_configure_net(WIFI_AP,STOP,NULL);
    network_set_mode(WIFI_AP,STOP,NULL);
    p_net->wifi_state = WIFI_AP_CLOSE; 
  }

  wifi_hal_start_station(); 
  wifi_clear_scan_results();

// ?? need delete
//wifi_disable_all_network();

  eloop_timer_stop(p_net->wifi_scan);
  eloop_timer_start(p_net->wifi_scan);

  p_net->wifi_state = WIFI_STATION_PRE_JOIN;

  netmanger_response_command(reply,COMMAND_OK,eREASON_NONE); 
  return 0 ;
}

static int netmanager_stop_wifi_scan(flora_call_reply_t reply,void *data)
{
   struct network_service *p_net = get_network_service();

   NM_LOGI("%s->%d\n",__func__,__LINE__);
   eloop_timer_stop(p_net->wifi_scan);
   netmanger_response_command(reply,COMMAND_OK,eREASON_NONE); 
   return 0;
}

static int netmanager_start_wifi_station(flora_call_reply_t reply,void *data)
{
    int iret = -1;
    int reason = eREASON_NONE;
    int state = COMMAND_OK;
    json_object * base  = (json_object *)data;
    tNETWORK_MODE_INFO info,*p_info = &info;
    struct network_service *p_net = get_network_service();

    NM_LOGI("%s->%d\n",__func__,__LINE__);
    memset(p_info,0,sizeof(tNETWORK_MODE_INFO));

   if(get_network_mode() & WIFI_AP)
   {

     if(g_ap_timeout_flag == 1)
      {
        eloop_timer_stop(p_net->wifi_ap_timeout);
        eloop_timer_delete(p_net->wifi_ap_timeout);
         g_ap_timeout_flag  =  0;
      }

     network_configure_net(WIFI_AP,STOP,NULL);
     network_set_mode(WIFI_AP,STOP,NULL);
     p_net->wifi_state = WIFI_AP_CLOSE;
   }
/*
  if(get_network_mode() & WIFI)
  {
      network_configure_net(WIFI,STOP,NULL);
      network_set_mode(WIFI,STOP,NULL);
      p_net->wifi_state = WIFI_STATE_CLOSE;
  }
*/
 
   memset(p_info,0,sizeof(tNETWORK_MODE_INFO));

    p_net->wifi_state = WIFI_STATION_JOINING;

    if(get_wifi_station_info(base,p_info))
    {
       state = COMMAND_NOK;
       reason = eREASON_COMMAND_FORMAT_ERROR;
       NM_LOGE("get_wifi_station_info() fail\n");
       iret =-1;
       goto ret;
    }

    if(network_configure_net(WIFI,START,p_info))
    {
       reason  = p_info->reason;
       state  = COMMAND_NOK;

    //   wifi_enable_all_network();
       p_net->wifi_state = WIFI_STATION_UNCONNECTED;
    }
    else
    {
      struct wifi_network *p_wifi =  get_wifi_network();

      network_set_mode(WIFI,START,NULL);
        /* ADD BY SJM  need ????*/
      eloop_timer_stop(p_net->wifi_scan);
      p_net->wifi_state = WIFI_STATION_CONNECTED;
     
      p_net->wpa_network_id = p_wifi->id;
     
      if(p_net->wpa_network_id >=0)
        strncpy(p_net->ssid,p_wifi->ssid,36);
   //   strncpy(p_net->psk,p_wifi->psk,68);

      NM_LOGW("----ssid:%s,id:%d connect----\n",p_net->ssid,p_net->wpa_network_id);

    }

    /* ADD BY SJM  need ????*/
      wifi_enable_all_network();
ret:
   netmanger_response_command(reply,state,reason); 
   return iret;
}


static int netmanager_stop_wifi_station(flora_call_reply_t  reply,void *data)
{
    struct network_service *p_net = get_network_service();

    NM_LOGI("%s->%d\n",__func__,__LINE__);

    p_net->wifi_state = WIFI_STATE_CLOSE;

    eloop_timer_stop(p_net->wifi_scan);
    network_configure_net(WIFI,STOP,NULL);

    network_set_mode(WIFI,STOP,NULL);

    netmanger_response_command(reply,COMMAND_OK,eREASON_NONE); 

    return 0;
}

static int netmanager_get_wifi_scan_result(flora_call_reply_t reply,void *data) 
{
    struct wifi_scan_list list;
    json_object *root = json_object_new_object();
    json_object *wifi_list = json_object_new_array();
    json_object *item[32];
    int i = 0;
    char *buf = NULL;
    
    NM_LOGI("%s->%d\n",__func__,__LINE__);
    memset(&list, 0, sizeof(struct wifi_scan_list));

    json_object_object_add(root, "result", json_object_new_string("OK"));

    wifi_get_scan_result(&list);

   for (i = 0; i < list.num; i++) 
   {
        item[i] = json_object_new_object();

        json_object_object_add(item[i], "SSID", json_object_new_string(list.ssid[i].ssid));
        json_object_object_add(item[i], "SIGNAL",json_object_new_int(list.ssid[i].sig));

        json_object_array_add(wifi_list, item[i]);
    }

    json_object_object_add(root, "wifilist", wifi_list);

    buf = (char *)json_object_to_json_string(root);

    network_ipc_return(reply,(uint8_t *)buf);

    for (i = 0; i < list.num; i++) 
    {
        json_object_put(item[i]);
    }

//    json_object_put(wifi_list);
    json_object_put(root);

    return 0;
}

static void wifi_change_to_station(void *eloop_ctx) 
{
   struct network_service *p_net = get_network_service();

    pthread_mutex_lock(&func_mutex);
    NM_LOGW("wifi change to station");

    p_net->wifi_state = WIFI_AP_CLOSE;
    network_configure_net(WIFI_AP,STOP,NULL);
    network_set_mode(WIFI_AP,STOP,NULL);

    wifi_hal_start_station(); 
    wifi_start_monitor();

    eloop_timer_stop(p_net->wifi_ap_timeout);
    eloop_timer_delete(p_net->wifi_ap_timeout);
    g_ap_timeout_flag =0; 
    pthread_mutex_unlock(&func_mutex);

}

static int netmanager_start_wifi_ap(flora_call_reply_t reply,void *data)
{
    int iret = 0;
    int reason = eREASON_NONE;
    int state = COMMAND_OK;
    struct network_service *p_net = get_network_service();
    uint32_t mode = get_network_mode(); 

    json_object * base  = (json_object *)data;
    tNETWORK_MODE_INFO info, *p_info = &info;

    NM_LOGI("%s->%d\n",__func__,__LINE__);
    eloop_timer_stop(p_net->wifi_scan);


   // MOD BY SJM @2019.2.2,to stop wpa_supplicant,start by wifi scan,and get config 
   //  if(mode & WIFI)
   //  {
      network_configure_net(WIFI,STOP,NULL);
      network_set_mode(WIFI,STOP,NULL);
      p_net->wifi_state = WIFI_STATE_CLOSE;
   //  }
  
     if(mode & WIFI_AP)
     {
      network_configure_net(WIFI_AP,STOP,NULL);
      network_set_mode(WIFI_AP,STOP,NULL);
      p_net->wifi_state = WIFI_AP_CLOSE;

      if(g_ap_timeout_flag){
        eloop_timer_stop(p_net->wifi_ap_timeout);
        eloop_timer_delete(p_net->wifi_ap_timeout);
        g_ap_timeout_flag = 0;
      }
     }

    memset(p_info,0,sizeof(tNETWORK_MODE_INFO));

    if(get_wifi_ap_info(base,p_info))
    {
       state = COMMAND_NOK;
       reason = eREASON_COMMAND_FORMAT_ERROR;
       NM_LOGE("get_wifi_ap_info()\n");
       iret =-1;
       goto ret;
    }

//   sleep(1);
   if(network_configure_net(WIFI_AP,START,p_info))
   {
     reason  = p_info->reason;
     state  = COMMAND_NOK;
     p_net->wifi_state = WIFI_AP_UNCONNECTED;
   }
   else
   {
     p_net->wifi_state = WIFI_AP_CONNECTED;
     memcpy(&g_ap_info,p_info,sizeof(tNETWORK_MODE_INFO));
     network_set_mode(WIFI_AP,START,p_info);
   }

  NM_LOGW("p_info->timeout:%d\n",p_info->timeout);
  if(p_info->timeout > 0 && g_ap_timeout_flag  ==0)
  {
    NM_LOGW("eloop_timer_add \n");
    p_net->wifi_ap_timeout = eloop_timer_add(wifi_change_to_station,NULL,p_info->timeout,0);
    eloop_timer_start(p_net->wifi_ap_timeout);
    g_ap_timeout_flag = 1;
  }
   
ret:
   netmanger_response_command(reply,state,reason); 
   return iret;
}

static int netmanager_stop_wifi_ap(flora_call_reply_t reply,void *data)
{
   int reason = eREASON_NONE;
   int state = COMMAND_OK;
   struct network_service *p_net = get_network_service();

    NM_LOGI("%s->%d\n",__func__,__LINE__);

    p_net->wifi_state = WIFI_AP_CLOSE;

    network_configure_net(WIFI_AP,STOP,NULL);
    network_set_mode(WIFI_AP,STOP,NULL);
   if(g_ap_timeout_flag){
     eloop_timer_stop(p_net->wifi_ap_timeout);
     eloop_timer_delete(p_net->wifi_ap_timeout);
     g_ap_timeout_flag = 0;
   }

    netmanger_response_command(reply,state,reason); 

    return  0;
}

static int netmanager_start_modem(flora_call_reply_t reply,void *data)
{
   int flag=0;
   int reason = eREASON_NONE;
   int state = COMMAND_OK;
   struct network_service *p_net = get_network_service();
   tNETWORK_MODE_INFO info, *p_info = &info;
   uint32_t mode = get_network_mode();

   
   NM_LOGI("%s->%d\n",__func__,__LINE__);

   if( mode & WIFI_AP)
   {
      flag = 1;
      network_configure_net(WIFI_AP,STOP,NULL);
   //   network_set_mode(WIFI_AP,STOP,NULL);
      p_net->wifi_state = WIFI_AP_CLOSE;
   }


   if(mode  & MODEM)
   {
      network_configure_net(MODEM,STOP,NULL);
      network_set_mode(MODEM,STOP,NULL);
      p_net->modem_state = MODEM_UNCONNECTED; 
   }

   if(network_configure_net(MODEM,START,p_info))
   {
      state   = COMMAND_NOK;
      reason = p_info->reason;
      p_net->modem_state  = MODEM_UNCONNECTED;
   }
   else
   {
    p_net->modem_state  = MODEM_CONNECTED;
    network_set_mode(MODEM,START,NULL);
   }

   if(flag ==1)
   {
      if(network_configure_net(WIFI_AP,START,&g_ap_info))
      {
         state   = COMMAND_NOK;
         reason = g_ap_info.reason;
         network_set_mode(WIFI_AP,STOP,NULL);
      } 
      else 
        p_net->wifi_state = WIFI_AP_CONNECTED;
        
      flag = 0;
   }

   netmanger_response_command(reply,state,reason); 

   return 0;
}

static int netmanager_stop_modem(flora_call_reply_t reply,void *data)
{
   int reason = eREASON_NONE;
   int state = COMMAND_OK;
   struct network_service *p_net = get_network_service();

   NM_LOGI("%s->%d\n",__func__,__LINE__);

   network_configure_net(MODEM,STOP,NULL);
  
   p_net->modem_state  = MODEM_UNCONNECTED;
   network_set_mode(MODEM,STOP,NULL);

   netmanger_response_command(reply,state,reason); 

   return 0;
}

static int netmanager_start_etherent(flora_call_reply_t reply,void *data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  tNETWORK_MODE_INFO info, *p_info = &info;
  struct network_service *p_net = get_network_service();

  NM_LOGI("%s->%d\n",__func__,__LINE__);

   if(get_network_mode() & ETHERENT)
   {
      network_configure_net(ETHERENT,STOP,NULL);
      network_set_mode(ETHERENT,STOP,NULL);
      p_net->etherent_state =  ETHERENT_UNCONNECTED;
   }

  if(network_configure_net(ETHERENT,START,p_info))
  {
      state   = COMMAND_NOK;
      reason = p_info->reason;
  }
  else
  {
    p_net->etherent_state =  ETHERENT_CONNECTED;
    network_set_mode(ETHERENT,START,NULL);
  }

  netmanger_response_command(reply,state,reason); 

  return 0;
}

static int netmanager_stop_etherent(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  struct network_service *p_net = get_network_service();

  NM_LOGI("%s->%d\n",__func__,__LINE__);

  network_set_mode(ETHERENT,STOP,NULL);
  network_configure_net(ETHERENT,STOP,NULL);
  p_net->etherent_state =  ETHERENT_UNCONNECTED;
  netmanger_response_command(reply,state,reason); 

  return 0;
}

static int netmanager_get_network_status(flora_call_reply_t reply,void* data)
{
   return  monitor_respond_status(reply);
}

static int netmanager_get_wifi_status(flora_call_reply_t reply,void* data)
{
   return  wifi_respond_status(reply);
}

static int netmanager_get_modem_status(flora_call_reply_t reply,void* data)
{

  return modem_respond_status(reply);
}

static int netmanager_get_etherent_status(flora_call_reply_t reply,void* data)
{
  return etherent_respond_status(reply);
}

static int netmanager_get_config_network(flora_call_reply_t reply,void* data)
{
    int num =0;
    uint32_t mode;
    uint32_t capacity;
    
    json_object *root = json_object_new_object();

    char *buf = NULL;
    
    NM_LOGI("%s->%d\n",__func__,__LINE__);

    #if 0 
    capacity = get_network_capacity();
    mode =  get_network_mode();

    json_object_object_add(root, "result", json_object_new_string("OK"));

    if((capacity& NET_CAPACITY_WIFI) &&  !(mode & WIFI_AP))
    {
       wifi_hal_start_station(); 

      if(wifi_get_config_network(&num))
      {
        num =0;
        NM_LOGW("wifi get_config_num fail\n");
      }
    }
    #endif
   
    if(is_wifi_configured()){
     num = 1;
    }
    NM_LOGI("wifi get_config_num:%d\n",num);
    
    json_object_object_add(root, "wifi_config", json_object_new_int(num));

    buf = (char *)json_object_to_json_string(root);

    network_ipc_return(reply,(uint8_t *)buf);

    json_object_put(root);
  
    return 0;
}

static int netmanager_save_config_network(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  NM_LOGI("%s->%d\n",__func__,__LINE__);

  if(wifi_save_network())
  {
     state = COMMAND_NOK;
     reason = eREASON_COMMAND_RESPOND_ERROR;
  }

  netmanger_response_command(reply,state,reason); 

  return 0;
}


static char wifi_reply[4096] = {0};
//static int reply_len = sizeof(wifi_reply);

static int get_line_from_buf(int index, char *line, char *buf) 
{
   int i = 0;
   int j;
   int endcnt = -1;
   char *linestart = buf;
   int len;

   while (1) 
   {
       if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0') {
           endcnt++;
           if (index == endcnt) {
               len = &buf[i] - linestart;
               strncpy(line, linestart, len);
               line[len] = '\0';
               return 0;
           } 
           else {
               for (j = i + 1; buf[j]; ) 
               {
                   if (buf[j] == '\n' || buf[j] == '\r')
                       j++;
                   else
                       break;
               }
               if (!buf[j])
                   return -1;
               linestart = &buf[j];
               i = j;
           }
       }

       if (!buf[i])
           return -1;
       i++;
   }
 }

static int wpa_sync_send_command(char *command,char *reply,size_t *len)
{
     int ret=0;
     struct wpa_ctrl *wifi_conn = NULL;

   //  NM_LOGI("WIFI_WPA_CTRL_PATH:: %s \n",WIFI_WPA_CTRL_PATH);

     wifi_conn = wpa_ctrl_open(WIFI_WPA_CTRL_PATH);

     if (wifi_conn == NULL) {
         NM_LOGE("wifi open error\n");
         return -1;
     }

   //  NM_LOGI("wifi request %s \n", command);

     ret = wpa_ctrl_request(wifi_conn, command, strlen(command), reply,len, NULL);

   //  NM_LOGI("wifi respond %s ,ret:%d\n",reply,ret);
     if (ret < 0) 
     {
         NM_LOGE("wifi request error %s \n", command);
     }
 
     wpa_ctrl_close(wifi_conn);

    return ret;
}

static int netmanager_remove_network(flora_call_reply_t reply,void* data)
{
  int i;
  int iret=0;
  size_t len=100;
  char command[64]={0};
  char line[512] = {0};
  int flag = 0;
  struct network_service *p_net = get_network_service();

  int reason = eREASON_NONE;
  int state = COMMAND_OK;

  json_object * base  = (json_object *)data;
  tNETWORK_MODE_INFO info,*p_info = &info;
  uint32_t mode = get_network_mode();
// struct network_service *p_net = get_network_service();

  NM_LOGI("%s->%d\n",__func__,__LINE__);
/** support for used when station is not open **/ 

 if(mode & WIFI_AP)
  {
      network_configure_net(WIFI_AP,STOP,NULL);
      network_set_mode(WIFI_AP,STOP,NULL);
      p_net->wifi_state = WIFI_AP_CLOSE;

     if(g_ap_timeout_flag == 1)
      {
        eloop_timer_stop(p_net->wifi_ap_timeout);
        eloop_timer_delete(p_net->wifi_ap_timeout);
         g_ap_timeout_flag  =  0;
      }
  }

//  wifi_hal_stop_station(); 
  wifi_hal_start_station(); 



 if(get_wifi_station_info(base,p_info))
 {
       state = COMMAND_NOK;
       reason = eREASON_COMMAND_FORMAT_ERROR;
       NM_LOGE("get_wifi_station_info() fail\n");
       iret =-1;
       goto ret;
  }


 // NM_LOGI("%s->%d,ssid:%s\n",__func__,__LINE__,p_info->ssid);

  memset(command,0,64);
  snprintf(command,64,"%s", "LIST_NETWORKS");

  memset(wifi_reply,0,4096);

  if(wpa_sync_send_command(command,wifi_reply,&len)==0)
  {
     memset(line,0,512);

    NM_LOGI("%s->%d,wifi_reply:%s,reply_len:%d\n",__func__,__LINE__,wifi_reply,len);

    for (i = 1; !get_line_from_buf(i, line, wifi_reply); i++) 
    {
        char *c;
        NM_LOGI("line%d:%s\n",i,line); 

       if((c = strstr(line,p_info->ssid)) != NULL && *(c+strlen(p_info->ssid)) == '\t')
       {
         int id = atoi(line);
         NM_LOGI("#######remove id:%d\n",id); 
         wifi_remove_network(&id);
         flag = 1;
     //  wifi_save_network();
     //    break;
       }

    }

   if(flag)
   {
     flag = 0;
     wifi_save_network();
   }

  }
  else
  {
        NM_LOGE("wpa_sync_send_command error\n"); 
       state = COMMAND_NOK;
       reason = eREASON_COMMAND_FORMAT_ERROR;

  }
/*
   p_net->wifi_state = WIFI_STATE_CLOSE;
   eloop_timer_stop(p_net->wifi_scan);
   network_configure_net(WIFI,STOP,NULL);
   network_set_mode(WIFI,STOP,NULL);
*/

ret:
  netmanger_response_command(reply,state,reason); 
  return iret;
}


static int netmanager_enable_all_network(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;

  NM_LOGI("%s->%d\n",__func__,__LINE__);

  if(wifi_enable_all_network())
  {
     state = COMMAND_NOK;
     reason = eREASON_COMMAND_RESPOND_ERROR;
  }

  netmanger_response_command(reply,state,reason); 
  return 0;
}

static int netmanager_disable_all_network(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  NM_LOGI("%s->%d\n",__func__,__LINE__);

  if(wifi_disable_all_network())
  {
     state = COMMAND_NOK;
     reason = eREASON_COMMAND_RESPOND_ERROR;
  }

  netmanger_response_command(reply,state,reason); 

  return 0;
}

static int netmanager_reset_dns(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  NM_LOGI("%s->%d\n",__func__,__LINE__);

  if(res_init())
  {
     state = COMMAND_NOK;
     reason = eREASON_COMMAND_RESPOND_ERROR;
  }

  netmanger_response_command(reply,state,reason); 

  return 0;
}

static int netmanager_remove_all_network(flora_call_reply_t reply,void* data)
{
  int reason = eREASON_NONE;
  int state = COMMAND_OK;
  NM_LOGI("%s->%d\n",__func__,__LINE__);

  if(wifi_remove_all_network())
  {
     state = COMMAND_NOK;
     reason = eREASON_COMMAND_RESPOND_ERROR;
  }

  if(state == COMMAND_OK)
    wifi_save_network();

  netmanger_response_command(reply,state,reason); 

  return 0;
}


static tCTRL_CMD_FMT ctrl_cmd[] = 
{
   {"NETWORK"     ,"GET_CAPACITY"  ,netmanager_get_net_capacity},
   {"NETWORK"     ,"GET_MODE"      ,netmanager_get_net_mode},
   {"NETWORK"     ,"TRIGGER_STATUS",netmanager_report_all_status},
   {"WIFI"        ,"START_SCAN"  ,netmanager_start_wifi_scan},
   {"WIFI"        ,"STOP_SCAN"   ,netmanager_stop_wifi_scan},
   {"WIFI"        ,"GET_WIFILIST",netmanager_get_wifi_scan_result},
   {"WIFI"        ,"CONNECT"     ,netmanager_start_wifi_station},
   {"WIFI"        ,"DISCONNECT"  ,netmanager_stop_wifi_station},
   {"WIFI_AP"     ,"CONNECT"     ,netmanager_start_wifi_ap},
   {"WIFI_AP"     ,"DISCONNECT"  ,netmanager_stop_wifi_ap}, 
   {"MODEM"       ,"CONNECT"     ,netmanager_start_modem},
   {"MODEM"       ,"DISCONNECT"  ,netmanager_stop_modem},
   {"ETHERENT"    ,"CONNECT"     ,netmanager_start_etherent},
   {"ETHERENT"    ,"DISCONNECT"  ,netmanager_stop_etherent},
   {"NETWORK"     ,"GET_STATUS"  ,netmanager_get_network_status},  
   {"WIFI"        ,"GET_STATUS"  ,netmanager_get_wifi_status},
   {"ETHERENT"    ,"GET_STATUS"  ,netmanager_get_etherent_status},
   {"MODEM"       ,"GET_STATUS"  ,netmanager_get_modem_status},
   {"WIFI"       ,"GET_CFG_NUM"  ,netmanager_get_config_network},
   {"WIFI"       ,"SAVE_CFG"     ,netmanager_save_config_network},
   {"WIFI"       ,"REMOVE"       ,netmanager_remove_network},
   {"WIFI"       ,"ENABLE"       ,netmanager_enable_all_network},
   {"WIFI"       ,"DISABLE"      ,netmanager_disable_all_network},
   {"NETWORK"    ,"RESET_DNS"    ,netmanager_reset_dns},
   {"WIFI"       ,"REMOVE_ALL"   ,netmanager_remove_all_network},
};

static int get_cmd_str(json_object *base,tCTRL_CMD_STR *cmd)
{
    int iret = -1;
    json_object *device = NULL;
    json_object *command = NULL;

    if(cmd == NULL)
     return iret;

    if (json_object_object_get_ex(base,CMD_DEVICE_KEY, &device) == TRUE && 
        json_object_object_get_ex(base,CMD_COMMAND_KEY, &command) == TRUE)
    {
        strncpy(cmd->device,json_object_get_string(device),CMD_MAX_LEN-1);
        strncpy(cmd->command,json_object_get_string(command),CMD_MAX_LEN-1);
        iret  = 0;
    }

    return iret;
}

static int get_ctrl_cmd_idx(tCTRL_CMD_STR*cmd,int *idx)
{

  int iret = 0;
  int i,cnts;
 
  tCTRL_CMD_FMT *p_cmd;

  cnts  = sizeof(ctrl_cmd)/sizeof(ctrl_cmd[0]);

  for(i=0;i<cnts;i++)
  {
     p_cmd = &ctrl_cmd[i];

     if(strcmp(p_cmd->device,cmd->device) == 0 && 
        strcmp(p_cmd->command,cmd->command) == 0)
      {
          *idx  = i;
          break;
      }

  }

  if(i >= cnts)
   iret = -1;

  return iret;
}

void handle_net_command(flora_call_reply_t reply,const char *buf)
{
  int idx;
  uint32_t reason = eREASON_COMMAND_FORMAT_ERROR;

  json_object *obj = NULL;
  tCTRL_CMD_STR cmd;
  tCTRL_CMD_FMT *p_cmd;

  memset(&cmd,0,sizeof(tCTRL_CMD_STR));  
  obj = json_tokener_parse(buf);
    
  if(get_cmd_str(obj,&cmd))
  {
    NM_LOGE("get_cmd_str fail:%s\n",buf);
    netmanger_response_command(reply,COMMAND_NOK,reason); 
    goto iret ;
  }

  if(get_ctrl_cmd_idx(&cmd,&idx))
  {
    NM_LOGE("device:%s,command:%s is not support\n",
       cmd.device,cmd.command);
    netmanger_response_command(reply,COMMAND_NOK,reason); 
    goto iret;
  }

  p_cmd = &ctrl_cmd[idx];

  pthread_mutex_lock(&func_mutex);
  p_cmd->respond_func(reply,(void *)obj);
  pthread_mutex_unlock(&func_mutex);
iret:
  json_object_put(obj);
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <json-c/json.h>
#include <ipc.h>
#include <common.h>
#include <network_report.h>
#include <hardware/modem_tty.h>

#define REPORT_WIFI_MAIN_KEY     "wifi"
#define REPORT_WIFI_STATE_KEY    "state"
#define REPORT_WIFI_SIGNAL_KEY    "signal"
#define REPORT_MODEM_MAIN_KEY     "modem"
#define REPORT_MODEM_STATE_KEY    "state"
#define REPORT_MODEM_TYPE_KEY     "type"
#define REPORT_MODEM_SIGNAL_KEY   "signal"
#define REPORT_ETHERENT_MAIN_KEY  "etherent"
#define REPORT_ETHERENT_STATE_KEY "state"
#define REPORT_NETWORK_MAIN_KEY  "network"
#define REPORT_NETWORK_STATE_KEY "state"

static char *net_status[]= 
{
 "DISCONNECTED",
 "CONNECTED"
};

int report_wifi_status(tWIFI_STATUS *p_status)
{

  char str[64] = {0}; 
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status) /*|| 
   p_status->signal >= sizeof(signal_strength)*/){
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
 }
 json_object_object_add(elem, REPORT_WIFI_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 snprintf(str,64,"%d",p_status->signal);
 json_object_object_add(elem, REPORT_WIFI_SIGNAL_KEY, json_object_new_string(str));
 json_object_object_add(root,REPORT_WIFI_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

// NM_LOGI("report wifi status:%s\n",status);

 network_ipc_upload((uint8_t *)status); 

// json_object_put(elem);
 json_object_put(root);

 return 0; 
}

typedef struct val2str
{
   uint32_t val;
   char*   str;
}tVAL2STR;

static tVAL2STR signal_mode_map[] = 
{
  {eSIGNAL_SERVICE_NONE,    "none" }, 
  {eSIGNAL_SERVICE_2G,       "2G"  },
  {eSIGNAL_SERVICE_3G,       "3G"  },
  {eSIGNAL_SERVICE_4G,       "4G"  }
};


static int find_mode_string(uint32_t val,char **str)
{
   int i,iret = -1;

  for(i=0;i<sizeof(signal_mode_map)/sizeof(signal_mode_map[0]);i++)
  {
      if(val == signal_mode_map[i].val)
      {
         iret  = 0;   
         *str = signal_mode_map[i].str;
      }
  }
  return iret;
}

int report_modem_status(tMODEM_STATUS *p_status)
{
  const char *status = NULL;
  char *type/*,*strength*/;

  char strength[64]= {0};
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
  }
  
  json_object_object_add(elem, REPORT_MODEM_STATE_KEY,json_object_new_string(net_status[p_status->state]));


  if(find_mode_string(p_status->type,&type) == 0)
  {
    json_object_object_add(elem, REPORT_MODEM_TYPE_KEY,json_object_new_string(type));
  }

  snprintf(strength,64,"%d",p_status->signal);


  json_object_object_add(elem, REPORT_MODEM_SIGNAL_KEY,json_object_new_string(strength));

  json_object_object_add(root, REPORT_MODEM_MAIN_KEY,elem);

  status = json_object_to_json_string(root);

 // NM_LOGI("report modem status:%s\n",status);

  network_ipc_upload((uint8_t *)status); 

//  json_object_put(elem);
  json_object_put(root);

  return 0;
} 

int report_etherent_status(tETHERENT_STATUS *p_status)
{
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
  }
  
 json_object_object_add(elem, REPORT_ETHERENT_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 json_object_object_add(root,REPORT_ETHERENT_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

// NM_LOGI("report etherent status:%s\n",status);

 network_ipc_upload((uint8_t *)status); 

// json_object_put(elem);
 json_object_put(root);

 return  0;
}

int report_network_status(tNETWORK_STATUS *p_status)
{
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
  }
  
 json_object_object_add(elem,REPORT_NETWORK_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 json_object_object_add(root,REPORT_NETWORK_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

 //NM_LOGI("report network status:%s\n",status);

 network_ipc_upload((uint8_t *)status); 

// json_object_put(elem);
 json_object_put(root);

 return  0;
}


///////////////////////////////////////////////////////////////////////////////////////////

int respond_wifi_status(flora_call_reply_t reply,tWIFI_STATUS *p_status)
{

  char str[64] = {0}; 
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status) /*|| 
   p_status->signal >= sizeof(signal_strength)*/){
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
 }

 json_object_object_add(root, "result", json_object_new_string("OK"));
 json_object_object_add(elem, REPORT_WIFI_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 snprintf(str,64,"%d",p_status->signal);
 json_object_object_add(elem, REPORT_WIFI_SIGNAL_KEY, json_object_new_string(str));
 json_object_object_add(root,REPORT_WIFI_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

// NM_LOGI("report wifi status:%s\n",status);

 network_ipc_return(reply,(uint8_t *)status); 

// json_object_put(elem);
 json_object_put(root);

 return 0; 
}

int respond_modem_status(flora_call_reply_t reply,tMODEM_STATUS *p_status)
{
  const char *status = NULL;
  char *type/*,*strength*/;

  char strength[64]= {0};
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);

     return -1;
  }
  
  json_object_object_add(root, "result", json_object_new_string("OK"));

  json_object_object_add(elem, REPORT_MODEM_STATE_KEY,json_object_new_string(net_status[p_status->state]));


  if(find_mode_string(p_status->type,&type) == 0)
  {
    json_object_object_add(elem, REPORT_MODEM_TYPE_KEY,json_object_new_string(type));
  }

  snprintf(strength,64,"%d",p_status->signal);

  json_object_object_add(elem, REPORT_MODEM_SIGNAL_KEY,json_object_new_string(strength));

  json_object_object_add(root, REPORT_MODEM_MAIN_KEY,elem);

  status = json_object_to_json_string(root);

 // NM_LOGI("report modem status:%s\n",status);

  network_ipc_return(reply,(uint8_t *)status); 

//  json_object_put(elem);
  json_object_put(root);

  return 0;
} 

int respond_etherent_status(flora_call_reply_t reply,tETHERENT_STATUS *p_status)
{
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
  }
  
 json_object_object_add(root, "result", json_object_new_string("OK"));
 json_object_object_add(elem, REPORT_ETHERENT_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 json_object_object_add(root,REPORT_ETHERENT_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

 network_ipc_return(reply,(uint8_t *)status); 

// json_object_put(elem);
 json_object_put(root);

 return  0;
}

int respond_network_status(flora_call_reply_t reply,tNETWORK_STATUS *p_status)
{
  const char *status = NULL;
  json_object *root = json_object_new_object();
  json_object *elem = json_object_new_object();

  if(p_status->state >= sizeof(net_status))
  {
     NM_LOGE("wifi report parm error\n");
     json_object_put(elem);
     json_object_put(root);
     return -1;
  }
  
 json_object_object_add(root, "result", json_object_new_string("OK"));
 json_object_object_add(elem,REPORT_NETWORK_STATE_KEY, json_object_new_string(net_status[p_status->state]));
 json_object_object_add(root,REPORT_NETWORK_MAIN_KEY,elem);

 status = json_object_to_json_string(root);

// NM_LOGI("report network status:%s\n",status);

 network_ipc_return(reply,(uint8_t *)status); 
// json_object_put(elem);
 json_object_put(root);

 return  0;
}


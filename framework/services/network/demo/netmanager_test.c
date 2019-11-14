#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include <flora-agent.h>

typedef struct cmd_info
{ 
  int idx;
  char * device;
  char *command;
  char * info;
  int (*send_func)(char *device,char *command);
}tCMD_INFO;

static flora_agent_t agent;

static int app_get_choice(const char *querystring) {
    int neg, value, c, base;
    int count = 0;
    base = 10;
    neg = 1;
    printf("%s => ", querystring);
    value = 0;
    do {
        c = getchar();

        if ((count == 0) && (c == '\n')) {
            return -1;
        }
        count ++;

        if ((c >= '0') && (c <= '9')) {
            value = (value * base) + (c - '0');
        } else if ((c >= 'a') && (c <= 'f')) {
            value = (value * base) + (c - 'a' + 10);
        } else if ((c >= 'A') && (c <= 'F')) {
            value = (value * base) + (c - 'A' + 10);
        } else if (c == '-') {
            neg *= -1;
        } else if (c == 'x') {
            base = 16;
        }

    } while ((c != EOF) && (c != '\n'));

    return value * neg;
}



int net_flora_init(void) 
{
   agent = flora_agent_create();

   flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock");
   flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
   flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

   flora_agent_start(agent, 0);

   return 0;
}
// add new
void net_flora_exit(void)
{
  flora_agent_close(agent);
  flora_agent_delete(agent);
}

int floar_xfer(char *path,caps_t send_msg,char *recv,uint32_t len)
{
   int iret = -1;

 do{

  flora_call_result result;

  if ((iret = flora_agent_call(agent,path,send_msg, "net_manager", &result,50000)) == FLORA_CLI_SUCCESS) 
  {
     const char * buf;
     caps_read_string (result.data,&buf);
     printf("read string:%s,ret_code:%d\n",buf,result.ret_code);

  }
  else
  printf("%s->%d,ret:%d\n",__func__,__LINE__,iret);

}while(0);

 return iret;
}



int wifi_send_func(char *device,char *command)
{

    char ssid[36] = {0};
    char psk[64] =  {0};
    const char *net_command = NULL;
    caps_t msg = caps_create();
    json_object *root = json_object_new_object();
    json_object *wifi_info = json_object_new_object();
 #if 0
    printf("input :: ssid :: \t");
    gets(ssid);

    printf("input :: psk :: \t");
   gets(psk);
#endif

    strcpy(ssid,"ROKID.TC");
    strcpy(psk,"rokidguys");
    printf("input :: ssid :: %s\n", ssid);
    printf("input :: psk :: %s\n", psk);

    json_object_object_add(wifi_info, "SSID", json_object_new_string(ssid));
    json_object_object_add(wifi_info, "PASSWD", json_object_new_string(psk));

    json_object_object_add(root, "device", json_object_new_string(device));
    json_object_object_add(root, "command", json_object_new_string(command));
    json_object_object_add(root, "params", wifi_info);

    net_command = json_object_to_json_string(root);
    printf("send:%s\n",net_command);
    caps_write_string(msg, (const char *)net_command);

    if(floar_xfer("network.command",msg,NULL,0))
    {
      fprintf(stderr,"floar xfer fail\n");
            
    }
    caps_destroy(msg);
//    json_object_put(wifi_info);
    json_object_put(root);
    return 0;
}

int wifi_ap_send_func(char *device,char *command)
{
    char ssid[36] = {0};
    char psk[64] =  {0};
    char ip[64] =  {0};
    char timeout[64] =  {0};
    const char *net_command = NULL;
    caps_t msg = caps_create();
    json_object *root = json_object_new_object();
    json_object *wifi_info = json_object_new_object();
 #if 0
    printf("input :: ssid :: \t");
    gets(ssid);

    printf("input :: psk :: \t");
   gets(psk);
#endif

    strcpy(ssid,"shanjiaming");
    //strcpy(psk,"11112222");
    strcpy(psk,"");
    strcpy(ip,"192.168.10.88");
    snprintf(timeout,64,"120000");

    printf("input :: ssid :: %s\n", ssid);
    printf("input :: psk :: %s\n", psk);
    printf("input :: ip :: %s\n", ip);
    printf("input :: timeout :: %s\n",timeout);

    json_object_object_add(wifi_info, "SSID", json_object_new_string(ssid));
    json_object_object_add(wifi_info, "PASSWD", json_object_new_string(psk));
    json_object_object_add(wifi_info, "IP", json_object_new_string(ip));
    json_object_object_add(wifi_info, "TIMEOUT", json_object_new_string(timeout));

    json_object_object_add(root, "device", json_object_new_string(device));
    json_object_object_add(root, "command", json_object_new_string(command));
    json_object_object_add(root, "params", wifi_info);


    net_command = json_object_to_json_string(root);
    printf("send:%s\n",net_command);
    caps_write_string(msg, (const char *)net_command);

    if(floar_xfer("network.command",msg,NULL,0))
    {
      fprintf(stderr,"floar xfer fail\n");
            
    }
    caps_destroy(msg);
//    json_object_put(wifi_info);
    json_object_put(root);
    return 0;
  return 0;
}


int general_send_func(char *device,char *command)
{

    const char *net_command = NULL;
   caps_t msg = caps_create();
   json_object *root = json_object_new_object();

   json_object_object_add(root, "device", json_object_new_string(device));
   json_object_object_add(root, "command", json_object_new_string(command));

   net_command = json_object_to_json_string(root);

   printf("send:%s\n",net_command);
   caps_write_string(msg, (const char *)net_command);

  if(floar_xfer("network.command",msg,NULL,0))
  {
       fprintf(stderr,"floar xfer fail\n");
  }

  caps_destroy(msg);
  json_object_put(root);

  return 0 ;
}

tCMD_INFO g_cmd_list[] = 
{
   {0,"NETWORK","GET_CAPACITY","network get capacity",general_send_func},
   {1,"NETWORK","GET_MODE","network get mode",general_send_func},
   {2,"NETWORK","TRIGGER_STATUS","network trigger status",general_send_func}, 
   {3,"NETWORK","GET_STATUS","network get status",general_send_func},
   {4,"WIFI","START_SCAN","wifi start scan",general_send_func},
   {5,"WIFI","STOP_SCAN","wifi stop scan",general_send_func},
   {6,"WIFI","CONNECT","wifi connect station",wifi_send_func},
   {7,"WIFI","DISCONNECT","wifi disconnect station",general_send_func},
   {8,"WIFI","GET_WIFILIST","wifi get wifi list",general_send_func},
   {9,"WIFI_AP","CONNECT","wifi connect ap",wifi_ap_send_func},
   {10,"WIFI_AP","DISCONNECT","wifi disconnect ap",general_send_func},
   {11,"WIFI","GET_STATUS","wifi get status",general_send_func},
   {12,"MODEM","CONNECT","modem connect network",general_send_func},
   {13,"MODEM","DISCONNECT","modem disconnect network",general_send_func},
   {14,"MODEM", "GET_STATUS","modem get status",general_send_func},
   {15,"ETHERENT","CONNECT","etherent connect network",general_send_func},
   {16,"ETHERENT","DISCONNECT","etherent disconnect network",general_send_func},
   {17,"ETHERENT","GET_STATUS","etherent get status",general_send_func}, 
   {18,"WIFI","GET_CFG_NUM","wifi get config num",general_send_func}, 
   {19,"WIFI","SAVE_CFG"   ,"wifi save config file",general_send_func},
   {20,"WIFI","REMOVE","wifi remove station",wifi_send_func},
   {21,"WIFI","ENABLE","wifi enable all network",general_send_func},
   {22,"WIFI","DISABLE","wifi disabel all network",general_send_func},
   {23,"NETWORK","RESET_DNS","network reset dns",general_send_func},
   {24,"WIFI","REMOVE_ALL","wifi remove all config",general_send_func},
};


void usage(void)
{
  int i;
  tCMD_INFO *p_info;

  printf("netmanager test CMD menu:\n");
   
   for(i=0;i< sizeof(g_cmd_list)/sizeof(g_cmd_list[0]);i++)
   {
     p_info = &g_cmd_list[i];
     printf("%d %s\n",p_info->idx,p_info->info);
   }

   printf("    99 exit\n");
}



int main_loop(void) 
{
    int choice = 0;
    tCMD_INFO *p_info = &g_cmd_list[0];
    int max_choice = sizeof(g_cmd_list)/sizeof(g_cmd_list[0]);
    
    do {

        usage();

        choice = app_get_choice("Select cmd");

        printf("choice:%d\n",choice);

        if(choice >=0 && choice < max_choice)
             p_info[choice].send_func(p_info[choice].device,p_info[choice].command);
         
    } while (choice != 99);

    return 0;
}

int main(int argc, char *argv[argc]) {

    net_flora_init();
    main_loop();
    net_flora_exit();

    return 0;
}

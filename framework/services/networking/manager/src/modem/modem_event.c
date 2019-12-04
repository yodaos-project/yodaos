#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <ipc.h>
#include <flora_upload.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <common.h>
#include <network_report.h>
#include <modem_hal.h>
#include <hardware/modem_tty.h>

static int  g_modem_start_monitor = 0;
static tMODEM_STATUS g_modem_status= {0};
static pthread_mutex_t report_mutex  = PTHREAD_MUTEX_INITIALIZER;
/*
int modem_get_start_flag(void)
{
   return g_modem_start_monitor;
}
*/

static int get_modem_state(uint32_t type,uint32_t strength)
{
   int state;

   if (type == eSIGNAL_SERVICE_NONE || 
     strength == 99)
      state =  REPORT_UNCONNECT;
   else
      state = REPORT_CONNECT;

   return state;
}


static int modem_get_stauts(tMODEM_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(status,&g_modem_status,sizeof(tMODEM_STATUS));
    pthread_mutex_unlock(&report_mutex);

    return 0;
}

static int modem_set_status(tMODEM_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(&g_modem_status,status,sizeof(tMODEM_STATUS));
    pthread_mutex_unlock(&report_mutex);
    return 0;
}

static void modem_report_signal_strength(int signal)
{
    tMODEM_STATUS status;

    memset(&status,0,sizeof(tMODEM_STATUS));
    modem_get_stauts(&status);

  //  if(signal != status.signal)
  // {
       status.signal = signal;
       modem_set_status(&status);
       report_modem_status(&status);
  // }
}

static void modem_report_signal_type(int type)
{

    tMODEM_STATUS status;

    memset(&status,0,sizeof(tMODEM_STATUS));
    modem_get_stauts(&status);

    if(type != status.type)
    {
       status.type = type;
       modem_set_status(&status);
       report_modem_status(&status);
    }
}

static void modem_report_connect_state(int state)
{
   tMODEM_STATUS status;

   memset(&status,0,sizeof(tMODEM_STATUS));
   modem_get_stauts(&status);

   if(state != status.state)
   {
       status.state = state;
       modem_set_status(&status);
       report_modem_status(&status);
   }
}

#define MAX_UNCONNECT_CNTS (12)

void modem_event_monitor(void *eloop_ctx) 
{
    int state;
    int  strength;
    eMODEM_SIGNAL_TYPE type;
    eMODEM_SIGNAL_VENDOR vendor;

   static int unconnect_cnts = 0;


  if(modem_hal_get_signal_strength(&strength)==0)
   {
      modem_report_signal_strength(strength);
   }
   else 
   {
       NM_LOGW("modem_hal_get_signal_strength fail\n");
       return;
   }

   if(modem_hal_get_signal_type(&type,&vendor)==0)
   {
       modem_report_signal_type(type);
   }
   else
   {
      NM_LOGW("modem_hal_get_signal_type fail\n");
      return;
   }
          
    state = get_modem_state(type,strength); 

    modem_report_connect_state(state);
       
    if(state  == REPORT_UNCONNECT)
    {
        unconnect_cnts ++;
    }
   
    if(unconnect_cnts >= MAX_UNCONNECT_CNTS)
    {
          NM_LOGI("restart modem ");
          unconnect_cnts = 0;
          modem_hal_stop_modem();
          usleep(50000);
          modem_hal_start_modem(); 
    }

    return;
}

void modem_report_status(void)
{
    tMODEM_STATUS status;

    memset(&status,0,sizeof(tMODEM_STATUS));
    modem_get_stauts(&status);
    report_modem_status(&status);
}

int modem_respond_status(flora_call_reply_t reply)
{
     tMODEM_STATUS status;

     memset(&status,0,sizeof(tMODEM_STATUS));
     modem_get_stauts(&status);
     respond_modem_status(reply,&status);

     return 0;
} 


int modem_start_monitor(void)
{
  struct network_service *p_net = get_network_service();

 if(g_modem_start_monitor ==0)
 {
   g_modem_start_monitor  =1;
   eloop_timer_start(p_net->modem_monitor);
 }

   return 0;
}

void modem_stop_monitor(void)
{
  struct network_service *p_net = get_network_service();
  
  if(g_modem_start_monitor)
  {
     g_modem_start_monitor = 0;
     eloop_timer_stop(p_net->modem_monitor);
  }
}


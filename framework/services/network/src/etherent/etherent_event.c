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
#include <network_report.h>
#include <modem_hal.h>
#include <etherent_event.h>

#define ETHERENT_DEF_NAME  ("eth0")

static tETHERENT_STATUS g_etherent_status= {0};
static pthread_mutex_t report_mutex  = PTHREAD_MUTEX_INITIALIZER;


static int g_etherent_start_monitor= 0 ; 

int etherent_get_start_flag(void)
{
    return g_etherent_start_monitor;
}

static int etherent_is_link(int *status)
{
    int iret = 0;
    FILE *fp;
    char path[128] = {0};
    char buf[128] = {0};

    memset(path,0,sizeof(path)); 

    snprintf(path, sizeof(path), "/sys/class/net/%s/carrier",ETHERENT_DEF_NAME);

    if ((fp = fopen(path, "r")) != NULL)
    {
        while (fgets(buf, sizeof(buf), fp) != NULL)
        {
            if (buf[0] == '0')
            {
                *status = ETHERENT_UNLINK;
            }
            else
            {
                *status = ETHERENT_LINK;
            }
        }
    }
    else
    {
        iret = -1;
        NM_LOGE("Open carrier fail\n");
    }
    fclose(fp);
    return iret;
}

static int etherent_get_stauts(tETHERENT_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(status,&g_etherent_status,sizeof(tETHERENT_STATUS));
    pthread_mutex_unlock(&report_mutex);

    return 0;
}

static int etherent_set_status(tETHERENT_STATUS *status)
{
    pthread_mutex_lock(&report_mutex);
    memcpy(&g_etherent_status,status,sizeof(tETHERENT_STATUS));
    pthread_mutex_unlock(&report_mutex);
    return 0;
}


int etherent_get_rt_status(tETHERENT_STATUS *p_status)
{
   int link;

   tETHERENT_STATUS status;

    memset(&status,0,sizeof(tETHERENT_STATUS)); 

   status.state = REPORT_UNCONNECT;

   if(etherent_is_link(&link) == 0)
   {
      if(link == ETHERENT_LINK)
      {
         status.state = REPORT_CONNECT;
      }
   }

   etherent_set_status(&status);
   memcpy(p_status,&status,sizeof(tETHERENT_STATUS));

   return 0;
}

void etherent_report_status(void)
{
    tETHERENT_STATUS status;

    memset(&status,0,sizeof(tETHERENT_STATUS)); 
    etherent_get_stauts(&status);
    report_etherent_status(&status);
}

int etherent_respond_status(flora_call_reply_t reply)
{
     tETHERENT_STATUS status;

     memset(&status,0,sizeof(tETHERENT_STATUS)); 
     etherent_get_stauts(&status);
     respond_etherent_status(reply,&status);

     return 0;
}

int etherent_start_monitor(void)
{

/*
   struct network_service *p_net = get_network_service();
   eloop_timer_start(p_net->etherent_monitor);
*/
   g_etherent_start_monitor = 1;

   return 0;
}

void etherent_stop_monitor(void)
{
/*
    struct network_service *p_net = get_network_service();
    eloop_timer_stop(p_net->etherent_monitor);
*/
   g_etherent_start_monitor = 0;
}

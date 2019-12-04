#ifndef _NETWORK_REPORT_H_
#define _NETWORK_REPORT_H_

typedef struct wifi_status
{
   int  state;
   int signal;
}tWIFI_STATUS;

typedef struct modem_status
{
   int state;
   int type; //4G,3G,2G
   int signal;
}tMODEM_STATUS;

typedef struct etherent_status
{
   int state;
}tETHERENT_STATUS;

typedef struct network_status
{
   int state;
}tNETWORK_STATUS;

enum
{
  WIFI_STATUS = 0,
  MODEM_STATUS ,
  ETHERENT_STATUS   
};

enum{
 REPORT_UNCONNECT = 0, 
 REPORT_CONNECT   = 1,
};

enum
{
 REPORT_SIGNAL_WEAK=   0, 
 REPORT_SIGNAL_MIDDLE =1,
 REPORT_SIGNAL_STRONG =2,
};


extern int report_etherent_status(tETHERENT_STATUS *p_status);
extern int report_modem_status(tMODEM_STATUS *p_status);
extern int report_wifi_status(tWIFI_STATUS *p_status);
extern int report_network_status(tNETWORK_STATUS *p_status);

extern int respond_wifi_status(flora_call_reply_t reply,tWIFI_STATUS *p_status);
extern int respond_modem_status(flora_call_reply_t reply,tMODEM_STATUS *p_status);
extern int respond_etherent_status(flora_call_reply_t reply,tETHERENT_STATUS *p_status);
extern int respond_network_status(flora_call_reply_t reply,tNETWORK_STATUS *p_status);

#endif

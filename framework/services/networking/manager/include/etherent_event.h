#ifndef _ETHERENT_EVENT_H_
#define _ETHERENT_EVENT_H_
#include  "network_report.h"

enum
{
  ETHERENT_UNLINK=0,
  ETHERENT_LINK,
};

extern void etherent_report_status(void);
extern int etherent_start_monitor(void);
extern void etherent_stop_monitor(void);
extern int etherent_respond_status(flora_call_reply_t reply);

//extern void etherent_event_monitor(void *eloop_ctx); 
extern int etherent_get_start_flag(void);
extern int etherent_get_rt_status(tETHERENT_STATUS *p_status);
#endif

#ifndef _NETWORK_MONITOR_
#define _NETWORK_MONITOR_

#include <network_report.h>

extern int monitor_get_status(tNETWORK_STATUS *status);
extern void monitor_report_status(void);
extern  void judge_network_link(void *eloop_ctx); 
extern int monitor_respond_status(flora_call_reply_t reply);
#endif

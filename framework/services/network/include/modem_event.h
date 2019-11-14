#ifndef  _MODEM_EVENT_H_
#define _MODEM_EVENT_H_

extern void modem_report_status(void);
extern int modem_start_monitor(void);
extern void modem_stop_monitor(void);
extern int modem_respond_status(flora_call_reply_t reply);
extern void modem_event_monitor(void *eloop_ctx);
#endif

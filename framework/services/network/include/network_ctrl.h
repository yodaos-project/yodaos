#ifndef _NETWORK_CTRL_H_
#define _NETWORK__CTRL_H_

extern int get_wifi_capacity();
extern int get_modem_capacity();
extern int get_etherent_capacity();

extern  void handle_net_command(flora_call_reply_t  reply,const char *buf);
#endif

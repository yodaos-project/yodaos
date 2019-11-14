#ifndef __NETLINK_SWITCH_H
#define __NETLINK_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
    NETWORK_NONE = 0,
    WIFI =  1<<0,
    WIFI_AP = 1 <<1,
    MODEM =  1 <<2,
    ETHERENT = 1 <<3,
}eNETWORK_CONFIG_STATE;

#define MAX_SIZE  64 

typedef enum
{
  START =1,
  STOP = 2,
}eNETWORK_START;

typedef struct network_mode_info
{
//  int start;
 // int mode;
  char ssid[128];
  char psk[MAX_SIZE];
  char ip[MAX_SIZE];
  long timeout;
  uint32_t reason;

}tNETWORK_MODE_INFO;

extern int is_mode_in_capacity(int mode);
extern int  network_configure_net(int mode,int state, tNETWORK_MODE_INFO*info);

#ifdef __cplusplus
}
#endif
#endif

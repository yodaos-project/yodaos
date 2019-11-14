#ifndef __NET_SERVICE_IPC_H_
#define __NET_SERVICE_IPC_H_

#include <common.h>

__BEGIN_DECLS

extern void network_ipc_exit(void);
extern int network_ipc_init(void);
extern void network_ipc_upload(uint8_t *buf); 
extern void network_ipc_return(flora_call_reply_t reply, uint8_t *buf);

__END_DECLS

#endif

/**
 * @file   ipc.c
 * @author  <peng.zhang@rokid.com>
 * @date   Tue Nov  6 15:21:29 2018
 * 
 * @brief  
 * 
 * 
 */

#include <stdio.h>
#include <string.h>
#include <common.h>
#include <flora_upload.h>


void network_ipc_return(flora_call_reply_t reply, uint8_t *buf) 
{
    network_flora_return_msg(reply,buf);
}

void network_ipc_upload(uint8_t *buf) 
{
   network_flora_send_msg(buf);
}

int network_ipc_init(void) 
{
    int ret = 0;

    ret = network_flora_init();

    if (ret < 0) 
    {
        NM_LOGE("network flora init error\n");
        return -1;
    }
    
    return 0;
}

void network_ipc_exit(void)
{
    network_flora_exit();
}

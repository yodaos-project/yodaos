#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <flora-agent.h>
#include <flora_upload.h>
#include <network_ctrl.h>
#include <ipc.h>
#include <common.h>

#define NETWORK_STATUS_NAME      "network.status" 
#define NETWORK_COMMAND_NAME     "network.command"
#define FLORA_AGET_NET_MANAGER_URI  "unix:/var/run/flora.sock#net_manager"

static flora_agent_t agent;

void network_flora_send_msg(uint8_t *buf) 
{
     caps_t msg = caps_create();
 
     NM_LOGI("%s:%s\n",__func__,buf);
     caps_write_string(msg,(const char *)buf);
     flora_agent_post(agent,NETWORK_STATUS_NAME,msg, FLORA_MSGTYPE_INSTANT);
     caps_destroy(msg);
}

void network_flora_return_msg(flora_call_reply_t reply, uint8_t *buf) 
{
  caps_t data = caps_create();

  NM_LOGW("%s:%s\n",__func__,buf);
  caps_write_string(data,(char*) buf);
  flora_call_reply_write_code(reply, FLORA_CLI_SUCCESS);
  flora_call_reply_write_data(reply, data);
  flora_call_reply_end(reply);
  caps_destroy(data);
}


static void net_manager_command_callback(const char *name ,caps_t msg,flora_call_reply_t reply, void* arg) 
{
    const char *buf = NULL;
    caps_read_string(msg, &buf);
    struct network_service *p_net = get_network_service();

    NM_LOGW("name:%s:command:%s\n",name, buf);

//    eloop_timer_stop(p_net->network_sched);
    handle_net_command(reply,buf);
//    eloop_timer_start(p_net->network_sched);
}

int network_flora_init(void) 
{
    agent = flora_agent_create();
    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI,FLORA_AGET_NET_MANAGER_URI);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_declare_method(agent, NETWORK_COMMAND_NAME,net_manager_command_callback,NULL);

    flora_agent_start(agent, 0);

    return 0;
}

void network_flora_exit(void)
{
  flora_agent_close(agent);
  flora_agent_delete(agent);
}

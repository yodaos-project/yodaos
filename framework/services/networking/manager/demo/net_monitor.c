#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <json-c/json.h>
#include <flora-agent.h>

static flora_agent_t agent;

static void net_manager_command_callback(const char* name, caps_t msg, uint32_t type, void* arg) 
{
    const char * buf = NULL;
    caps_read_string(msg, &buf);
    printf("%s\n", buf);

}


#if 0
static void net_manager_sync_command_callback(const char *name ,caps_t msg,flora_call_reply_t reply, void* arg) 
{
    const char *buf = NULL;
   // struct network_service *ns = arg;

    printf("name :: %s \n", name);
    caps_read_string(msg, &buf);
    printf("upload :: %s\n", buf);

   caps_t data = caps_create();
   caps_write_string(data, "hello word");
   flora_call_reply_write_code(reply, FLORA_CLI_SUCCESS);
   flora_call_reply_write_data(reply, data);
   flora_call_reply_end(reply);
   caps_destroy(data);
}
#endif


int ns_flora_init(void) 
{

#if 0
    agent = flora_agent_create();
    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock#net_manager");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, "network.status",net_manager_command_callback, NULL);
    flora_agent_declare_method(agent,"network.command",net_manager_sync_command_callback,NULL);

    flora_agent_start(agent, 0);
#else

    agent = flora_agent_create();

    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, "network.status",net_manager_command_callback, NULL);

    flora_agent_start(agent, 0);
#endif

    return 0;
}

void ns_flora_exit(void)
{
  flora_agent_close(agent);
  flora_agent_delete(agent);
}

int main(int argc, char *argv[argc]) {

    ns_flora_init();

    while(1) 
    {
        sleep(1);
    }

    ns_flora_exit();

    return 0;
}

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include <json-c/json.h>
#if defined(BT_SERVICE_IPC_BY_FLORA)
#include <flora-agent.h>
static flora_agent_t agent;
#endif

/*BT FLORA EVT*/
#define FLORA_BT_COMMON_EVT "bluetooth.common.event"
#define FLORA_BT_BLE_EVT "bluetooth.ble.event"
#define FLORA_BT_BLE_CLI_EVT "bluetooth.ble.client.event"
#define FLORA_BT_A2DPSINK_EVT "bluetooth.a2dpsink.event"
#define FLORA_BT_A2DPSOURCE_EVT "bluetooth.a2dpsource.event"
#define FLORA_BT_HFP_EVT "bluetooth.hfp.event"
#define FLORA_BT_HH_EVT "bluetooth.hh.event"

/*BT FLORA CMD*/
#define FLORA_BT_COMMON_CMD "bluetooth.common.command"
#define FLORA_BT_BLE_CMD "bluetooth.ble.command"
#define FLORA_BT_BLE_CLI_CMD "bluetooth.ble.client.command"
#define FLORA_BT_A2DPSINK_CMD "bluetooth.a2dpsink.command"
#define FLORA_BT_A2DPSOURCE_CMD "bluetooth.a2dpsource.command"
#define FLORA_BT_HFP_CMD "bluetooth.hfp.command"
#define FLORA_BT_HH_CMD "bluetooth.hh.command"

/*BT FLORA raw data  with rokid header*/
#define FLORA_BT_BLE_CLI_WRITE "bluetooth.ble.client.write"
#define FLORA_BT_BLE_CLI_READ "bluetooth.ble.client.read"
#define FLORA_BT_BLE_CLI_NOTIFY "bluetooth.ble.client.notify"

/*OTHER CMD*/
#define FLORA_MODULE_SLEEP "module.sleep"
#define FLORA_POWER_SLEEP "power.sleep"

#define FLORA_VOICE_COMING "rokid.turen.voice_coming"

void print_time() {
    time_t now;
    struct tm *tm_now;
    struct timeval tv;

    time(&now);
    tm_now = localtime(&now);
    gettimeofday(&tv, NULL);

    printf("[%04d-%02d-%02d %02d:%02d:%02d %03d]  ", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, (int)tm_now->tm_sec, (int)tv.tv_usec/1000);
}

#if defined(BT_SERVICE_IPC_BY_FLORA)
static void bt_service_string_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const char *buf = NULL;

    print_time();
    printf("name :: %s\n", name);

    caps_read_string(msg, &buf);

    print_time();
    printf("data :: %s\n", buf);
}

static void bt_service_binary_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const char *buf = NULL;
    uint32_t len = 0;
    int i;

    print_time();
    printf("name :: %s\n", name);

    caps_read_binary(msg, (const void **)&buf, &len);

    print_time();
    for (i=0; i<len; i++) {
        printf("0x%02X ", buf[i]);
    }
    printf("\n");
}



int bt_flora_init() {
    agent = flora_agent_create();

    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, FLORA_BT_COMMON_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSINK_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSOURCE_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HFP_CMD, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HH_CMD, bt_service_string_callback, NULL);

    flora_agent_subscribe(agent, FLORA_BT_COMMON_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSINK_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSOURCE_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HFP_EVT, bt_service_string_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HH_EVT, bt_service_string_callback, NULL);

    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_WRITE, bt_service_binary_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_READ, bt_service_binary_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_NOTIFY, bt_service_binary_callback, NULL);
    flora_agent_start(agent, 0);

    return 0;
}
#endif

int main(int argc, char *argv[argc]) {
    int ret = 0;

#if defined(BT_SERVICE_IPC_BY_FLORA)
    ret = bt_flora_init();
#endif
    while(1) {
        sleep(1);
    }
#if defined(BT_SERVICE_IPC_BY_FLORA)
    flora_agent_close(agent);
    flora_agent_delete(agent);
    agent = 0;
#endif
    return ret;
}

#include <string.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/route.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <sys/un.h>
#include <errno.h>

#include <flora-agent.h>
#include <json-c/json.h>
#include <common.h>

static flora_agent_t agent;

void bt_flora_send_msg(int element, uint8_t *buf, int len) {
    caps_t msg = caps_create();

    caps_write_string(msg, (const char *)buf);
    switch (element) {
    case BLUETOOTH_FUNCTION_COMMON: {
        flora_agent_post(agent, FLORA_BT_COMMON_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_BLE: {
        flora_agent_post(agent, FLORA_BT_BLE_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_BLE_CLIENT: {
        flora_agent_post(agent, FLORA_BT_BLE_CLI_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_A2DPSINK: {
        flora_agent_post(agent, FLORA_BT_A2DPSINK_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_A2DPSOURCE: {
        flora_agent_post(agent, FLORA_BT_A2DPSOURCE_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_HFP : {
        flora_agent_post(agent, FLORA_BT_HFP_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    case BLUETOOTH_FUNCTION_HH : {
        flora_agent_post(agent, FLORA_BT_HH_EVT, msg, FLORA_MSGTYPE_INSTANT);
        break;
    }
    default:
        break;
    }
    caps_destroy(msg);
}

void bt_flora_send_binary_msg(int element, uint8_t *buf, int len) {
    caps_t msg = caps_create();
    caps_write_binary(msg, (const char *)buf, len);
    switch (element) {
    case BLUETOOTH_FUNCTION_BLE_CLI_NOTIFY: {
        flora_agent_post(agent, FLORA_BT_BLE_CLI_NOTIFY, msg, FLORA_MSGTYPE_INSTANT);
        break;
        }
    case BLUETOOTH_FUNCTION_BLE_CLI_READ: {
        flora_agent_post(agent, FLORA_BT_BLE_CLI_READ, msg, FLORA_MSGTYPE_INSTANT);
        break;
        }
    default:
        break;
    }
    caps_destroy(msg);
}

static void bt_service_command_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const char *buf = NULL;
    json_object *bt_command = NULL;

    BT_LOGV("name :: %s\n", name);

    if (strcmp(name, FLORA_BT_COMMON_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_common_handle(bt_command, g_bt_handle, NULL);
    } else if (strcmp(name, FLORA_BT_BLE_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_ble_handle(bt_command, g_bt_handle, NULL);
    } else if (strcmp(name, FLORA_BT_BLE_CLI_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_ble_client_handle(bt_command, g_bt_handle, NULL);
    } else if (strcmp(name, FLORA_BT_A2DPSINK_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_a2dpsink_handle(bt_command, g_bt_handle, NULL);
    } else if (strcmp(name, FLORA_VOICE_COMING) == 0) {
#if defined(ROKID_YODAOS)
        bt_a2dpsink_set_mute();
#endif
    } else if (strcmp(name, FLORA_MODULE_SLEEP) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_module_sleep(bt_command, g_bt_handle);
    } else if (strcmp(name, FLORA_POWER_SLEEP) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_power_sleep(bt_command, g_bt_handle);
    } else if (strcmp(name, FLORA_BT_A2DPSOURCE_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_a2dpsource_handle(bt_command, g_bt_handle, NULL);
    }

#if defined(BT_SERVICE_HAVE_HFP)
    else if (strcmp(name, FLORA_BT_HFP_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_hfp_handle(bt_command, g_bt_handle, NULL);
    }
#endif
    else if (strcmp(name, FLORA_BT_HH_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_hh_handle(bt_command, g_bt_handle, NULL);
    }

    json_object_put(bt_command);
}

void bt_flora_report_reply(uint32_t msgtype, void *data, char *buf) {
    flora_call_reply_t reply = (flora_call_reply_t)data;
    if (reply && buf) {
        caps_t msg = caps_create();
        caps_write_string(msg, buf);
        flora_call_reply_write_code(reply, FLORA_CLI_SUCCESS);
        flora_call_reply_write_data(reply, msg);
        flora_call_reply_end(reply);
        caps_destroy(msg);
    }
}

static void bt_service_command_method_callback(const char *name, caps_t msg, flora_call_reply_t reply, void *arg)
{
    const char *buf = NULL;
    json_object *bt_command = NULL;
    BT_LOGV("method_callback::name :: %s\n", name);

    if (strcmp(name, FLORA_BT_COMMON_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_common_handle(bt_command, g_bt_handle, (void*)reply);
    } else if (strcmp(name, FLORA_BT_BLE_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_ble_handle(bt_command, g_bt_handle, (void*)reply);
    } else if (strcmp(name, FLORA_BT_BLE_CLI_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_ble_client_handle(bt_command, g_bt_handle, (void*)reply);
    } else if (strcmp(name, FLORA_BT_A2DPSINK_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_a2dpsink_handle(bt_command, g_bt_handle, (void*)reply);
    } else if (strcmp(name, FLORA_BT_A2DPSOURCE_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_a2dpsource_handle(bt_command, g_bt_handle, (void*)reply);
    } else if (strcmp(name, FLORA_BT_HFP_CMD) == 0) {
    #if defined(BT_SERVICE_HAVE_HFP)
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_hfp_handle(bt_command, g_bt_handle, (void*)reply);
    #endif
    } else if (strcmp(name, FLORA_BT_HH_CMD) == 0) {
        caps_read_string(msg, &buf);
        bt_command = json_tokener_parse(buf);
        handle_hh_handle(bt_command, g_bt_handle, (void*)reply);
    }

    json_object_put(bt_command);

}


static void bt_service_binary_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const void *buf = NULL;
    uint32_t len = 0;
    caps_read_binary(msg, &buf, &len);
    if (strcmp(name, FLORA_BT_BLE_CLI_WRITE) == 0) {
        handle_ble_client_write(g_bt_handle, buf, len);
    }
}

void *bt_flora_handle(void *arg) {
    agent = flora_agent_create();

    prctl(PR_SET_NAME,"bluetooth_service_by_flora");

    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock#bluetoothservice-agent");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, FLORA_BT_COMMON_CMD, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CMD, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_CMD, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSINK_CMD, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSOURCE_CMD, bt_service_command_callback, NULL);
#if defined(BT_SERVICE_HAVE_HFP)
    flora_agent_subscribe(agent, FLORA_BT_HFP_CMD, bt_service_command_callback, NULL);
#endif
    flora_agent_subscribe(agent, FLORA_BT_HH_CMD, bt_service_command_callback, NULL);

    flora_agent_subscribe(agent, FLORA_VOICE_COMING, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_POWER_SLEEP, bt_service_command_callback, NULL);
    flora_agent_subscribe(agent, FLORA_MODULE_SLEEP, bt_service_command_callback, NULL);

    flora_agent_declare_method(agent, FLORA_BT_COMMON_CMD, bt_service_command_method_callback, NULL);
    flora_agent_declare_method(agent, FLORA_BT_BLE_CMD, bt_service_command_method_callback, NULL);
    flora_agent_declare_method(agent, FLORA_BT_BLE_CLI_CMD, bt_service_command_method_callback, NULL);
    flora_agent_declare_method(agent, FLORA_BT_A2DPSINK_CMD, bt_service_command_method_callback, NULL);
    flora_agent_declare_method(agent, FLORA_BT_A2DPSOURCE_CMD, bt_service_command_method_callback, NULL);
#if defined(BT_SERVICE_HAVE_HFP)
    flora_agent_declare_method(agent, FLORA_BT_HFP_CMD, bt_service_command_method_callback, NULL);
#endif
    flora_agent_declare_method(agent, FLORA_BT_HH_CMD, bt_service_command_method_callback, NULL);


    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_WRITE, bt_service_binary_callback, NULL);

    flora_agent_start(agent, 0);

    BT_LOGI("eloop run\n");
    eloop_run();
    BT_LOGI("exit:close flora\n");
    flora_agent_close(agent);
    flora_agent_delete(agent);
    agent = 0;

    return NULL;
}

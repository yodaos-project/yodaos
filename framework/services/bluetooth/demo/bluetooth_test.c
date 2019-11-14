#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <hardware/bt_type.h>
#include <hardware/bt_avrc.h>

#if defined(BT_SERVICE_IPC_BY_FLORA)
#include <flora-agent.h>
static flora_agent_t agent;


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
#endif

#pragma pack(1)
struct rokid_ble_client_header {
    uint32_t len; // header length
    uint8_t address[18];
    uint8_t service[MAX_LEN_UUID_STR];
    uint8_t character[MAX_LEN_UUID_STR];
    uint8_t description[MAX_LEN_UUID_STR];
};
#pragma pack()


static int connecting = 0;

static int app_get_choice(const char *querystring) {
    int neg, value, c, base;
    int count = 0;

    base = 10;
    neg = 1;
    printf("%s => ", querystring);
    value = 0;
    do {
        c = getchar();

        if ((count == 0) && (c == '\n')) {
            return -1;
        }
        count ++;

        if ((c >= '0') && (c <= '9')) {
            value = (value * base) + (c - '0');
        } else if ((c >= 'a') && (c <= 'f')) {
            value = (value * base) + (c - 'a' + 10);
        } else if ((c >= 'A') && (c <= 'F')) {
            value = (value * base) + (c - 'A' + 10);
        } else if (c == '-') {
            neg *= -1;
        } else if (c == 'x') {
            base = 16;
        }

    } while ((c != EOF) && (c != '\n'));

    return value * neg;
}

#if defined(BT_SERVICE_IPC_BY_FLORA)
static void bt_flora_send_string_cmd(char *name, uint8_t *buf, int len) {
    caps_t msg = caps_create();

    caps_write_string(msg, (const char *)buf);
    flora_agent_post(agent, name, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
}

static void bt_flora_send_binary_msg(char* name, uint8_t *buf, int len) {
    caps_t msg = caps_create();
    caps_write_binary(msg, (const char *)buf, len);
    flora_agent_post(agent, name, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
}
#endif

static int common_operation() {
    int choice;
    char *command = NULL;
    char *bt_command = NULL;
    //char *name = "Rokid-Me-9999zz";

    do {
        printf("Bluetooth common CMD menu:\n");
        printf("    1 discovery on\n");
        printf("    2 cancel discovery\n");
        printf("    3 get module address\n");
        printf("    4 set  discoverable\n");
        printf("    5  get paired list\n");
        printf("    6  remove device\n");

        printf("    99 exit\n");
        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1: {
            int type = BT_BR_EDR;
            command = "DISCOVERY";
            type = app_get_choice("input discovery type");
            json_object_object_add(root, "type", json_object_new_int(type));
        }
            break;
        case 2:
            command = "CANCEL_DISCOVERY";
            break;
        case 3:
            command = "MODULE_ADDRESS";
            break;
        case 4: {
            int discoverable = 1;
            command = "VISIBILITY";
            discoverable = app_get_choice("input discoverable (0 or 1)");
            json_object_object_add(root, "discoverable", json_object_new_boolean(discoverable));
        }
            break;
        case 5: 
            command = "PAIREDLIST";
            break;
        case 6:  {
            char address[18] ={0};

            printf("input remove address:");
            scanf("%s", address);
            address[17] = 0;
            printf("\nyour input address: %s\n", address);
            command = "REMOVE_DEV";
            json_object_object_add(root, "address", json_object_new_string(address));
        }
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if ( command) {
            json_object_object_add(root, "command", json_object_new_string(command));

            bt_command = (char *)json_object_to_json_string(root);

        #if defined(BT_SERVICE_IPC_BY_FLORA)
            bt_flora_send_string_cmd(FLORA_BT_COMMON_CMD, (uint8_t *)bt_command, strlen(bt_command));
        #endif
        }
        json_object_put(root);
    } while (choice != 99);

    return 0;
}

static int a2dp_source_operation() {
    int ret = 0;
    int choice;
    char *name = "Rokid-Me-9999zz";
    char *proto = "A2DPSOURCE";
    char *command = NULL;
    char *bt_command = NULL;
    //int unique = 1;
    int flora_call = 0;
    do {
        while (connecting != 0) {
            sleep(1);
        }
        printf("Bluetooth a2dp source CMD menu:\n");
        printf("    1 a2dp source on\n");
        printf("    2 a2dp source off\n");
        printf("    3 a2dp source discovery then connect one\n");
        printf("    4 disconnect_peer\n");
        printf("    5 call getstate\n");
        printf("    99 exit a2dp\n");

        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");
        switch (choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            break;
        case 2:
            command = "OFF";
            break;
        case 3:
            command = "DISCOVERY";
            connecting = 1;
            break;
        case 4:
            command = "DISCONNECT_PEER";
            break;
        case 5:
            flora_call = 1;
            command = "GETSTATE";
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if (!command) {
            json_object_put(root);
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));
        json_object_object_add(root, "command", json_object_new_string(command));

        bt_command = (char *)json_object_to_json_string(root);
#if defined(BT_SERVICE_IPC_BY_FLORA)

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)bt_command);

        if (flora_call) {
            flora_call = 0;
            flora_call_result result = {0};
            if (flora_agent_call(agent, FLORA_BT_A2DPSOURCE_CMD, msg, "bluetoothservice-agent", &result, 0) == FLORA_CLI_SUCCESS) {
                if (result.data && result.ret_code == FLORA_CLI_SUCCESS) {
                    const char *buf = NULL;
                    caps_read_string(result.data, &buf);
                    printf("data :: %s\n", buf);
                }
                flora_result_delete(&result);
            } else {
                printf("failed to flora_agent_call\n");
            }
        } else {
            flora_agent_post(agent, FLORA_BT_A2DPSOURCE_CMD, msg, FLORA_MSGTYPE_INSTANT);
        }
        caps_destroy(msg);
#endif
        json_object_put(root);
    } while (choice != 99);
    return ret;
}

static int a2dp_sink_operation() {
    int ret = 0;
    int choice;
    char *name = "Rokid-Me-9999zz";
    char *proto = "A2DPSINK";
    char *subquent = "PLAY";
    char *command = NULL;
    char *bt_command = NULL;
    int unique = 1;
    int flora_call = 0;

    do {
        while (connecting != 0) {
            sleep(1);
        }
        printf("Bluetooth a2dp sink CMD menu:\n");
        printf("    1 a2dp sink on\n");
        printf("    2 a2dp sink off\n");
        printf("    3 play\n");
        printf("    4 pause\n");
        printf("    5 stop\n");
        printf("    6 next\n");
        printf("    7 prev\n");
        printf("    8 disconnect\n");
        printf("    9 on play\n");
        printf("    10 mute\n");
        printf("    11 unmute\n");
        printf("    12 disconnect_peer\n");
        printf("    13 set volume\n");
        printf("    14 getstate\n");
        printf("    15 call getstate\n");
        printf("    16 getsong attrs\n");
        printf("    17 send cmd key\n");

        printf("    20 play & unmute\n");
        printf("    21 pause & mute\n");
        printf("    50 a2dp sink on &&  hfp on\n");
        printf("    51 a2dp sink off &&  hfp off\n");

        printf("    99 exit \n");

        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            break;
        case 2:
            command = "OFF";
            break;
        case 3:
            command = "PLAY";
            break;
        case 4:
            command = "PAUSE";
            break;
        case 5:
            command = "STOP";
            break;
        case 6:
            command = "NEXT";
            break;
        case 7:
            command = "PREV";
            break;
        case 8:
            command = "DISCONNECT";
            break;
        case 9:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "subsequent", json_object_new_string(subquent));
            json_object_object_add(root, "unique", json_object_new_boolean(unique));
            break;
        case 10:
            command = "MUTE";
            break;
        case 11:
            command = "UNMUTE";
            break;
        case 12:
            command = "DISCONNECT_PEER";
            break;
        case 13:
        {
            int vol;
            command = "VOLUME";
            vol = app_get_choice("input volume(0-100)");
            if (vol < 0) vol = 0;
            if (vol > 100) vol = 100;
            json_object_object_add(root, "value", json_object_new_int(vol));
            break;
         }
        case 14:
            command = "GETSTATE";
            break;
        case 15:
            flora_call = 1;
            command = "GETSTATE";
            break;
        case 16:
            command = "GETSONG_ATTRS";
            break;
        case 17: {
            int avrcp_key = BT_AVRC_REWIND;
            avrcp_key = app_get_choice("input key");
            printf("your input key is 0x%X\n", avrcp_key);
            command = "SEND_KEY";
            json_object_object_add(root, "key", json_object_new_int(avrcp_key));
        }
            break;
        case 20:
            command = "PLAY_UNMUTE";
            break;
        case 21:
            command = "PAUSE_MUTE";
            break;
        case 50:
        {
            command = "ON";
            char *sec_pro = "HFP";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "sec_pro", json_object_new_string(sec_pro));
        }
            break;
        case 51:
        {
            command = "OFF";
            char *sec_pro = "HFP";
            json_object_object_add(root, "sec_pro", json_object_new_string(sec_pro));
        }
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if (!command) {
            json_object_put(root);
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));
        json_object_object_add(root, "command", json_object_new_string(command));

        bt_command = (char *)json_object_to_json_string(root);

#if defined(BT_SERVICE_IPC_BY_FLORA)
        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)bt_command);
        if (flora_call) {
            flora_call = 0;
            flora_call_result result = {0};
            if (flora_agent_call(agent, FLORA_BT_A2DPSINK_CMD, msg, "bluetoothservice-agent", &result, 0) == FLORA_CLI_SUCCESS) {
                if (result.data && result.ret_code == FLORA_CLI_SUCCESS) {
                    const char *buf = NULL;
                    caps_read_string(result.data, &buf);
                    printf("data :: %s\n", buf);
                }
                flora_result_delete(&result);
            } else {
                printf("failed to flora_agent_call\n");
            }
        } else {
            flora_agent_post(agent, FLORA_BT_A2DPSINK_CMD, msg, FLORA_MSGTYPE_INSTANT);
        }
        caps_destroy(msg);
#endif
        json_object_put(root);
    } while (choice != 99);

    return ret;
}

int ble_operation() {
    //int ret = 0;
    int choice = 0;
    char *name = "Rokid-Pebble-9999zz";
    char *proto = "ROKID_BLE";
    char *command = NULL;
    char *data = NULL;
    char *bt_command = NULL;
    int unique = 1;
    int flora_call = 0;

    do {
        printf("Bluetooth ble test CMD menu:\n");
        printf("    1 ble on\n");
        printf("    2 ble off\n");
        printf("    3 send data\n");
        printf("    4 send raw data\n");
        printf("    5 call getstate\n");

        printf("    99 exit ble and off\n");

        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            json_object_object_add(root, "command", json_object_new_string(command));
            json_object_object_add(root, "unique", json_object_new_boolean(unique));
            break;
        case 2:
            command = "OFF";
            json_object_object_add(root, "command", json_object_new_string(command));
            break;
        case 3:
            data = "dashuaige";
            json_object_object_add(root, "data", json_object_new_string(data));
            break;
        case 4:
            data = "yaojingnalizou";
            json_object_object_add(root, "rawdata", json_object_new_string(data));
            break;
        case 5:
            flora_call = 1;
            command = "GETSTATE";
            break;
        case 99:
            command = "OFF";
            json_object_object_add(root, "command", json_object_new_string(command));
            break;
        default:
            json_object_put(root);
            continue;
        }
        if (!command) {
            json_object_put(root);
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));

        bt_command = (char *)json_object_to_json_string(root);

#if defined(BT_SERVICE_IPC_BY_FLORA)
        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)bt_command);
        if (flora_call) {
            flora_call = 0;
            flora_call_result result = {0};
            if (flora_agent_call(agent, FLORA_BT_BLE_CMD, msg, "bluetoothservice-agent", &result, 0) == FLORA_CLI_SUCCESS) {
                if (result.data && result.ret_code == FLORA_CLI_SUCCESS) {
                    const char *buf = NULL;
                    caps_read_string(result.data, &buf);
                    printf("data :: %s\n", buf);
                }
                flora_result_delete(&result);
            } else {
                printf("failed to flora_agent_call\n");
            }
        } else {
            flora_agent_post(agent, FLORA_BT_BLE_CMD, msg, FLORA_MSGTYPE_INSTANT);
        }
        caps_destroy(msg);
#endif
        json_object_put(root);
    } while (choice != 99);

    return 0;
}

static int hfp_operation(void) {
    int ret = 0;
    int choice;
    char *proto = "HFP";
    char *name = "Rokid-Me-9999zz";
    char *command = NULL;
    char *bt_command = NULL;
    int flora_call = 0;

    do {
        printf("Bluetooth hfp CMD menu:\n");
        printf("    1 hfp on\n");
        printf("    2 hfp off\n");
        printf("    3 answer\n");
        printf("    4 hangup\n");
        printf("    5 dailnum\n");
        printf("    6 call getstate\n");
        printf("    7 disconnect\n");

        printf("    99 exit hfp\n");
        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            break;
        case 2:
            command = "OFF";
            break;
        case 3:
            command = "ANSWERCALL";
            break;
        case 4:
            command = "HANGUP";
            break;
        case 5: {
            int  i = 0;
            char c = 0;
            char num[255] = {0};
            command = "DIALING";
            printf("input dial numbers\n");
            do
            {
                c = getchar();
                num[i] = c;
                i ++;
            } while ((c != EOF) && (c != '\n') && i < 255);
            printf("your input: %s\n", num);
            json_object_object_add(root, "NUMBER", json_object_new_string(num));
        }
            break;
        case 6:
            flora_call = 1;
            command = "GETSTATE";
            break;
        case 7:
            command = "DISCONNECT";
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if (!command) {
            json_object_put(root);
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));
        json_object_object_add(root, "command", json_object_new_string(command));

        bt_command = (char *)json_object_to_json_string(root);

#if defined(BT_SERVICE_IPC_BY_FLORA)

        caps_t msg = caps_create();
        caps_write_string(msg, (const char *)bt_command);
        if (flora_call) {
            flora_call = 0;
            flora_call_result result = {0};
            if (flora_agent_call(agent, FLORA_BT_HFP_CMD, msg, "bluetoothservice-agent", &result, 0) == FLORA_CLI_SUCCESS) {
                if (result.data && result.ret_code == FLORA_CLI_SUCCESS) {
                    const char *buf = NULL;
                    caps_read_string(result.data, &buf);
                    printf("data :: %s\n", buf);
                }
                flora_result_delete(&result);
            } else {
                printf("failed to flora_agent_call\n");
            }
        } else {
            flora_agent_post(agent, FLORA_BT_HFP_CMD, msg, FLORA_MSGTYPE_INSTANT);
        }
        caps_destroy(msg);
#endif
        json_object_put(root);
    } while (choice != 99);

    return ret;
}

static int hh_operation(void) {
    int choice;
    char *proto = "HH";
    char *command = NULL;
    char *bt_command = NULL;
    char *name = "Rokid-Me-9999zz";

    do {
        printf("Bluetooth hid host CMD menu:\n");
        printf("    1 hid host on\n");
        printf("    2 hid host off\n");
        printf("    3 connect\n");
        printf("    4 disconnect\n");

        printf("    99 exit HID host\n");
        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            break;
        case 2:
            command = "OFF";
            break;
        case 3: {
            char address[18] ={0};

            printf("input address:");
            scanf("%s", address);
            address[17] = 0;
            printf("\nyour input address: %s\n", address);
            json_object_object_add(root, "address", json_object_new_string(address));
            command = "CONNECT";
            }
            break;
        case 4:
            command = "DISCONNECT";
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if (!command) {
            json_object_put(root);
            break;
        }
        json_object_object_add(root, "proto", json_object_new_string(proto));
        json_object_object_add(root, "command", json_object_new_string(command));

        bt_command = (char *)json_object_to_json_string(root);

#if defined(BT_SERVICE_IPC_BY_FLORA)
        bt_flora_send_string_cmd(FLORA_BT_HH_CMD, (uint8_t *)bt_command, strlen(bt_command));
#endif
        json_object_put(root);
    } while (choice != 99);

    return 0;
}

static int ble_client_operation(void) {
    int choice;
    char *proto = "BLE_CLIENT";
    char *command = NULL;
    char *bt_command = NULL;
    char *name = "Rokid-Me-9999zz";

    do {
        printf("Bluetooth ble client CMD menu:\n");
        printf("    1 ble client on\n");
        printf("    2 ble client off\n");
        printf("    3 connect\n");
        printf("    4 disconnect\n");
        printf("    5 register nofity\n");
        printf("    6  write data\n");
        printf("    7  read data\n");

        printf("    99 exit ble client\n");
        json_object *root = json_object_new_object();
        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 1:
            command = "ON";
            json_object_object_add(root, "name", json_object_new_string(name));
            break;
        case 2:
            command = "OFF";
            break;
        case 3:{
            char address[18] ={0};

            printf("input address:");
            scanf("%s", address);
            address[17] = 0;
            printf("\nyour input address: %s\n", address);
            json_object_object_add(root, "address", json_object_new_string(address));
            command = "CONNECT";
            }
            break;
        case 4:
            command = "DISCONNECT";
            break;
        case 5: {
            char address[18];
            char s_uuid[MAX_LEN_UUID_STR];
            char c_uuid[MAX_LEN_UUID_STR];

            command = "REGISTER_NOTIFY";
            printf("input address:");
            scanf("%s", address);
            address[17] = 0;
            printf("\nyour input address : %s\n", address);

            printf("input service uuid:");
            scanf("%s", s_uuid);
            s_uuid[MAX_LEN_UUID_STR -1] = 0;
            printf("\nyour input service uuid:%s\n", s_uuid);
            printf("input character uuid:");
            scanf("%s", c_uuid);
            c_uuid[MAX_LEN_UUID_STR -1] = 0;
            printf("\nyour input character uuid:%s\n", c_uuid);
            json_object_object_add(root, "address", json_object_new_string(address));
            json_object_object_add(root, "service", json_object_new_string(s_uuid));
            json_object_object_add(root, "character", json_object_new_string(c_uuid));
        }
            break;
        case 6: {
            struct rokid_ble_client_header *head;
            unsigned char buff[1024] = {0};
            unsigned int data_len = 0;
            int i;
            unsigned char *data;

            command = NULL;
            head = (struct rokid_ble_client_header *)buff;
            head->len = sizeof(struct rokid_ble_client_header);
            data = buff + head->len;
            printf("input address:");
            scanf("%s", head->address);
            head->address[17] = 0;
            printf("\nyour input address : %s\n", head->address);

            printf("input service uuid:");
            scanf("%s", head->service);
            head->service[MAX_LEN_UUID_STR - 1] = 0;
            printf("\nyour input service uuid:%s\n", head->service);

            printf("input character uuid:");
            scanf("%s", head->character);
            head->character[MAX_LEN_UUID_STR - 1] = 0;
            printf("\nyour input character uuid:%s\n", head->character);

            printf("input write data len(1-200)");
            scanf("%d", &data_len);
            if (data_len <= 0 ||data_len > 200) break;
            printf("\ninput write data:");
            for(i=0; i< data_len; i++) {
                scanf("%02x", &data[i]);
            }
            printf("\n");
        #if defined(BT_SERVICE_IPC_BY_FLORA)
            bt_flora_send_binary_msg(FLORA_BT_BLE_CLI_WRITE, buff, head->len + data_len);
        #endif
        }
        break;
        case 7: {
            char address[18];
            char s_uuid[MAX_LEN_UUID_STR];
            char c_uuid[MAX_LEN_UUID_STR];

            command = "READ";
            printf("input address:");
            scanf("%s", address);
            address[17] = 0;
            printf("\nyour input address : %s\n", address);

            printf("input service uuid:");
            scanf("%s", s_uuid);
            s_uuid[MAX_LEN_UUID_STR -1] = 0;
            printf("\nyour input service uuid:%s\n", s_uuid);
            printf("input character uuid:");
            scanf("%s", c_uuid);
            c_uuid[MAX_LEN_UUID_STR -1] = 0;
            printf("\nyour input character uuid:%s\n", c_uuid);
            json_object_object_add(root, "address", json_object_new_string(address));
            json_object_object_add(root, "service", json_object_new_string(s_uuid));
            json_object_object_add(root, "character", json_object_new_string(c_uuid));
        }
            break;
        case 99:
            command = NULL;
            break;
        default:
            json_object_put(root);
            continue;
        }
        if ( command) {
            json_object_object_add(root, "proto", json_object_new_string(proto));
            json_object_object_add(root, "command", json_object_new_string(command));

            bt_command = (char *)json_object_to_json_string(root);

        #if defined(BT_SERVICE_IPC_BY_FLORA)
            bt_flora_send_string_cmd(FLORA_BT_BLE_CLI_CMD, (uint8_t *)bt_command, strlen(bt_command));
        #endif
        }
        json_object_put(root);
    } while (choice != 99);

    return 0;
}

int main_loop() {
    int choice = 0;

    do {
        printf("Bluetooth test CMD menu:\n");
        printf("    0 common\n");
        printf("    1 ble\n");
        printf("    2 a2dp_sink\n");
        printf("    3 a2dp_source\n");
        printf("    4 hfp\n");
        printf("    5 hid host\n");
        printf("    6 ble client\n");

        printf("    99 exit\n");

        choice = app_get_choice("Select cmd");

        switch(choice) {
        case 0:
            common_operation();
            break;
        case 1:
            ble_operation();
            break;
        case 2:
            a2dp_sink_operation();
            break;
        case 3:
            a2dp_source_operation();
            break;
        case 4:
            hfp_operation();
            break;
        case 5:
            hh_operation();
            break;
        case 6:
            ble_client_operation();
            break;
        case 99:
            break;
        default:
            break;
        }
    } while (choice != 99);

    return 0;
}

void a2dpsource_parse_scan_results(json_object *obj) {
    int choice = 0;
    json_object *bt_cmd = NULL;
    json_object *bt_res = NULL;
    json_object *bt_dev = NULL;
    json_object *child_obj = NULL;
    json_object *bt_addr = NULL;
    json_object *bt_name = NULL;
    json_object *bt_completed = NULL;
    char *proto = "A2DPSOURCE";
    char *blue_cmd = "CONNECT";
    int i = 0;
    //int ret = 0;

    if (connecting != 1) {
        return ;
    }

    if (json_object_object_get_ex(obj, "action", &bt_cmd) == TRUE) {
        if (strcmp(json_object_get_string(bt_cmd), "discovery") == 0) {

            if (json_object_object_get_ex(obj, "results", &bt_res) == TRUE) {

                if (json_object_object_get_ex(bt_res, "is_completed", &bt_completed) == TRUE) {
                    if (json_object_get_boolean(bt_completed) == 0) {
                        return;//not scan completed
                    }
                }

                if (json_object_object_get_ex(bt_res, "deviceList", &bt_dev) == TRUE) {
                    printf("length :: %d\n", json_object_array_length(bt_dev));

                    for (i = 0; i < json_object_array_length(bt_dev); i++) {
                        child_obj = json_object_array_get_idx(bt_dev, i);

                        json_object_object_get_ex(child_obj, "address", &bt_addr);
                        json_object_object_get_ex(child_obj, "name", &bt_name);

                        printf("%d\t%s\t%s\n", i, json_object_to_json_string(bt_addr), json_object_to_json_string(bt_name));
                    }

                    while (1) {
                        choice = app_get_choice("Select devices");
                        if (choice == 99) {
                            break;
                        }

                        if (choice >= json_object_array_length(bt_dev) || choice < 0) {
                            printf("num is invalid\n");
                            continue;
                        }

                        child_obj = json_object_array_get_idx(bt_dev, choice);
                        json_object_object_get_ex(child_obj, "address", &bt_addr);
                        json_object_object_get_ex(child_obj, "name", &bt_name);

                        printf("select device ::  %s\n", json_object_to_json_string(bt_name));
                        json_object *send_cmd = json_object_new_object();
                        json_object_object_add(send_cmd, "proto", json_object_new_string(proto));
                        json_object_object_add(send_cmd, "command",json_object_new_string(blue_cmd));
                        json_object_object_add(send_cmd, "address", bt_addr);
                        json_object_object_add(send_cmd, "name", bt_name);

                        printf("connect cmd :: %s\n", json_object_to_json_string(send_cmd));

#if defined(BT_SERVICE_IPC_BY_FLORA)
                        caps_t msg = caps_create();
                        caps_write_string(msg, (const char *)json_object_to_json_string(send_cmd));

                        flora_agent_post(agent, FLORA_BT_A2DPSOURCE_CMD, msg, FLORA_MSGTYPE_INSTANT);
                        caps_destroy(msg);
#endif
                        json_object_put(send_cmd);
                        break;
                    }
                    connecting = 0;
                }
            }
        }
    }
}

static int a2dpsource_event_handle(json_object *obj)
{
    if (is_error(obj)) {
        return -1;
    }
    a2dpsource_parse_scan_results(obj);
    return 0;
}

static int common_event_handle(json_object *obj)
{
    json_object *bt_event = NULL;
    json_object *bt_res = NULL;
    json_object *bt_dev = NULL;
    json_object *child_obj = NULL;
    json_object *bt_addr = NULL;
    json_object *bt_name = NULL;
    int i, dev_count ;
    //int ret;
    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "action", &bt_event) == TRUE) {
        if (strcmp(json_object_get_string(bt_event), "pairedList") == 0) {
            if (json_object_object_get_ex(obj, "results", &bt_res) == TRUE) {
                if (json_object_object_get_ex(bt_res, "deviceList", &bt_dev) == TRUE) {
                    dev_count = json_object_array_length(bt_dev);
                    printf("length :: %d\n", dev_count);
                    for (i = 0; i < dev_count; i++) {
                        child_obj = json_object_array_get_idx(bt_dev, i);

                        json_object_object_get_ex(child_obj, "address", &bt_addr);
                        json_object_object_get_ex(child_obj, "name", &bt_name);

                        printf("%d\t%s\t%s\n", i, json_object_to_json_string(bt_addr), json_object_to_json_string(bt_name));
                    }  
                }
            }
        }
    }
    return 0;
}

#if defined(BT_SERVICE_IPC_BY_FLORA)
static void bt_service_event_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const char *buf = NULL;
    json_object *bt_event = NULL;
    caps_read_string(msg, &buf);
    bt_event = json_tokener_parse(buf);

    if (strcmp(name, FLORA_BT_COMMON_EVT) == 0) {
        //printf("%s\n", buf);
        common_event_handle(bt_event);
    } else if (strcmp(name, FLORA_BT_A2DPSOURCE_EVT) == 0) {
        a2dpsource_event_handle(bt_event);
    } else if (strcmp(name, FLORA_BT_BLE_EVT) == 0) {
        //printf("%s\n", buf);
    } else if (strcmp(name, FLORA_BT_A2DPSINK_EVT) == 0) {
        //printf("%s\n", buf);
    }
    json_object_put(bt_event);
}

static void bt_service_ble_client_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const void *buf = NULL;
    uint32_t length;
    struct rokid_ble_client_header *head;
    char *data;
    int i;

    caps_read_binary(msg, &buf, &length);
    head = (struct rokid_ble_client_header *)buf;
    data = (char *)buf + head->len;
    printf("ble client : length(%d), data(%d)\n", length, length-head->len);
    printf("ble client : address %s, service %s, character %s\n", head->address, head->service, head->character);
    printf("ble client  data:\n");
    for (i = 0; i < (length - head->len); i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
}

int bt_flora_init() {
    agent = flora_agent_create();

    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, FLORA_BT_COMMON_EVT, bt_service_event_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_EVT, bt_service_event_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSINK_EVT, bt_service_event_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_A2DPSOURCE_EVT, bt_service_event_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HFP_EVT, bt_service_event_callback, NULL);


    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_NOTIFY, bt_service_ble_client_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_READ, bt_service_ble_client_callback, NULL);

    flora_agent_start(agent, 0);

    return 0;
}
#endif

int main(int argc, char *argv[argc]) {
    int ret = 0;

#if defined(BT_SERVICE_IPC_BY_FLORA)
    ret = bt_flora_init();
#endif
    ret = main_loop();
    printf("exit\n");
#if defined(BT_SERVICE_IPC_BY_FLORA)
    flora_agent_close(agent);
    flora_agent_delete(agent);
    agent = 0;
#endif
    return ret;
}

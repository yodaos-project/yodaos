#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <hardware/bt_type.h>
#include <flora-agent.h>
#include "adpcm_decoder.h"

#define DUMP_DATA

#ifdef USE_K18_HW_OPUS
#include <gxopus/opus.h>
#define USE_OPUS
#elif defined(USE_SW_OPUS)
#include <opus/opus.h>
#define USE_OPUS
#endif


#define BT_NAME "Rokid-Me-9999zz"

#define PROTO_HH "HH"
#define PROTO_BLE_CLIENT "BLE_CLIENT"
#define PROTO_BLE_SERVICE "ROKID_BLE"

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


#define FLORA_VOICE_START     "rokid.turen.start_voice"
#define FLORA_VOICE                  "rokid.turen.voice"
#define FLORA_VOICE_COMING  "rokid.turen.voice_coming"
#define FLORA_VOICE_END         "rokid.turen.end_voice"
#define FLORA_VOICE_CANCEL   "rokid.turen.cancel_voice"

#define FLORA_SPEECH_COMPLETED "rokid.speech.completed"

     

#define ATVV_SERVICE_UUID		"AB5E0001-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_TX_CHAR_UUID		"AB5E0002-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_RX_CHAR_UUID		"AB5E0003-5A21-4F05-BC7D-AF01F617B664"
#define ATVV_CTL_CHAR_UUID		"AB5E0004-5A21-4F05-BC7D-AF01F617B664"

#define ATVV_GET_CAPS (0x0A)
#define ATVV_MIC_OPEN (0x0C)
#define ATVV_MIC_CLOSE (0x0D)

#define ATVV_CTL_AUDIO_STOP  (0x00)
#define ATVV_CTL_AUDIO_START  (0x04)
#define ATVV_CTL_START_SEARCH  (0x08)
#define ATVV_CTL_AUDIO_SYNC  (0x0A)
#define ATVV_CTL_GET_CAPS_RESP  (0x0B)
#define ATVV_CTL_MIC_OPEN_ERROR  (0x0C)


#pragma pack(1)
struct rokid_ble_client_header {
    uint32_t len; // header length
    uint8_t address[18];
    uint8_t service[MAX_LEN_UUID_STR];
    uint8_t character[MAX_LEN_UUID_STR];
    uint8_t description[MAX_LEN_UUID_STR];
};
#pragma pack()

enum connect_state {
    BT_CONNECT_INVALID = 0,
    BT_CONNECTING,
    BT_CONNECTED,
    BT_DISCONNECTED,
    BT_CONNECTFAILED,
};

struct rokid_encode_data {
    int in_offset;
    uint8_t in_buffer[4096];
    uint8_t out_buffer[1024];
};

static int channels = 1;
static int sample = 16000;

static flora_agent_t agent;

static int hh_connect_state = BT_CONNECT_INVALID;
static int ble_client_connect_state = BT_CONNECT_INVALID;
static int discovery = 0;
static int audio_start;
uint8_t connect_address[18];

static FILE *raw_fp = NULL;
static FILE *pcm_fp = NULL;
static FILE *opus_fp = NULL;
static void * adpcm_decoder;

#ifdef USE_OPUS
#define MAX_PACKET 1024
static int frame_duration = 20; //ms
static int pcmSamples;
static int pcmFrameSize;
static struct rokid_encode_data g_data;
OpusEncoder *opus_encoder;
#endif

static uint32_t voice_id_;

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

static int send_bt_cmd(const char *msg_name, const char *proto, const char *cmd, const char *name,
                        const char *addr, const char *service, const char *character, int discovery_type) {
    json_object *root = json_object_new_object();
    char *bt_command = NULL;

    if (proto)
        json_object_object_add(root, "proto", json_object_new_string(proto));
    if (cmd) {
        json_object_object_add(root, "command", json_object_new_string(cmd));
        if (strcmp(cmd, "DISCOVERY") == 0) {
            json_object_object_add(root, "type", json_object_new_int(discovery_type));
        }
    }
    if (name)
        json_object_object_add(root, "name", json_object_new_string(name));
    if (addr)
        json_object_object_add(root, "address", json_object_new_string(addr));
    if (service)
        json_object_object_add(root, "service", json_object_new_string(service));
    if (character)
        json_object_object_add(root, "character", json_object_new_string(character));

    bt_command = (char *)json_object_to_json_string(root);

    bt_flora_send_string_cmd((char *)msg_name, (uint8_t *)bt_command, strlen(bt_command));
 
    json_object_put(root);

    return 0;
}

/*common*/
static int start_discovery(bool on, int type)
{
    const char *cmd;
    if (on) {
        cmd = "DISCOVERY";
        discovery = 1;
    } else {
        cmd = "CANCEL_DISCOVERY";
        discovery = 0;
    }

    return send_bt_cmd(FLORA_BT_COMMON_CMD, NULL, cmd, NULL, NULL, NULL, NULL, type); 
}
/*common end*/

/*hh*/
static int hh_on(bool on, const char *name)
{
    const char *cmd;
    if (on)
        cmd = "ON";
    else
        cmd = "OFF";

    return send_bt_cmd(FLORA_BT_HH_CMD, PROTO_HH, cmd, name, NULL, NULL, NULL, 0); 
}

static int hh_connect(const char *address)
{
    const char *cmd = "CONNECT";
    return send_bt_cmd(FLORA_BT_HH_CMD, PROTO_HH, cmd, NULL, address, NULL, NULL, 0); 
}
/*hh end*/

/*ble service*/
static int ble_service_on(bool on, const char *name)
{
    const char *cmd;
    if (on)
        cmd = "ON";
    else
        cmd = "OFF";

    return send_bt_cmd(FLORA_BT_BLE_CMD, PROTO_BLE_SERVICE, cmd, name, NULL, NULL, NULL, 0); 
}

/*ble client*/
static int ble_client_on(bool on, const char *name)
{
    const char *cmd;
    if (on)
        cmd = "ON";
    else
        cmd = "OFF";

    return send_bt_cmd(FLORA_BT_BLE_CLI_CMD, PROTO_BLE_CLIENT, cmd, name, NULL, NULL, NULL, 0); 
}

static int ble_client_connect(const char *address)
{
    const char *cmd = "CONNECT";
    return send_bt_cmd(FLORA_BT_BLE_CLI_CMD, PROTO_BLE_CLIENT, cmd, NULL, address, NULL, NULL, 0); 
}

static int ble_client_notify(const char *address, const char *service, const char *character)
{
    const char *cmd = "REGISTER_NOTIFY";
    return send_bt_cmd(FLORA_BT_BLE_CLI_CMD, PROTO_BLE_CLIENT, cmd, NULL, address, service, character, 0); 
}

static int ble_client_read(const char *address,const char *service, const char *character)
{
    const char *cmd = "READ";
    return send_bt_cmd(FLORA_BT_BLE_CLI_CMD, PROTO_BLE_CLIENT, cmd, NULL, address, service, character, 0); 
}

static int ble_client_write(const char *address, const char *service, const char *character, const char *data, int len)
{
    struct rokid_ble_client_header *head;
    unsigned char *buff;

    buff = malloc(sizeof(struct rokid_ble_client_header) + len);
    if (!buff) return -1;

    head = (struct rokid_ble_client_header *)buff;
    head->len = sizeof(struct rokid_ble_client_header);
    snprintf((char *)head->address, sizeof(head->address), "%s", address);
    snprintf((char *)head->service, sizeof(head->service), "%s", service);
    snprintf((char *)head->character, sizeof(head->character), "%s", character);
    memcpy(buff + head->len, data, len);
 
    bt_flora_send_binary_msg(FLORA_BT_BLE_CLI_WRITE, buff, head->len + len); 
    free(buff);
    return 0;
}

/*ble client end*/

static int common_event_handle(json_object *obj) {
    int choice = 0;
    char *event = NULL;
    json_object *bt_event = NULL;
    json_object *bt_results = NULL;
    json_object *bt_dev = NULL;
    json_object *child_obj = NULL;
    json_object *bt_addr = NULL;
    json_object *bt_name = NULL;
    json_object *bt_completed = NULL;
    int i = 0, dev_count = 0;

    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "action", &bt_event) == TRUE) {
        event = (char *)json_object_get_string(bt_event);
        if (strcmp(event, "discovery") == 0) {
            if (json_object_object_get_ex(obj, "results", &bt_results) == TRUE) {
                if (json_object_object_get_ex(bt_results, "is_completed", &bt_completed) == TRUE) {
                    if (json_object_get_boolean(bt_completed) == 0) {
                        return 0;//not scan completed
                    }
                }

                if (json_object_object_get_ex(bt_results, "deviceList", &bt_dev) == TRUE) {
                    dev_count = json_object_array_length(bt_dev);
                    printf("dev_count :: %d\n", dev_count);

                    for (i = 0; i < dev_count; i++) {
                        child_obj = json_object_array_get_idx(bt_dev, i);

                        json_object_object_get_ex(child_obj, "address", &bt_addr);
                        json_object_object_get_ex(child_obj, "name", &bt_name);

                        printf("%d\t%s\t%s\n", i, json_object_to_json_string(bt_addr), json_object_to_json_string(bt_name));
                    }

                    while ((hh_connect_state != BT_CONNECTING && hh_connect_state != BT_CONNECTED)) {
                        choice = app_get_choice("Select devices to connect hid,  choice 99 to break:");
                        if (choice == 99) {
                            break;
                        }

                        if (choice >= dev_count || choice < 0) {
                            printf("num is invalid\n");
                            continue;
                        }

                        child_obj = json_object_array_get_idx(bt_dev, choice);
                        json_object_object_get_ex(child_obj, "address", &bt_addr);
                        json_object_object_get_ex(child_obj, "name", &bt_name);

                        printf("select device ::  %s %s\n", (char *)json_object_to_json_string(bt_name), (char *)json_object_to_json_string(bt_addr));
                        hh_connect((char *)json_object_get_string(bt_addr));
                        break;
                    }
                }
                discovery = 0;
            }
        }
        else if (strcmp(event, "pairedList") == 0) {
            if (json_object_object_get_ex(obj, "results", &bt_results) == TRUE) {
                if (json_object_object_get_ex(bt_results, "deviceList", &bt_dev) == TRUE) {
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

static int ble_client_event_handle(json_object *obj)
{
    char *connect = NULL;
    json_object *bt_event = NULL;
    json_object *bt_connect = NULL; 
    json_object *bt_addr = NULL;
    json_object *bt_name = NULL;
 
    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "action", &bt_event) == TRUE) {
        if (strcmp(json_object_get_string(bt_event), "stateupdate") == 0) {
            if (json_object_object_get_ex(obj, "connect_state", &bt_connect) == TRUE) {
                connect = (char *)json_object_get_string(bt_connect);
                if (strcmp(connect, "connected") == 0) {
                    if (ble_client_connect_state != BT_CONNECTED) {
                        if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                            char value[10];
                            int len;
                            if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                                printf("ble_client: %s [%s]: connected\n", json_object_get_string(bt_name), json_object_get_string(bt_addr));
                            }
                            snprintf((char *)connect_address, sizeof(connect_address), "%s", json_object_get_string(bt_addr));
                            usleep(20 * 1000);
                            ble_client_notify(json_object_get_string(bt_addr), ATVV_SERVICE_UUID, ATVV_RX_CHAR_UUID);
                            usleep(20*1000);
                            ble_client_notify(json_object_get_string(bt_addr), ATVV_SERVICE_UUID, ATVV_CTL_CHAR_UUID);
                            usleep(20*1000);
                            memset(value, 0, sizeof(value));
                            value[0] = ATVV_GET_CAPS;
                            value[1] = 0;
                            value[2] = 04;
                            value[3] = 00;
                            value[4] = 02;
                            len = 5;
                            ble_client_write(json_object_get_string(bt_addr), ATVV_SERVICE_UUID, ATVV_TX_CHAR_UUID, value, len);
                        }
                        ble_client_connect_state = BT_CONNECTED;
                    }
                } else if (strcmp(connect, "connectfailed") == 0) {
                    ble_client_connect_state = BT_CONNECTFAILED;
                    printf("ble_client: connect failed\n");
                } else if (strcmp(connect, "disconnected") == 0) {
                    ble_client_connect_state = BT_DISCONNECTED;
                    printf("ble_client: disconnect\n");
                } else if (strcmp(connect, "conncting") == 0) {
                    ble_client_connect_state = BT_CONNECTING;
                } else {
                    ble_client_connect_state = BT_CONNECT_INVALID;
                }
            }
        }
    }
    return 0;
}

static int hh_event_handle(json_object *obj)
{
    char *connect = NULL;
    json_object *bt_event = NULL;
    json_object *bt_connect = NULL; 
    json_object *bt_addr = NULL;
    json_object *bt_name = NULL;
 
    if (is_error(obj)) {
        return -1;
    }
    if (json_object_object_get_ex(obj, "action", &bt_event) == TRUE) {
        if (strcmp(json_object_get_string(bt_event), "stateupdate") == 0) {
            if (json_object_object_get_ex(obj, "connect_state", &bt_connect) == TRUE) {
                connect = (char *)json_object_get_string(bt_connect);
                if (strcmp(connect, "connected") == 0) {
                    if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                        if (json_object_object_get_ex(obj, "name", &bt_name) == TRUE) {
                            printf("hid: %s [%s]: connected\n", json_object_get_string(bt_name), json_object_get_string(bt_addr));
                        }
                        if (ble_client_connect_state != BT_CONNECTED && ble_client_connect_state != BT_CONNECTING)
                            ble_client_connect(json_object_get_string(bt_addr));
                    }
                    hh_connect_state = BT_CONNECTED;
                } else if (strcmp(connect, "connectfailed") == 0) {
                    hh_connect_state = BT_CONNECTFAILED;
                    printf("hid: connect failed\n");
                } else if (strcmp(connect, "disconnected") == 0) {
                    hh_connect_state = BT_DISCONNECTED;
                    printf("hid: disconnect\n");
                } else if (strcmp(connect, "conncting") == 0) {
                    hh_connect_state = BT_CONNECTING;
                } else {
                    hh_connect_state = BT_CONNECT_INVALID;
                }
            }
        }
    }
    return 0;
}

static void bt_service_event_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const char *buf = NULL;
    json_object *bt_event = NULL;
    caps_read_string(msg, &buf);
    bt_event = json_tokener_parse(buf);

    if (strcmp(name, FLORA_BT_COMMON_EVT) == 0) {
        common_event_handle(bt_event);
    } else if (strcmp(name, FLORA_BT_BLE_CLI_EVT) == 0) {
        ble_client_event_handle(bt_event);
    } else if (strcmp(name, FLORA_BT_HH_EVT) == 0) {
        hh_event_handle(bt_event);
    }
    json_object_put(bt_event);
}

static int flora_start_voice () {
    caps_t msg = caps_create();

    caps_write_string(msg, ""); // trigger string
    caps_write_integer(msg, 0);// triggre_start
    caps_write_integer(msg, 0);// triggre_length
    caps_write_float(msg, 0.0);// voice_energy
    caps_write_integer(msg, 0);// trigger_confirm
    caps_write_string(msg, "");// extra
    caps_write_integer(msg, ++voice_id_);

    flora_agent_post(agent, FLORA_VOICE_START, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
    return 0;
}

static int flora_voice_coming () {
    caps_t msg = caps_create();

    caps_write_float(msg, 0.0);// sl

    flora_agent_post(agent, FLORA_VOICE_COMING, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
    return 0;
}

static int flora_put_voice (uint8_t *data, uint32_t len) {
    caps_t msg = caps_create();

    caps_write_binary(msg, data, len);
    caps_write_integer(msg, voice_id_);

    flora_agent_post(agent, FLORA_VOICE, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
    return 0;
}

static int flora_end_voice () {
    caps_t msg = caps_create();
    caps_write_integer(msg, voice_id_);
    flora_agent_post(agent, FLORA_VOICE_END, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
    return 0;
}

static int flora_cancel_voice () {
    caps_t msg = caps_create();
    caps_write_integer(msg, voice_id_);
    flora_agent_post(agent, FLORA_VOICE_CANCEL, msg, FLORA_MSGTYPE_INSTANT);
    caps_destroy(msg);
    return 0;
}

#ifdef USE_OPUS
static int init_opus_buff(void)
{
    memset(g_data.in_buffer, 0, sizeof(g_data.in_buffer));
    memset(g_data.out_buffer, 0, sizeof(g_data.out_buffer));
    g_data.in_offset = 0;
    return 0;
}

static int put_remain_encode_opus(void)
{
    int eSize = 0;
    if (g_data.in_offset >= pcmFrameSize) {
        printf("[Error] :remaind over framesize pls check!!!\n");
    }

    while (g_data.in_offset > 0) {
        memset(g_data.in_buffer + g_data.in_offset, 0, sizeof(g_data.in_buffer) - g_data.in_offset);
        eSize = opus_encode(opus_encoder, (const opus_int16*)g_data.in_buffer, pcmSamples, g_data.out_buffer+1, MAX_PACKET);
        if (eSize < 0) {
            printf("[encode OPUS]: Error! %s, %d\n", __func__, __LINE__);
            memset(g_data.in_buffer, 0, sizeof(g_data.in_buffer));
            g_data.in_offset = 0;
        } else {
            g_data.out_buffer[0] = eSize & 0xFF;
            if (opus_fp)
                fwrite(g_data.out_buffer, eSize + 1, 1, opus_fp);
            flora_put_voice(g_data.out_buffer, eSize + 1);
            if (g_data.in_offset > pcmFrameSize) {
                memmove(g_data.in_buffer, g_data.in_buffer + pcmFrameSize, g_data.in_offset - pcmFrameSize);
                g_data.in_offset -= pcmFrameSize;
            } else {
                g_data.in_offset = 0;
            }
        }
    }
    return 0;
}

static int put_encode_opus(void *handle, uint8_t *buffer, int size)
{
    int eSize = 0;

    if ((g_data.in_offset + size) > sizeof(g_data.in_buffer)) {
        printf("[Error] :buffer overflow, pls set bigger input buffer\n");
        memset(g_data.in_buffer, 0, sizeof(g_data.in_buffer));
        g_data.in_offset = 0;
    }
    memcpy(g_data.in_buffer+ g_data.in_offset, buffer, size);
    g_data.in_offset += size;
    while (g_data.in_offset >= pcmFrameSize) {//encode to opus
        eSize = opus_encode(opus_encoder, (const opus_int16*)g_data.in_buffer, pcmSamples, g_data.out_buffer+1, MAX_PACKET);
        if (eSize < 0) {
            printf("[encode OPUS]: Error! %s, %d, %d\n", __func__, __LINE__, eSize);
            memset(g_data.in_buffer, 0, sizeof(g_data.in_buffer));
            g_data.in_offset = 0;
        } else {
            g_data.out_buffer[0] = eSize & 0xFF;
            if (opus_fp)
                fwrite(g_data.out_buffer, eSize + 1, 1, opus_fp);
            flora_put_voice(g_data.out_buffer, eSize + 1);
            if (g_data.in_offset > pcmFrameSize) {
                memmove(g_data.in_buffer, g_data.in_buffer + pcmFrameSize, g_data.in_offset - pcmFrameSize);
            }
            g_data.in_offset -= pcmFrameSize;
        }
    }
    return 0;
}
#endif

static int adpcm_decode_callback(void *caller_handle, uint8_t *buffer, int size) {
    if (pcm_fp)
        fwrite(buffer, size, 1, pcm_fp);
 #ifdef USE_OPUS
    put_encode_opus(caller_handle, buffer, size);
 #else
    flora_put_voice(buffer, size);
 #endif
    return 0;
}

static void bt_service_ble_client_binary_callback(const char* name, caps_t msg, uint32_t type, void* arg) {
    const void *buf = NULL;
    uint32_t length;
    struct rokid_ble_client_header *head;
    char *data;
    uint32_t data_len;
    static int get_frames = 0;
    static int head_frames = 0;

    caps_read_binary(msg, &buf, &length);
    head = (struct rokid_ble_client_header *)buf;
    data = (char *)buf + head->len;
    data_len = length - head->len;
    if (data_len == 0) return;
#ifdef DUMP_LOG
    int i;
    printf("ble client : length(%d), data(%d)\n", length, data_len);
    printf("ble client : address %s, service %s, character %s\n", head->address, head->service, head->character);

    printf("ble client  data:\n");
    for (i = 0; i < data_len; i++) {
        printf("0x%02X ", data[i]);
    }
    printf("\n");
#endif
    if (strcmp((char *)head->service, ATVV_SERVICE_UUID) == 0) {
        if (strcmp((char *)head->character, ATVV_CTL_CHAR_UUID) == 0) {
            switch(data[0]) { //contrl type
                case ATVV_CTL_AUDIO_STOP:
                    printf("audio stop\n");
                    audio_start  = 0;
                    printf(" get frame num %d head frames(%d)\n", get_frames, head_frames);
                #ifdef USE_OPUS
                    put_remain_encode_opus();
                #endif
                    flora_end_voice();
                    break;
                case ATVV_CTL_AUDIO_START:
                    printf("audio start\n");
                    audio_start  = 1;
                    get_frames = 0;
                #ifdef USE_OPUS
                    init_opus_buff();
                #endif
                    flora_voice_coming();
                    break;
                case ATVV_CTL_START_SEARCH: {
                    char value [3];

                    printf("search key\n");
                    value[0] = ATVV_MIC_OPEN;
                    value[1] = 00;
                    value[2] = 02;
                    if (!audio_start) {
                        flora_start_voice();
                        ble_client_write((char *)head->address, (char *)head->service, ATVV_TX_CHAR_UUID, value, 3);
                    }
                }
                    break;
                case ATVV_CTL_AUDIO_SYNC: {
                    int frame;
                    frame = data[1];
                    frame = frame << 8 | data[2];
                    printf(" sync frame num %d  get %d, head frames(%d)\n", frame, get_frames, head_frames);
                }
                    break;
                case ATVV_CTL_GET_CAPS_RESP:
                    printf("get caps resp\n");
                    break;
                case ATVV_CTL_MIC_OPEN_ERROR:
                    printf("failed to mic open\n");
                    flora_end_voice();
                    break;
                default:
                    printf("unknow: contrl type 0x%02X\n", data[0]);
                    break;
            }
        }
        else if (strcmp((char *)head->character, ATVV_RX_CHAR_UUID) == 0) { //ADPCM
            get_frames++;
            head_frames = data[0];
            head_frames = head_frames << 8 | data[1];

            if (raw_fp)
                fwrite(data + 3, data_len - 3, 1, raw_fp);
            adpcm_decode_frame(adpcm_decoder, (unsigned char *)data+3, data_len - 3, adpcm_decode_callback, NULL);
        }
    }
}

static void handle_speech_completed(const char* name, caps_t msg, uint32_t type, void* arg) {
    uint32_t id;
    char data = ATVV_MIC_CLOSE;

    caps_read_integer(msg, (int32_t *)&id);
    printf("handleCompleted: id %d/%d\n", id, voice_id_);
    if (id != voice_id_) return;

    ble_client_write((const char *)connect_address, ATVV_SERVICE_UUID, ATVV_TX_CHAR_UUID, &data, 1);
    flora_end_voice();
}

int bt_flora_init() {
    agent = flora_agent_create();

    flora_agent_config(agent, FLORA_AGENT_CONFIG_URI, "unix:/var/run/flora.sock");
    flora_agent_config(agent, FLORA_AGENT_CONFIG_BUFSIZE, 80 * 1024);
    flora_agent_config(agent, FLORA_AGENT_CONFIG_RECONN_INTERVAL, 5000);

    flora_agent_subscribe(agent, FLORA_BT_COMMON_EVT, bt_service_event_callback, NULL); 
    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_EVT, bt_service_event_callback, NULL);
    flora_agent_subscribe(agent, FLORA_BT_HH_EVT, bt_service_event_callback, NULL);

    flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_NOTIFY, bt_service_ble_client_binary_callback, NULL);
    //flora_agent_subscribe(agent, FLORA_BT_BLE_CLI_READ, bt_service_ble_client_binary_callback, NULL);

    flora_agent_subscribe(agent, FLORA_SPEECH_COMPLETED, handle_speech_completed, NULL); 
    flora_agent_start(agent, 0);

    return 0;
}

int main(int argc, char *argv[argc]) {
    int ret = 0;
    int choice;

    ret = bt_flora_init();
    adpcm_decoder = adpcm_decoder_create(sample, channels, 16);
    if (!adpcm_decoder) {
        printf("faile to creat adpcm decoder\n");
        return -1;
    }

#ifdef USE_OPUS
    int application   = OPUS_APPLICATION_VOIP;
    int err;
    frame_duration = 20; //ms
    pcmSamples   = sample * frame_duration / 1000;
    pcmFrameSize = pcmSamples * channels * 16 / 8;
    opus_encoder = opus_encoder_create(sample, channels, application, &err);
    if ((err != OPUS_OK) ||( opus_encoder == NULL)) {
        printf("[OPUS]: Create Encoder Failed\n");
        adpcm_decode_close(adpcm_decoder);
        return -1;
    }
    opus_encoder_ctl(opus_encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_20_MS));
    opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(27800));
    #ifdef USE_K18_HW_OPUS
        opus_encoder_ctl(opus_encoder, OPUS_SET_COMPLEXITY(0));
    #elif defined(USE_SW_OPUS)
        //opus_encoder_ctl(opus_encoder, OPUS_SET_BANDWIDTH(OPUS_AUTO));
        opus_encoder_ctl(opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
        //opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(OPUS_AUTO));
        opus_encoder_ctl(opus_encoder, OPUS_SET_COMPLEXITY(4));
        opus_encoder_ctl(opus_encoder, OPUS_SET_VBR(1));
        opus_encoder_ctl(opus_encoder, OPUS_SET_VBR_CONSTRAINT(0));
        opus_encoder_ctl(opus_encoder, OPUS_SET_FORCE_CHANNELS(channels));
        //opus_encoder_ctl(opus_encoder, OPUS_SET_PACKET_LOSS_PERC(0));
    #endif
#endif

    ble_service_on(false, NULL);//close ble service
    ble_client_on(true, BT_NAME);
    hh_on(true, BT_NAME);

#ifdef DUMP_DATA
    raw_fp = fopen("/data/atvv.adpcm", "wb");
    pcm_fp = fopen("/data/atvv.pcm", "wb");
    opus_fp = fopen("/data/atvv.opus", "wb");
#endif

    usleep(500 * 1000);

#if 0
    int times = 50;
    while((hh_connect_state != BT_CONNECTED) || (ble_client_connect_state != BT_CONNECTED)) {
        sleep(1);
        times--;
        if (times == 0) {
            times = 50;
            if (discovery)
                start_discovery(false, BT_BLE);
            if (hh_connect_state != BT_CONNECTING && hh_connect_state != BT_CONNECTED) {//disvoery over,if  not connect we want device, restart discovery and try connect it
                start_discovery(true, BT_BLE);
            }
        }
    }
#endif
    do {
        while (discovery != 0) {
            sleep(1);
        }
        printf("Bluetooth ATVV CMD menu:\n");
        printf("    1 discovery on \n");
        printf("    2 discovery off \n");
        printf("    99 break\n");
        choice = app_get_choice("Select cmd:");
        switch (choice) {
            case 1:
                start_discovery(true, BT_BLE);
                break;
            case 2:
                start_discovery(false, BT_BLE);
                break;
            default:
                continue;
                break;
        }
    } while (choice != 99);

    printf("exit\n");
    adpcm_decode_close(adpcm_decoder);
#ifdef USE_OPUS
    opus_encoder_destroy(opus_encoder);
#endif
    if (raw_fp) {
        fclose(raw_fp);
        raw_fp = NULL;
    }
    if (pcm_fp) {
        fclose(pcm_fp);
        pcm_fp = NULL;
    }
    if (opus_fp) {
        fclose(opus_fp);
        opus_fp = NULL;
    }
    hh_on(false, NULL);
    ble_client_on(false, NULL);
    flora_agent_close(agent);
    flora_agent_delete(agent);
    agent = 0;

    return ret;
}

#ifndef __BT_SERVICE_IPC_H_
#define __BT_SERVICE_IPC_H_

#ifdef __cplusplus
extern "C" {
#endif

int report_bluetooth_information(int element, uint8_t *buf, int len);
int report_bluetooth_binary_information(int element, uint8_t *buf, int len);
int method_report_reply(uint32_t msgtype, void *data, char *buf);

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
void bt_zeromq_send_msg(int element, uint8_t *buf, int len);
#elif defined(BT_SERVICE_IPC_BY_FLORA)

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

void bt_flora_send_msg(int element, uint8_t *buf, int len);
void bt_flora_send_binary_msg(int element, uint8_t *buf, int len);
void bt_flora_report_reply(uint32_t msgtype, void *data, char *buf);
#endif

#ifdef __cplusplus
}
#endif
#endif

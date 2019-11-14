#include <string.h>
#include <stdio.h>

#include <common.h>


int report_bluetooth_information(int element, uint8_t *buf, int len) {

#if defined(BT_SERVICE_IPC_BY_ZEROMQ)
    bt_zeromq_send_msg(element, buf, len);
#elif defined(BT_SERVICE_IPC_BY_FLORA)
    bt_flora_send_msg(element, buf, len);
#endif
    return 0;
}

int report_bluetooth_binary_information(int element, uint8_t *buf, int len) {

#if defined(BT_SERVICE_IPC_BY_FLORA)
    bt_flora_send_binary_msg(element, buf, len);
#endif
    return 0;
}

int method_report_reply(uint32_t msgtype, void *data, char *buf) {

#if defined(BT_SERVICE_IPC_BY_FLORA)
    bt_flora_report_reply(msgtype, data, buf);
#endif
    return 0;
}


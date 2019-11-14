#include <string.h>
#include <stdio.h>
#include <sys/prctl.h>

#include <libzeromq_handle/sub.h>
#include <libzeromq_handle/pub.h>

#include <json-c/json.h>
#include <common.h>

ZEROMQ_SUB_DEFINE(bt_command, "ipc:///var/run/bluetooth/command", 1, 2048, 200)
ZEROMQ_SUB_DEFINE(test, "ipc:///var/run/bluetooth/bluetooth_test", 1, 2048, 200)

ZEROMQ_PUB_DEFINE(pub_bt_ble, "ipc:///var/run/bluetooth/rokid_ble_event", 10)
ZEROMQ_PUB_DEFINE(pub_bt_a2dpsink, "ipc:///var/run/bluetooth/a2dpsink_event", 10)
ZEROMQ_PUB_DEFINE(pub_bt_a2dp, "ipc:///var/run/bluetooth/a2dp_event", 10)
ZEROMQ_PUB_DEFINE(pub_bt_hfp, "ipc:///var/run/bluetooth/hfp_event", 10)
ZEROMQ_PUB_DEFINE(pub_bt_avrcp, "ipc:///var/run/bluetooth/avrcp_event", 10)

ZEROMQ_SUB_DEFINE(ble_out_stream, "ipc:///var/run/bluetooth/ble_out_stream", 1, 2048, 200)
ZEROMQ_PUB_DEFINE(ble_in_stream, "ipc:///var/run/bluetooth/ble_in_stream", 10)


void bt_zeromq_send_msg(int element, uint8_t *buf, int len) {
    int ret = -1;

    switch (element) {
    case BLUETOOTH_FUNCTION_BLE: {
        ret = zeromq_publish_data(ZEROMQ_PUB_ID(pub_bt_ble), buf, len);
        break;
    }

    case BLUETOOTH_FUNCTION_A2DPSINK: {
        ret = zeromq_publish_data(ZEROMQ_PUB_ID(pub_bt_a2dpsink), buf, len);
        break;
    }

    case BLUETOOTH_FUNCTION_A2DPSOURCE: {
        ret = zeromq_publish_data(ZEROMQ_PUB_ID(pub_bt_a2dp), buf, len);
        break;
    }

    case BLUETOOTH_FUNCTION_HFP: {
        ret = zeromq_publish_data(ZEROMQ_PUB_ID(pub_bt_hfp), buf, len);
        break;
    }

    default:
        break;
    }

    if (ret) {
        BT_LOGE("pub send error element %d\n", element);
    }
}


void bt_command_callback(void *eloop_ctx, uint32_t event_mask) {
    int ret = 0;
    char buf[1024] = {0};
    json_object *bt_command = NULL;
    json_object *bt_proto = NULL;
    char *proto = NULL;

    ret = zeromq_subscriber_recv(eloop_ctx, (uint8_t *)buf, sizeof(buf));
    if (ret > 0) {
        BT_LOGI("recv :: %s len:: %d\n", buf, ret);
        bt_command = json_tokener_parse(buf);
        if (json_object_object_get_ex(bt_command, "proto", &bt_proto) == TRUE) {
            proto = (char *)json_object_get_string(bt_proto);
            if (strcmp(proto, "ROKID_BLE") == 0) {
                handle_ble_handle(bt_command, g_bt_handle, NULL);
            } else if (strcmp(proto, "A2DPSINK") == 0) {
                handle_a2dpsink_handle(bt_command, g_bt_handle, NULL);
            } else if (strcmp(proto, "A2DPSOURCE") == 0) {
                handle_a2dpsource_handle(bt_command, g_bt_handle, NULL);
            } else if (strcmp(proto, "AVRCP") == 0) {
            }
#if defined(BT_SERVICE_HAVE_HFP)
            else if (strcmp(proto, "HFP") == 0) {
                handle_hfp_handle(bt_command, g_bt_handle, NULL);
            }
#endif
            else if (strcmp(proto, "COMMON") == 0) {

            }
        }
        json_object_put(bt_command);
    }
}

void bt_bleout_raw_callback(void *eloop_ctx, uint32_t event_mask) {
    int len = 0;
    char buf[2048] = {0};
    struct rk_bluetooth *bt = g_bt_handle->bt;

    len = zeromq_subscriber_recv(eloop_ctx, (uint8_t *)buf, sizeof(buf));
    if (len > 0) {
        ble_send_raw_data(bt, (uint8_t *)buf, len);
    }
}

void *bt_zeromq_handle(void *arg) {
    int ret = 0;
    int fd_bt_commmand = 0;
    int fd_bt_test = 0;
    int fd_ble_out = 0;

    prctl(PR_SET_NAME,"bluetooth_service_by_zeromq");

    fd_bt_commmand = zeromq_subscriber_init(ZEROMQ_SUB_ID(bt_command));
    if (fd_bt_commmand < 0) {
        BT_LOGE("create bt_command error %d\n", fd_bt_commmand);
        return NULL;
    }

    fd_bt_test = zeromq_subscriber_init(ZEROMQ_SUB_ID(test));
    if (fd_bt_test < 0) {
        BT_LOGE("create test error %d\n", fd_bt_test);
        goto bt_command_release;
    }

    fd_ble_out = zeromq_subscriber_init(ZEROMQ_SUB_ID(ble_out_stream));
    if (fd_ble_out < 0) {
        BT_LOGE("create test error %d\n", fd_ble_out);
        goto bt_test_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(pub_bt_ble));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto ble_out_stream_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(pub_bt_a2dpsink));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto bt_pub_ble_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(pub_bt_a2dp));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto bt_pub_a2dpsink_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(pub_bt_hfp));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto bt_pub_a2dp_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(pub_bt_avrcp));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto bt_pub_hfp_release;
    }

    ret = zeromq_publish_init(ZEROMQ_PUB_ID(ble_in_stream));
    if (ret) {
        BT_LOGE("pub start error\n");
        goto bt_pub_avrcp_release;
    }

    eloop_handler_add_fd(fd_bt_commmand, bt_command_callback, ZEROMQ_SUB_ID(bt_command));
    eloop_handler_add_fd(fd_bt_test, bt_command_callback, ZEROMQ_SUB_ID(test));
    eloop_handler_add_fd(fd_ble_out, bt_bleout_raw_callback, ZEROMQ_SUB_ID(ble_out_stream));
    // eloop_register_read_sock(fd_bt_commmand, bt_command_callback, ZEROMQ_SUB_ID(bt_command), g_bt);
    // eloop_register_read_sock(fd_bt_test, bt_command_callback, ZEROMQ_SUB_ID(test), g_bt);
    // eloop_register_read_sock(fd_ble_out, bt_bleout_raw_callback, ZEROMQ_SUB_ID(ble_out_stream), g_bt);

    eloop_run();

    zeromq_publish_uninit(ZEROMQ_PUB_ID(ble_in_stream));
bt_pub_avrcp_release:
    zeromq_publish_uninit(ZEROMQ_PUB_ID(pub_bt_avrcp));
bt_pub_hfp_release:
    zeromq_publish_uninit(ZEROMQ_PUB_ID(pub_bt_hfp));
bt_pub_a2dp_release:
    zeromq_publish_uninit(ZEROMQ_PUB_ID(pub_bt_a2dp));
bt_pub_a2dpsink_release:
    zeromq_publish_uninit(ZEROMQ_PUB_ID(pub_bt_a2dpsink));
bt_pub_ble_release:
    zeromq_publish_uninit(ZEROMQ_PUB_ID(pub_bt_ble));
ble_out_stream_release:
    zeromq_subscriber_uninit(ZEROMQ_SUB_ID(ble_out_stream));
bt_test_release:
    zeromq_subscriber_uninit(ZEROMQ_SUB_ID(test));
bt_command_release:
    zeromq_subscriber_uninit(ZEROMQ_SUB_ID(bt_command));

    return NULL;
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

int bt_open(struct bt_service_handle *handle, const char *name)
{
    int ret;
    struct rk_bluetooth *bt = handle->bt;

    if (handle->open) return 0;
    ret = bt->open(bt, name);
    if (ret) {
        BT_LOGE("failed");
        return ret;
    }
    handle->open = 1;
    return 0;
}

int bt_close(struct bt_service_handle *handle)
{
    //int ret;
    struct rk_bluetooth *bt = handle->bt;

    BT_LOGI("status:%x", handle->status);
    if (handle->open && !handle->status) {
        /*
        ret = bt->set_visibility(bt, false, false);
        if (ret) {
            BT_LOGE("bt set visibility off error\n");
        }*/
        bt->close(bt);
        handle->open = 0;
    }
    return 0;
}

/*close mask_status opened profiles */
int bt_close_mask_profile(struct bt_service_handle *handle, uint32_t mask_status)
{
    int status = handle->status;

    status &= mask_status;

    if (status & HID_HOST_STATUS_MASK) {
        bt_hh_off(handle->bt);
    }
    if (status & HFP_STATUS_MASK) {
#if defined(BT_SERVICE_HAVE_HFP)
        bt_hfp_off(handle->bt);
#endif
    }
    if (status & A2DP_SINK_STATUS_MASK) {
        bt_a2dpsink_off(handle->bt);
    }
    if (status & A2DP_SOURCE_STATUS_MASK) {
        bt_a2dpsource_off(handle->bt);
    }
    if (status & BLE_CLIENT_STATUS_MASK) {
        bt_ble_client_off(handle->bt);
    }
    if (status & BLE_STATUS_MASK) {
        bt_ble_off(handle->bt);
    }
    return 0;
}

static int bt_get_mac_addr(struct bt_service_handle *handle, void *reply)
{
    int ret;
    BTAddr bd_addr;
    json_object *root = json_object_new_object();
    char *re_state;
    char address[18] = {0};
    struct rk_bluetooth *bt = handle->bt;

    ret = bt->get_module_addr(bt, bd_addr);
    snprintf(address, sizeof(address), "%02X:%02X:%02X:%02X:%02X:%02X",
                    bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    json_object_object_add(root, "module_address", json_object_new_string(address));
    re_state = (char *)json_object_to_json_string(root);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_COMMON, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_COMMON, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);

    return ret;
}

static int bt_set_discoveryable(struct rk_bluetooth *bt, bool discoverable, void *reply) {
    int ret = 0;
    json_object *root = json_object_new_object();
    char *re_state = NULL;
    char *broadcast_state = NULL;

    ret = bt->set_visibility(bt, discoverable, true);
    if (ret) {
        BT_LOGE("set visibility error\n");
    }
    if (discoverable)
        broadcast_state = "opened";
    else
        broadcast_state = "closed";

    json_object_object_add(root, "broadcast_state", json_object_new_string(broadcast_state));
    re_state = (char *)json_object_to_json_string(root);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_COMMON, reply, re_state);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_COMMON, (uint8_t *)re_state, strlen(re_state));

    json_object_put(root);

    return 0;
}

static int bt_get_paired_list(struct rk_bluetooth *bt, void *reply) {
    BTDevice devices[BT_DISC_NB_DEVICES];
    int dev_count, i;
    char address[18] = {0};
    char *action = "pairedList";
    char *buf = NULL;

    json_object *root = json_object_new_object();
    json_object *results = NULL;
    json_object *array = NULL;
    json_object *item[BT_DISC_NB_DEVICES];

    json_object_object_add(root, "action", json_object_new_string(action));

    dev_count = bt->get_bonded_devices(bt, devices, BT_DISC_NB_DEVICES);
    if (dev_count > 0) {
        results = json_object_new_object();
        array = json_object_new_array();

        for (i = 0; i < dev_count; i++) {
            memset(address, 0, sizeof(address));

            sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
                        devices[i].bd_addr[0],
                        devices[i].bd_addr[1],
                        devices[i].bd_addr[2],
                        devices[i].bd_addr[3],
                        devices[i].bd_addr[4],
                        devices[i].bd_addr[5]);
            item[i] = json_object_new_object();
            json_object_object_add(item[i], "address", json_object_new_string(address));
            json_object_object_add(item[i], "name", json_object_new_string(devices[i].name));
            json_object_array_add(array, item[i]);
        }
        json_object_object_add(results, "deviceList", array);
        json_object_object_add(root, "results", results);
    }

    buf = (char *)json_object_to_json_string(root);
    BT_LOGD("results :: %s\n", buf);

    if (reply)
        method_report_reply(BLUETOOTH_FUNCTION_COMMON, reply, buf);
    else
        report_bluetooth_information(BLUETOOTH_FUNCTION_COMMON, (uint8_t *)buf, strlen(buf));

    json_object_put(root);
    return 0;
}


void broadcast_remove_dev(const char* addr, int status)
{
    json_object *root = json_object_new_object();
    char *data = NULL;
    char *action = "remove_dev";

    json_object_object_add(root, "action", json_object_new_string(action));
    json_object_object_add(root, "address", json_object_new_string(addr));
    if (status == 99) {//devices list empty yet
        status = 0;
        json_object_object_add(root, "empty", json_object_new_boolean(1));
    }
    json_object_object_add(root, "status", json_object_new_int(status));
    data = (char *)json_object_to_json_string(root);
    report_bluetooth_information(BLUETOOTH_FUNCTION_COMMON, (uint8_t *)data, strlen(data));
    json_object_put(root);
}

static int bt_remove_dev(struct rk_bluetooth *bt, const char *address)
{
    int ret;
    struct bt_autoconnect_device dev ;
    struct bt_autoconnect_config *a2dpsink_config = &g_bt_a2dpsink->config;
    struct bt_autoconnect_config *a2dpsource_config = &g_bt_a2dpsource->config;
    int i;

    ret = bt_open(g_bt_handle, NULL);
    if (strcmp(address, "*") == 0) {
         ret = bt->config_clear(bt);
         if (ret)
            broadcast_remove_dev(address, ret);
        for (i = 0; i < a2dpsink_config->autoconnect_num; i++) {
           memset(&a2dpsink_config->dev[i], 0, sizeof(a2dpsink_config->dev[i]));
        }
        a2dpsink_config->autoconnect_linknum = 0;
        bt_autoconnect_sync(a2dpsink_config);

        for (i = 0; i < a2dpsource_config->autoconnect_num; i++) {
           memset(&a2dpsource_config->dev[i], 0, sizeof(a2dpsource_config->dev[i]));
        }
        a2dpsource_config->autoconnect_linknum = 0;
        bt_autoconnect_sync(a2dpsource_config);
    } else {
        ret = bd_strtoba(dev.addr, address);
        if (ret) {
            broadcast_remove_dev(address, ret);
            goto exit;
        }
        ret = bt->remove_dev(bt, dev.addr);
        if (ret) {
            broadcast_remove_dev(address, ret);
            //goto exit;
        }

        ret = bt_autoconnect_remove(a2dpsink_config, &dev);
        if (ret == 0)
            bt_autoconnect_sync(a2dpsink_config);

        ret = bt_autoconnect_remove(a2dpsource_config, &dev);
        if (ret == 0)
            bt_autoconnect_sync(a2dpsource_config);
    }
exit:
    bt_close(g_bt_handle);
    return ret;
}

void bt_upload_scan_results(const char *bt_name, BTAddr bt_addr, int is_completed, void *data) {
    struct rk_bluetooth *bt = g_bt_handle->bt;
    char address[18] = {0};
    BTDevice scan_devices[BT_DISC_NB_DEVICES];
    int scan_devices_num = 0;
    int i;
    char *action = "discovery";
    char *buf = NULL;

    if (!g_bt_handle->scanning) return;

    json_object *root = json_object_new_object();
    json_object *results = json_object_new_object();
    json_object *array = NULL;
    json_object *item[BT_DISC_NB_DEVICES];

    json_object_object_add(results, "is_completed", json_object_new_boolean(is_completed));
    json_object_object_add(root, "action", json_object_new_string(action));

    memset(scan_devices, 0, sizeof(scan_devices));
    scan_devices_num = bt->get_disc_devices(bt, scan_devices, BT_DISC_NB_DEVICES);
    if (scan_devices_num > 0) {
        array = json_object_new_array();
        for (i = 0; i < scan_devices_num; i++) {
            memset(address, 0, sizeof(address));

            sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
                        scan_devices[i].bd_addr[0],
                        scan_devices[i].bd_addr[1],
                        scan_devices[i].bd_addr[2],
                        scan_devices[i].bd_addr[3],
                        scan_devices[i].bd_addr[4],
                        scan_devices[i].bd_addr[5]);

                item[i] = json_object_new_object();
                json_object_object_add(item[i], "address", json_object_new_string(address));
                json_object_object_add(item[i], "name", json_object_new_string(scan_devices[i].name));
                json_object_array_add(array, item[i]);
        }
        json_object_object_add(results, "deviceList", array);
    }
    json_object_object_add(root, "results", results);

    buf = (char *)json_object_to_json_string(root);
    BT_LOGD("results :: %s\n", buf);
    report_bluetooth_information(BLUETOOTH_FUNCTION_COMMON, (uint8_t *)buf, strlen(buf));

    json_object_put(root);
}

int bt_discovery(struct bt_service_handle *handle, int type) {
    struct rk_bluetooth *bt = handle->bt;
    int ret = -1;

    if (type < BT_A2DP_SOURCE || type > BT_ALL) return -1;
    if (handle->open != 1) return -1;
    if (bt->start_discovery) {
        BT_LOGV("scanning  :: \n");
        if (handle->scanning) {
            BT_LOGW("scanning already :: \n");
            return 0;
        }

        ret = bt->start_discovery(bt, type);
        BT_LOGI("scanning begin :: type(%d)\n", type);
        if (ret) {
            BT_LOGW("discovery error\n");
        } else {
            handle->scanning = 1;
        }
    }
    return ret;
}

int bt_cancel_discovery(struct bt_service_handle *handle) {
    struct rk_bluetooth *bt = handle->bt;
    int ret = -1;

    if (handle->open != 1) return 0;
    if (bt->cancel_discovery) {
        if (!handle->scanning) {
            BT_LOGW("not scanning yet :: \n");
            return 0;
        }

        ret = bt->cancel_discovery(bt);
        if (ret) {
            BT_LOGW("cancel discovery error\n");
            handle->scanning = 0;
        } else {
            handle->scanning = 0;
        }
    }
    return ret;
}

int handle_common_handle(json_object *obj, struct bt_service_handle *handle, void *reply) {
    char *command = NULL;
    json_object *bt_cmd = NULL;
    struct rk_bluetooth *bt = handle->bt;

    if (is_error(obj)) {
        return -1;
    }

    if (json_object_object_get_ex(obj, "command", &bt_cmd) == TRUE) {
        command = (char *)json_object_get_string(bt_cmd);
        BT_LOGI("common :: command %s \n", command);
        if (strcmp(command, "MODULE_ADDRESS") == 0) {
            bt_get_mac_addr(handle, reply);
        } else if (strcmp(command, "VISIBILITY") == 0) {
            json_object *bt_discoverable = NULL;
            bool discoverable;
            if (json_object_object_get_ex(obj, "discoverable", &bt_discoverable) == TRUE) {
                discoverable = json_object_get_boolean(bt_discoverable);
                bt_set_discoveryable(bt, discoverable, reply);
            }
        } else if (strcmp(command, "PAIREDLIST") == 0) {
            bt_get_paired_list(bt, reply);
        } else if (strcmp(command, "REMOVE_DEV") == 0) {
            json_object *bt_addr = NULL;
            if (json_object_object_get_ex(obj, "address", &bt_addr) == TRUE) {
                bt_remove_dev(bt, (char *)json_object_get_string(bt_addr));
            }
        } else if (strcmp(command, "DISCOVERY") == 0) {
            json_object *bt_type = NULL;
            int type = BT_ALL;
            if (json_object_object_get_ex(obj, "type", &bt_type) == TRUE) {
                type = json_object_get_int(bt_type);
            }
            bt_discovery(handle, type);
        } else if (strcmp(command, "CANCEL_DISCOVERY") == 0) {
            bt_cancel_discovery(handle);
        }
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <bsa_rokid/bsa_api.h>
#include <bsa_rokid/btm_api.h>
#include "app_dm.h"
#include "app_manager.h"
#include "ble.h"

/*
 * Global Variables
 */
int g_ble_enable_status = 0;


int string_to_uuid16 (UINT16 *uuid16, const char *uuid)
{
    char **endptr = NULL;
    if (!uuid || !uuid16) {
        return -1;
    }
    if (strlen(uuid) != 6) {
        BT_LOGE("error uuid %s", uuid);
        return -1;
    }
    BT_LOGD("uuid %s", uuid);
    *uuid16 = strtoul(uuid, endptr, 16);
    BT_LOGD("uuid16: 0x%X", *uuid16);
    return 0;
}

int string_to_uuid32 (UINT32 *uuid32, const char *uuid)
{
    char **endptr = NULL;
    if (!uuid || !uuid32) {
        return -1;
    }
    if (strlen(uuid) != 10) {
        BT_LOGE("error uuid %s", uuid);
        return -1;
    }
    BT_LOGD("uuid %s", uuid);
    *uuid32 = strtoul(uuid, endptr, 16);
    BT_LOGD("uuid32: 0x%X", *uuid32);
    return 0;
}

int string_to_uuid128 (UINT8 *uuid128, const char *uuid)
{
    uint32_t data0, data4;
    uint16_t data1, data2, data3, data5;
    char **endptr = NULL;
    char tmp[5];
    int i;

    if (!uuid || !uuid128) {
        return -1;
    }
    if (strlen(uuid) != (MAX_LEN_UUID_STR - 1)) {
        BT_LOGE("error uuid %s", uuid);
        return -1;
    }
    BT_LOGD("uuid %s", uuid);
    if (sscanf(uuid, "%08x-%04hx-%04hx-%04hx-%08x%04hx",
				&data0, &data1, &data2,
				&data3, &data4, &data5) != 6)
        return -1;

    tmp[0] = '0';
    tmp[1] = 'x';
    memset(tmp+2, 0, sizeof(tmp)-2);
    for (i=0; i<16;) {
        if (*uuid == '-') {
            uuid++;
            continue;
        };
        strncpy(tmp+2, uuid, 2);
        uuid += 2;
        uuid128[15-i] = strtoul(tmp, endptr, 16);
        BT_LOGD("uuid128[%d]:0x%X", 15-i, uuid128[15-i]);
        i++;
    }
    return 0;
}

int uuid128_to_string (char *uuid, UINT8 *uuid128)
{

    if (!uuid || !uuid128) {
        return -1;
    }
    snprintf(uuid, MAX_LEN_UUID_STR, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                            uuid128[15], uuid128[14], uuid128[13], uuid128[12],
                            uuid128[11], uuid128[10], uuid128[9], uuid128[8],
                            uuid128[7], uuid128[6], uuid128[5], uuid128[4],
                            uuid128[3], uuid128[2], uuid128[1], uuid128[0]);

    return 0;
}

int uuid16_to_string (char *uuid, UINT16 uuid16)
{
    if (!uuid) {
        return -1;
    }
    snprintf(uuid, MAX_LEN_UUID_STR, "0000%04hX-0000-1000-8000-00805F9B34FB",
                            uuid16);

    return 0;
}

int uuid32_to_string (char *uuid, UINT32 uuid32)
{

    if (!uuid) {
        return -1;
    }
    snprintf(uuid, MAX_LEN_UUID_STR, "%08X-0000-1000-8000-00805F9B34FB",
                            uuid32);

    return 0;
}

int uuid128_normal_to_uuid128_bsa (UINT8 *uuid128_bsa, UINT8 *uuid128_nor)
{
    int i;
    if (!uuid128_bsa || uuid128_nor) return -1;
    for ( i = 0; i < 16; i++) {
        uuid128_bsa[15-i] = uuid128_nor[i];
    }
    return 0;
}

char *app_ble_display_service_name(UINT16 uuid)
{
    switch(uuid)
    {
    case BSA_BLE_UUID_SERVCLASS_GAP_SERVER:
        return ("UUID_SERVCLASS_GAP_SERVER");
    case BSA_BLE_UUID_SERVCLASS_GATT_SERVER:
        return ("UUID_SERVCLASS_GATT_SERVER");
    case BSA_BLE_UUID_SERVCLASS_IMMEDIATE_ALERT:
        return("UUID_SERVCLASS_IMMEDIATE_ALERT");
    case BSA_BLE_UUID_SERVCLASS_LINKLOSS:
        return ("UUID_SERVCLASS_LINKLOSS");
    case BSA_BLE_UUID_SERVCLASS_TX_POWER:
        return ("UUID_SERVCLASS_TX_POWER");
    case BSA_BLE_UUID_SERVCLASS_CURRENT_TIME:
        return ("UUID_SERVCLASS_CURRENT_TIME");
    case BSA_BLE_UUID_SERVCLASS_DST_CHG:
        return ("UUID_SERVCLASS_DST_CHG");
    case BSA_BLE_UUID_SERVCLASS_REF_TIME_UPD:
        return ("UUID_SERVCLASS_REF_TIME_UPD");
    case BSA_BLE_UUID_SERVCLASS_GLUCOSE:
        return ("UUID_SERVCLASS_GLUCOSE");
    case BSA_BLE_UUID_SERVCLASS_HEALTH_THERMOMETER:
        return ("UUID_SERVCLASS_HEALTH_THERMOMETER");
    case BSA_BLE_UUID_SERVCLASS_DEVICE_INFORMATION:
        return ("UUID_SERVCLASS_DEVICE_INFORMATION");
    case BSA_BLE_UUID_SERVCLASS_NWA:
        return ("UUID_SERVCLASS_NWA");
    case BSA_BLE_UUID_SERVCLASS_PHALERT:
        return ("UUID_SERVCLASS_PHALERT");
    case BSA_BLE_UUID_SERVCLASS_HEART_RATE:
        return ("UUID_SERVCLASS_HEART_RATE");
    case BSA_BLE_UUID_SERVCLASS_BATTERY_SERVICE:
        return ("UUID_SERVCLASS_BATTERY_SERVICE");
    case BSA_BLE_UUID_SERVCLASS_BLOOD_PRESSURE:
        return ("UUID_SERVCLASS_BLOOD_PRESSURE");
    case BSA_BLE_UUID_SERVCLASS_ALERT_NOTIFICATION_SERVICE:
        return ("UUID_SERVCLASS_ALERT_NOTIFICATION_SERVICE");
    case BSA_BLE_UUID_SERVCLASS_HUMAN_INTERFACE_DEVICE:
        return ("UUID_SERVCLASS_HUMAN_INTERFACE_DEVICE");
    case BSA_BLE_UUID_SERVCLASS_SCAN_PARAMETERS:
        return ("UUID_SERVCLASS_SCAN_PARAMETERS");
    case BSA_BLE_UUID_SERVCLASS_RUNNING_SPEED_AND_CADENCE:
        return ("UUID_SERVCLASS_RUNNING_SPEED_AND_CADENCE");
    case BSA_BLE_UUID_SERVCLASS_CYCLING_SPEED_AND_CADENCE:
        return ("UUID_SERVCLASS_CYCLING_SPEED_AND_CADENCE");
    case BSA_BLE_UUID_SERVCLASS_TEST_SERVER:
        return ("UUID_SERVCLASS_TEST_SERVER");

    default:
        break;
    }

    return ("UNKNOWN");
}

int ble_disable(int enable_status)
{
    tBSA_STATUS status;
    tBSA_BLE_DISABLE param;

    if (g_ble_enable_status) {
        BT_LOGD("0x%x  : g_status:0x%x", enable_status, g_ble_enable_status);
        g_ble_enable_status &= ~enable_status;
        if (g_ble_enable_status == 0) {
            /* make device non discoverable & non connectable */
            app_dm_set_ble_visibility(FALSE, FALSE);

            BSA_BleDisableInit(&param);
            status = BSA_BleDisable(&param);
            if (status != BSA_SUCCESS)
            {
                BT_LOGE("Unable to disable BLE status:%d", status);
                return status;
            }
        }
    }
    return 0;
}

int ble_enable(int enable_status)
{
    tBSA_STATUS status;
    tBSA_BLE_ENABLE enable_param;

    if (g_ble_enable_status) {
        BT_LOGW("0x%x already enabled: 0x%x", enable_status, g_ble_enable_status);
        g_ble_enable_status |= enable_status;
        return 0;
    }
    BT_LOGI("ble_enable");
    BSA_BleEnableInit(&enable_param);

    status = BSA_BleEnable(&enable_param);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("ERROR::Unable to enable BLE status:%d\n", status);
        return -1;
    }
    g_ble_enable_status |= enable_status;
    return 0;
}
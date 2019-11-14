#ifndef __BLE_H__
#define __BLE_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "app_dm.h"

#define BLE_SERVER_STATUS (1 << 0)
#define BLE_CLIENT_STATUS (1 << 1)


int string_to_uuid16 (UINT16 *uuid16, const char *uuid);

int string_to_uuid32 (UINT32 *uuid32, const char *uuid);

int string_to_uuid128 (UINT8 *uuid128, const char *uuid);

int uuid128_to_string (char *uuid, UINT8 *uuid128);

int uuid16_to_string (char *uuid, UINT16 uuid16);

int uuid32_to_string (char *uuid, UINT32 uuid32);

int uuid128_normal_to_uuid128_bsa (UINT8 *uuid128_bsa, UINT8 *uuid128_nor);

char *app_ble_display_service_name(UINT16 uuid);

int ble_disable(int enable_status);

int ble_enable(int enable_status);
#endif

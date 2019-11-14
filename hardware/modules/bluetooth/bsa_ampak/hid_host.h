/*****************************************************************************
 **
 **  Name:           app_hh.h
 **
 **  Description:    Bluetooth HID Host application
 **
 **  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef __APP_HH_H__
#define __APP_HH_H__

#include <hardware/bt_common.h>
/* for BSA functions and types */
#include <bsa_rokid/bsa_api.h>
/* for tAPP_XML_REM_DEVICE */
#include "app_xml_param.h"

/*
 * Definitions
 */
 
 #define APP_BTHID_INCLUDED TRUE
 
#ifndef APP_HH_DEVICE_MAX
#define APP_HH_DEVICE_MAX               10
#endif

#define APP_HH_VOICE_FILE_NAME_FORMAT   "/data/app_hh_voice_%u.wav"

/* Information mask of the HH devices */
#define APP_HH_DEV_USED     0x01    /* Indicates if this element is allocated */
#define APP_HH_DEV_OPENED   0x02    /* Indicates if this device is connected */

typedef struct
{
    UINT8 info_mask;
    char name[249];
    BD_ADDR bd_addr;
    tBSA_HH_HANDLE handle;
#if defined(APP_BTHID_INCLUDED) && (APP_BTHID_INCLUDED == TRUE)
    int bthid_fd;
#endif
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    BOOLEAN le_hid;
#endif
} tAPP_HH_DEVICE;

struct hid_host_t
{
    void *caller;
    tUIPC_CH_ID uipc_channel;
    tAPP_HH_DEVICE devices[APP_HH_DEVICE_MAX];
    tBSA_HH_HANDLE mip_handle;
    UINT8 mip_device_nb;
    tBSA_SEC_AUTH sec_mask_in;
    bool enable;

    hh_callbacks_t listener;
    void *          listener_handle;
    //pthread_mutex_t mutex;
};

typedef struct hid_host_t HidHost;

/*
 * Global variables
 */


/*
 * Interface functions
 */
int app_hh_get_dscpinfo(UINT8 handle);
int app_hh_change_proto_mode(tBSA_HH_HANDLE handle, tBSA_HH_PROTO_MODE mode);

HidHost *hh_create(void *caller);

int hh_set_listener(HidHost *hh, hh_callbacks_t listener, void *listener_handle);
int hh_destroy(HidHost *hh);
int hh_enable(HidHost *hh);
int hh_disable(HidHost *hh);
int app_hh_open(HidHost *hh, BD_ADDR bd_addr, tBSA_HH_PROTO_MODE mode);
int app_hh_disconnect(HidHost *hh, BD_ADDR bd_addr);
int app_hh_disconnect_all(HidHost *hh);
int app_hh_get_connected_devices(HidHost *hh, BTDevice *dev_array, int arr_len);
int app_hh_remove_dev(HidHost *hh, BD_ADDR bd_addr);
#endif /*__APP_HH_H__*/
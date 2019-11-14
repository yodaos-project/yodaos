/*****************************************************************************
**
**  Name:           app_manager.c
**
**  Description:    Bluetooth Manager application
**
**  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#include <bsa_rokid/gki.h>
#include <bsa_rokid/uipc.h>

#include <bsa_rokid/bsa_api.h>

#include "app_xml_utils.h"

#include "app_disc.h"
#include "app_utils.h"

#include "app_services.h"
#include "app_mgt.h"

#include "ble_server.h"

#include "app_manager.h"
#include "app_sec.h"

/* Default BdAddr */
#define APP_DEFAULT_BD_ADDR             {0xBE, 0xEF, 0xBE, 0xEF, 0x00, 0x01}

/* Default local Name */
#define APP_DEFAULT_BT_NAME             "rokid-bt"

/* Default COD SetTopBox (Major Service = none) (MajorDevclass = Audio/Video) (Minor=STB) */
#define APP_DEFAULT_CLASS_OF_DEVICE     {0x24, 0x04, 0x04}

#define APP_DEFAULT_ROOT_PATH           "./pictures"

#define APP_DEFAULT_PIN_CODE            "0000"
#define APP_DEFAULT_PIN_LEN             4

//#define APP_XML_CONFIG_FILE_PATH            "./bt_config.xml"
//#define APP_XML_REM_DEVICES_FILE_PATH       "./bt_devices.xml"
//#define APP_XML_LINK_STATUS_FILE_PATH       "./bt_link.xml"


/*
 * Simple Pairing Input/Output Capabilities:
 * BSA_SEC_IO_CAP_OUT  =>  DisplayOnly
 * BSA_SEC_IO_CAP_IO   =>  DisplayYesNo
 * BSA_SEC_IO_CAP_IN   =>  KeyboardOnly
 * BSA_SEC_IO_CAP_NONE =>  NoInputNoOutput
* * #if BLE_INCLUDED == TRUE && SMP_INCLUDED == TRUE
 * BSA_SEC_IO_CAP_KBDISP =>  KeyboardDisplay
 * #endif
 */
#ifndef APP_SEC_IO_CAPABILITIES
#define APP_SEC_IO_CAPABILITIES BSA_SEC_IO_CAP_NONE
//#define APP_SEC_IO_CAPABILITIES BSA_SEC_IO_CAP_IO
#endif

/*
 * Global variables
 */
tAPP_XML_CONFIG         app_xml_config;
//BD_ADDR                 app_sec_db_addr;    /* BdAddr of peer device requesting SP */
BOOLEAN                 app_sec_is_ble;

tAPP_MGR_CB app_mgr_cb;

tBSA_SEC_SET_REMOTE_OOB bsa_sec_set_remote_oob;

tBSA_SEC_PASSKEY_REPLY g_passkey_reply;

int A2DP_SINK_OPEN_PENDING_LINK_UP = 0;
BD_ADDR A2DP_SINK_OPEN_PENDING_LINK_ADDR;

/*
 * Local functions
 */
char *app_mgr_get_dual_stack_mode_desc(void);

/*******************************************************************************
 **
 ** Function         app_mgr_read_config
 **
 ** Description      This function is used to read the XML bluetooth configuration file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_read_config(void)
{
    int status;

    status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
    if (status < 0)
    {
        BT_LOGE("app_xml_read_cfg failed:%d", status);
        return -1;
    }
    else
    {
        BT_LOGD("Enable:%d", app_xml_config.enable);
        BT_LOGD("Discoverable:%d", app_xml_config.discoverable);
        BT_LOGD("Connectable:%d", app_xml_config.connectable);
        BT_LOGD("Name:%s", app_xml_config.name);
        BT_LOGD("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
                app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
                app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
        BT_LOGD("ClassOfDevice:%02x:%02x:%02x => %s", app_xml_config.class_of_device[0],
                app_xml_config.class_of_device[1], app_xml_config.class_of_device[2],
                app_get_cod_string(app_xml_config.class_of_device));
        BT_LOGD("RootPath:%s", app_xml_config.root_path);

        BT_LOGD("Default PIN (%d):%s", app_xml_config.pin_len, app_xml_config.pin_code);
    }
    return 0;
}



/*******************************************************************************
 **
 ** Function         app_mgr_write_config
 **
 ** Description      This function is used to write the XML bluetooth configuration file
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_write_config(void)
{
    int status;

    status = app_xml_write_cfg(APP_XML_CONFIG_FILE_PATH, &app_xml_config);
    if (status < 0)
    {
        BT_LOGE("app_xml_write_cfg failed:%d", status);
        return -1;
    }
    else
    {
        BT_LOGD("Enable:%d", app_xml_config.enable);
        BT_LOGD("Discoverable:%d", app_xml_config.discoverable);
        BT_LOGD("Connectable:%d", app_xml_config.connectable);
        BT_LOGD("Name:%s", app_xml_config.name);
        BT_LOGD("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                app_xml_config.bd_addr[0], app_xml_config.bd_addr[1],
                app_xml_config.bd_addr[2], app_xml_config.bd_addr[3],
                app_xml_config.bd_addr[4], app_xml_config.bd_addr[5]);
        BT_LOGD("ClassOfDevice:%02x:%02x:%02x", app_xml_config.class_of_device[0],
                app_xml_config.class_of_device[1], app_xml_config.class_of_device[2]);
        BT_LOGD("RootPath:%s", app_xml_config.root_path);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_remote_devices
 **
 ** Description      This function is used to read the XML bluetooth remote device file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_read_remote_devices(void)
{
    int status;
    int index;

    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
    {
        app_xml_remote_devices_db[index].in_use = FALSE;
    }

    status = app_xml_read_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        BT_LOGE("app_xml_read_db failed:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_write_remote_devices
 **
 ** Description      This function is used to write the XML bluetooth remote device file
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_write_remote_devices(void)
{
    int status;

    status = app_xml_write_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        BT_LOGE("app_xml_write_db failed:%d", status);
        return -1;
    }
    else
    {
        BT_LOGD("app_xml_write_db ok");
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_bt_config
 **
 ** Description      This function is used to get the bluetooth configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_set_bt_config(BOOLEAN enable)
{
    BT_LOGI("entry");
    int status;
    tBSA_DM_SET_CONFIG bt_config;

    /* Init config parameter */
    status = BSA_DmSetConfigInit(&bt_config);

    /*
     * Set bluetooth configuration (from XML file)
     */
    bt_config.enable = enable;
    bt_config.discoverable = app_xml_config.discoverable;
    bt_config.connectable = app_xml_config.connectable;
    bdcpy(bt_config.bd_addr, app_xml_config.bd_addr);

    snprintf((char *)bt_config.name, sizeof(bt_config.name), "%s", app_xml_config.name);
    memcpy(bt_config.class_of_device, app_xml_config.class_of_device, sizeof(DEV_CLASS));
    bt_config.config_mask &= ~BSA_DM_CONFIG_BDADDR_MASK;//rokid define
    
    BT_LOGD("Enable:%d", bt_config.enable);
    BT_LOGD("Discoverable:%d", bt_config.discoverable);
    BT_LOGD("Connectable:%d", bt_config.connectable);
    BT_LOGD("Name:%s", bt_config.name);
    BT_LOGD("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            bt_config.bd_addr[0], bt_config.bd_addr[1],
            bt_config.bd_addr[2], bt_config.bd_addr[3],
            bt_config.bd_addr[4], bt_config.bd_addr[5]);
    BT_LOGD("ClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
            bt_config.class_of_device[1], bt_config.class_of_device[2]);
    BT_LOGD("First host disabled channel:%d", bt_config.first_disabled_channel);
    BT_LOGD("Last host disabled channel:%d", bt_config.last_disabled_channel);

    /* Apply BT config */
    status = BSA_DmSetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed:%d", status);
        return(-1);
    }
    BT_LOGI("done");
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_get_bt_config
 **
 ** Description      This function is used to get the bluetooth configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_get_bt_config(void)
{
    int status;
    tBSA_DM_GET_CONFIG bt_config;

    /*
     * Get bluetooth configuration
     */
    status = BSA_DmGetConfigInit(&bt_config);
    status = BSA_DmGetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmGetConfig failed:%d", status);
        return(-1);
    }
    if (bdcmp(bt_config.bd_addr, app_xml_config.bd_addr)) {
        /*save  module bd addr*/
        BT_LOGI("save module bd addr");
        bdcpy(app_xml_config.bd_addr, bt_config.bd_addr);
        app_mgr_write_config();
    }
    BT_LOGD("Enable:%d", bt_config.enable);
    BT_LOGD("Discoverable:%d", bt_config.discoverable);
    BT_LOGD("Connectable:%d", bt_config.connectable);
    BT_LOGD("Name:%s", bt_config.name);
    BT_LOGD("Bdaddr %02x:%02x:%02x:%02x:%02x:%02x",
             bt_config.bd_addr[0], bt_config.bd_addr[1],
             bt_config.bd_addr[2], bt_config.bd_addr[3],
             bt_config.bd_addr[4], bt_config.bd_addr[5]);
    BT_LOGD("ClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
            bt_config.class_of_device[1], bt_config.class_of_device[2]);
    BT_LOGD("First host disabled channel:%d", bt_config.first_disabled_channel);
    BT_LOGD("Last host disabled channel:%d", bt_config.last_disabled_channel);

    return 0;

}

/*******************************************************************************
 **
 ** Function         app_mgr_read_oob
 **
 ** Description      This function is used to read local OOB data from local controller
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_read_oob_data()
{
    tBSA_SEC_READ_OOB bsa_sec_read_oob;
    if (BSA_SecReadOOBInit(&bsa_sec_read_oob) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }

    if (BSA_SecReadOOB(&bsa_sec_read_oob) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_local_oob_evt_data
 **
 ** Description      This function prints out local OOB data returned by the
 **                  local controller
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/

void app_mgr_local_oob_evt_data(tBSA_SEC_LOCAL_OOB_MSG* p_data)
{
    char szt[128];
    BT_OCTET16 p_c;
    BT_OCTET16 p_r;
    memcpy(p_c, p_data->c, 16);
    memcpy(p_r, p_data->r, 16);

    memset(szt,0, sizeof(szt));
    sprintf(szt,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    p_c[0],p_c[1],p_c[2],p_c[3],p_c[4],p_c[5],p_c[6],p_c[7],
    p_c[8],p_c[9],p_c[10],p_c[11],p_c[12],p_c[13],p_c[14],p_c[15]);
    BT_LOGD("app_mgr_local_oob_evt_data: C Hash: %s", szt);

    memset(szt,0, sizeof(szt));
    sprintf(szt,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
    p_r[0],p_r[1],p_r[2],p_r[3],p_r[4],p_r[5],p_r[6],p_r[7],
    p_r[8],p_r[9],p_r[10],p_r[11],p_r[12],p_r[13],p_r[14],p_r[15]);
    BT_LOGD("app_mgr_local_oob_evt_data: R Randomizer: %s", szt);

    BT_LOGD("app_mgr_local_oob_evt_data: OOB Valid: %d", p_data->valid);
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_remote_oob
 **
 ** Description      This function is used to set OOB data for peer device
 **                  During pairing stack uses this information to pair
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_remote_oob()
{
    int uarr[16];
    char szInput[64];
    int  i;

    if (BSA_SecSetRemoteOOBInit(&bsa_sec_set_remote_oob) != BSA_SUCCESS)
    {
        BT_LOGE("app_mgr_set_remote_oob failed");
        return;
    }

    BT_LOGI("Enter BDA of Peer device: AABBCCDDEEFF");
    memset(szInput, 0, sizeof(szInput));
    app_get_string("Enter BDA: ", szInput, sizeof(szInput));
    sscanf (szInput, "%02x%02x%02x%02x%02x%02x",
                   &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5]);

    for(i=0; i < 6; i++)
        bsa_sec_set_remote_oob.bd_addr[i] = (UINT8) uarr[i];

    bsa_sec_set_remote_oob.result = TRUE;   /* TRUE to accept; FALSE to reject */

    BT_LOGI("Enter 16 byte C Hash in this format: FA87C0D0AFAC11DE8A390800200C9A66");
    memset(szInput, 0, sizeof(szInput));
    app_get_string("C Hash: ", szInput, sizeof(szInput));
    sscanf (szInput, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                   &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
            &uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

    for(i=0; i < 16; i++)
        bsa_sec_set_remote_oob.p_c[i] = (UINT8) uarr[i];

    BT_LOGI("Enter 16 byte R Randomizer in this format: FA87C0D0AFAC11DE8A390800200C9A66");
    memset(szInput, 0, sizeof(szInput));
    app_get_string("R Randomizer: ", szInput, sizeof(szInput));
    sscanf (szInput, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                   &uarr[0], &uarr[1], &uarr[2], &uarr[3], &uarr[4], &uarr[5], &uarr[6], &uarr[7], &uarr[8],
            &uarr[9], &uarr[10], &uarr[11], &uarr[12], &uarr[13], &uarr[14], &uarr[15]);

    for(i=0; i < 16; i++)
        bsa_sec_set_remote_oob.p_r[i] = (UINT8) uarr[i];

    if (BSA_SecSetRemoteOOB(&bsa_sec_set_remote_oob) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecSetRemoteOOB failed");
        return;
    }
}

static tBSA_SEC_PIN_CODE_REPLY g_pin_code_reply;

/*******************************************************************************
 **
 ** Function         app_mgr_security_callback
 **
 ** Description      Security callback
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_security_callback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
    int status;
    int indexDisc;
    tBSA_SEC_PIN_CODE_REPLY pin_code_reply;
    tBSA_SEC_AUTH_REPLY autorize_reply;

    BT_LOGD("event:%d", event);

    switch(event)
    {
    case BSA_SEC_LINK_UP_EVT:       /* A device is physically connected (for info) */
        BT_LOGI("BSA_SEC_LINK_UP_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->link_up.bd_addr[0], p_data->link_up.bd_addr[1],
                p_data->link_up.bd_addr[2], p_data->link_up.bd_addr[3],
                p_data->link_up.bd_addr[4], p_data->link_up.bd_addr[5]);

        BT_LOGI("ClassOfDevice:%02x:%02x:%02x => %s",
                p_data->link_up.class_of_device[0],
                p_data->link_up.class_of_device[1],
                p_data->link_up.class_of_device[2],
                app_get_cod_string(p_data->link_up.class_of_device));
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
        BT_LOGD("LinkType: %d", p_data->link_up.link_type);
#endif
        if (rokidbt_a2dp_sink_get_open_pending_addr(_global_bt_ctx, A2DP_SINK_OPEN_PENDING_LINK_ADDR) == 1) {
            if (bdcmp(A2DP_SINK_OPEN_PENDING_LINK_ADDR, p_data->link_up.bd_addr) == 0) {
                A2DP_SINK_OPEN_PENDING_LINK_UP = 1;
                BT_LOGI("A2DP_SINK_OPEN_PENDING_LINK_UP");
            }
        }

        break;
    case BSA_SEC_LINK_DOWN_EVT:     /* A device is physically disconnected (for info)*/
        BT_LOGI("BSA_SEC_LINK_DOWN_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
                p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
                p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5]);
        BT_LOGI("Reason: %d", p_data->link_down.status);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
        BT_LOGD("LinkType: %d", p_data->link_down.link_type);
#endif

        if (bdcmp(A2DP_SINK_OPEN_PENDING_LINK_ADDR, p_data->link_down.bd_addr) == 0) {
            A2DP_SINK_OPEN_PENDING_LINK_UP = 0;
            BT_LOGI("A2DP_SINK_OPEN_PENDING_LINK down");
        }

        break;

    case BSA_SEC_PIN_REQ_EVT:
        BT_LOGD("BSA_SEC_PIN_REQ_EVT (Pin Code Request) received from: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->pin_req.bd_addr[0], p_data->pin_req.bd_addr[1],
                p_data->pin_req.bd_addr[2], p_data->pin_req.bd_addr[3],
                p_data->pin_req.bd_addr[4],p_data->pin_req.bd_addr[5]);

        BT_LOGD("call BSA_SecPinCodeReply pin:%s len:%d",
                app_xml_config.pin_code, app_xml_config.pin_len);

        BSA_SecPinCodeReplyInit(&pin_code_reply);
        bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
        pin_code_reply.pin_len = app_xml_config.pin_len;
        strncpy((char *)pin_code_reply.pin_code, (char *)app_xml_config.pin_code,
                app_xml_config.pin_len);
        /* note that this code will not work if pin_len = 16 */
        pin_code_reply.pin_code[PIN_CODE_LEN-1] = '\0';
        status = BSA_SecPinCodeReply(&pin_code_reply);
        break;

    case BSA_SEC_AUTH_CMPL_EVT:
        BT_LOGD("BSA_SEC_AUTH_CMPL_EVT (name=%s,success=%d)",
                   p_data->auth_cmpl.bd_name, p_data->auth_cmpl.success);
        if (!p_data->auth_cmpl.success)
        {
            BT_LOGD("    fail_reason=%d", p_data->auth_cmpl.fail_reason);
        }
        BT_LOGD("    bd_addr:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1], p_data->auth_cmpl.bd_addr[2],
                p_data->auth_cmpl.bd_addr[3], p_data->auth_cmpl.bd_addr[4], p_data->auth_cmpl.bd_addr[5]);
        if (p_data->auth_cmpl.key_present != FALSE)
        {
            BT_LOGD("    LinkKey:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                    p_data->auth_cmpl.key[0], p_data->auth_cmpl.key[1], p_data->auth_cmpl.key[2], p_data->auth_cmpl.key[3],
                    p_data->auth_cmpl.key[4], p_data->auth_cmpl.key[5], p_data->auth_cmpl.key[6], p_data->auth_cmpl.key[7],
                    p_data->auth_cmpl.key[8], p_data->auth_cmpl.key[9], p_data->auth_cmpl.key[10], p_data->auth_cmpl.key[11],
                    p_data->auth_cmpl.key[12], p_data->auth_cmpl.key[13], p_data->auth_cmpl.key[14], p_data->auth_cmpl.key[15]);
        }

        /* If success */
        if (p_data->auth_cmpl.success != 0)
        {
            /* Read the Remote device xml file to have a fresh view */
            app_mgr_read_remote_devices();

            if (strlen((char *)p_data->auth_cmpl.bd_name) > 0) {
                app_xml_update_name_db(app_xml_remote_devices_db,
                                       APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                       p_data->auth_cmpl.bd_addr,
                                       p_data->auth_cmpl.bd_name);
            }

            if (p_data->auth_cmpl.key_present != FALSE)
            {
                app_xml_update_key_db(app_xml_remote_devices_db,
                                      APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                      p_data->auth_cmpl.bd_addr,
                                      p_data->auth_cmpl.key,
                                      p_data->auth_cmpl.key_type);
            }

            /* Unfortunately, the BSA_SEC_AUTH_CMPL_EVT does not contain COD, let's look in the Discovery database */
            for (indexDisc = 0 ; indexDisc < BT_DISC_NB_DEVICES ; indexDisc++)
            {
                if ((_global_bt_ctx->discovery_devs[indexDisc].in_use != FALSE) && 
                    (bdcmp(_global_bt_ctx->discovery_devs[indexDisc].device.bd_addr, p_data->auth_cmpl.bd_addr) == 0))

                {
                    app_xml_update_cod_db(app_xml_remote_devices_db,
                                          APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                          p_data->auth_cmpl.bd_addr,
                                          _global_bt_ctx->discovery_devs[indexDisc].device.class_of_device);
                }
            }

            status = app_mgr_write_remote_devices();
            if (status < 0)
            {
                BT_LOGE("app_mgr_write_remote_devices failed:%d", status);
            }

#ifdef BSA_PEER_IOS
            /* Start device info discovery */
            //app_disc_start_dev_info(p_data->auth_cmpl.bd_addr, NULL);
#endif
        }
        break;

    case BSA_SEC_BOND_CANCEL_CMPL_EVT:
        BT_LOGD("BSA_SEC_BOND_CANCEL_CMPL_EVT status=%d",
                   p_data->bond_cancel.status);
        break;

    case BSA_SEC_AUTHORIZE_EVT:  /* Authorization request */
        BT_LOGD("BSA_SEC_AUTHORIZE_EVT");
        BT_LOGD("    Remote device:%s", p_data->authorize.bd_name);
        BT_LOGD("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->authorize.bd_addr[0], p_data->authorize.bd_addr[1],
                p_data->authorize.bd_addr[2], p_data->authorize.bd_addr[3],
                p_data->authorize.bd_addr[4],p_data->authorize.bd_addr[5]);
        BT_LOGD("    Request access to service:%x (%s)",
                (int)p_data->authorize.service,
                app_service_id_to_string(p_data->authorize.service));
        BT_LOGD("    Access Granted (permanently)");
        status = BSA_SecAuthorizeReplyInit(&autorize_reply);
        bdcpy(autorize_reply.bd_addr, p_data->authorize.bd_addr);
        autorize_reply.trusted_service = p_data->authorize.service;
        autorize_reply.auth = BSA_SEC_AUTH_PERM;
        status = BSA_SecAuthorizeReply(&autorize_reply);
        /*
         * Update XML database
         */
        /* Read the Remote device xml file to have a fresh view */
        app_mgr_read_remote_devices();
        /* Add AV service for this devices in XML database */
        app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->authorize.bd_addr,
                1 << p_data->authorize.service);

        if (strlen((char *)p_data->authorize.bd_name) > 0)
            app_xml_update_name_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                    p_data->authorize.bd_addr, p_data->authorize.bd_name);

        /* Update database => write on disk */
        status = app_mgr_write_remote_devices();
        if (status < 0)
        {
            BT_LOGE("app_mgr_write_remote_devices failed:%d", status);
        }
        break;

    case BSA_SEC_SP_CFM_REQ_EVT: /* Simple Pairing confirm request */
        BT_LOGD("BSA_SEC_SP_CFM_REQ_EVT");
        BT_LOGD("    Remote device:%s", p_data->cfm_req.bd_name);
        BT_LOGD("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->cfm_req.bd_addr[0], p_data->cfm_req.bd_addr[1],
                p_data->cfm_req.bd_addr[2], p_data->cfm_req.bd_addr[3],
                p_data->cfm_req.bd_addr[4], p_data->cfm_req.bd_addr[5]);

        BT_LOGD("    ClassOfDevice:%02x:%02x:%02x => %s",
                p_data->cfm_req.class_of_device[0], p_data->cfm_req.class_of_device[1], p_data->cfm_req.class_of_device[2],
                app_get_cod_string(p_data->cfm_req.class_of_device));
        BT_LOGD("    Just Work:%s", p_data->cfm_req.just_works == TRUE ? "TRUE" : "FALSE");
        BT_LOGD("    Numeric Value:%d", p_data->cfm_req.num_val);
        BT_LOGD("    Remote is %s device", p_data->cfm_req.is_ble ? "LE" : "BR/EDR");
        //BT_LOGD("    You must accept or refuse using menu\n");
        //bdcpy(app_sec_db_addr, p_data->cfm_req.bd_addr);
        app_sec_is_ble = p_data->cfm_req.is_ble;

        {
            BT_LOGD("\tAccept it automatically");
            app_mgr_sp_cfm_reply(TRUE, p_data->cfm_req.bd_addr);
        }
        break;

    case BSA_SEC_SP_KEY_NOTIF_EVT: /* Simple Pairing Passkey Notification */
        BT_LOGD("BSA_SEC_SP_KEY_NOTIF_EVT");
        BT_LOGD("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1], p_data->key_notif.bd_addr[2],
                p_data->key_notif.bd_addr[3], p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
        BT_LOGD("    ClassOfDevice:%02x:%02x:%02x => %s",
                p_data->key_notif.class_of_device[0], p_data->key_notif.class_of_device[1], p_data->key_notif.class_of_device[2],
                app_get_cod_string(p_data->key_notif.class_of_device));
        BT_LOGD("    Numeric Value:%d", p_data->key_notif.passkey);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
        BT_LOGD("    LinkType: %d", p_data->key_notif.link_type);
#endif
        BT_LOGD("    You must enter this value on peer device's keyboard");
        break;

    case BSA_SEC_SP_KEY_REQ_EVT: /* Simple Pairing Passkey request Notification */
        BT_LOGD("BSA_SEC_SP_KEY_REQ_EVT");
        BT_LOGD("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1], p_data->key_notif.bd_addr[2],
                p_data->key_notif.bd_addr[3], p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
        BT_LOGD("    ClassOfDevice:%02x:%02x:%02x => %s",
                p_data->key_notif.class_of_device[0], p_data->key_notif.class_of_device[1], p_data->key_notif.class_of_device[2],
                app_get_cod_string(p_data->key_notif.class_of_device));
        BT_LOGD("    Numeric Value:%d", p_data->key_notif.passkey);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
        BT_LOGD("    LinkType: %d", p_data->key_notif.link_type);
#endif
        BT_LOGD("    You must enter this value on peer device's keyboard");

        memset(&g_passkey_reply, 0, sizeof(g_passkey_reply));
        bdcpy(g_passkey_reply.bd_addr, p_data->pin_req.bd_addr);
        break;

    case BSA_SEC_SP_KEYPRESS_EVT: /* Simple Pairing Key press notification event. */
        BT_LOGD("BSA_SEC_SP_KEYPRESS_EVT (type:%d)", p_data->key_press.notif_type);
        BT_LOGD("    bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->key_press.bd_addr[0], p_data->key_press.bd_addr[1], p_data->key_press.bd_addr[2],
                p_data->key_press.bd_addr[3], p_data->key_press.bd_addr[4], p_data->key_press.bd_addr[5]);
        break;

   case BSA_SEC_SP_RMT_OOB_EVT: /* Simple Pairing Remote OOB Data request. */
       BT_LOGD("BSA_SEC_SP_RMT_OOB_EVT received - Handled internally if Peer OOB already set");
       break;

    case BSA_SEC_LOCAL_OOB_DATA_EVT: /* Local OOB Data response */
        BT_LOGD("BSA_SEC_LOCAL_OOB_DATA_EVT received");
        app_mgr_local_oob_evt_data(&p_data->local_oob);
        break;

    case BSA_SEC_SUSPENDED_EVT: /* Connection Suspended */
        BT_LOGI("BSA_SEC_SUSPENDED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->suspended.bd_addr[0], p_data->suspended.bd_addr[1],
                p_data->suspended.bd_addr[2], p_data->suspended.bd_addr[3],
                p_data->suspended.bd_addr[4], p_data->suspended.bd_addr[5]);
        break;

    case BSA_SEC_RESUMED_EVT: /* Connection Resumed */
        BT_LOGI("BSA_SEC_RESUMED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->resumed.bd_addr[0], p_data->resumed.bd_addr[1],
                p_data->resumed.bd_addr[2], p_data->resumed.bd_addr[3],
                p_data->resumed.bd_addr[4], p_data->resumed.bd_addr[5]);
        break;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    case BSA_SEC_BLE_KEY_EVT: /* BLE KEY event */
        BT_LOGI("BSA_SEC_BLE_KEY_EVT");
        switch (p_data->ble_key.key_type)
        {
        case BSA_LE_KEY_PENC:
            BT_LOGD("\t key_type: BTM_LE_KEY_PENC(%d)", p_data->ble_key.key_type);
            APP_DUMP("LTK", p_data->ble_key.key_value.penc_key.ltk, 16);
            APP_DUMP("RAND", p_data->ble_key.key_value.penc_key.rand, 8);
            BT_LOGD("ediv: 0x%x:", p_data->ble_key.key_value.penc_key.ediv);
            BT_LOGD("sec_level: 0x%x:", p_data->ble_key.key_value.penc_key.sec_level);
            BT_LOGD("key_size: %d:", p_data->ble_key.key_value.penc_key.key_size);
            break;
        case BSA_LE_KEY_PID:
            BT_LOGD("\t key_type: BTM_LE_KEY_PID(%d)", p_data->ble_key.key_type);
            APP_DUMP("IRK", p_data->ble_key.key_value.pid_key.irk, 16);
            BT_LOGD("addr_type: 0x%x:", p_data->ble_key.key_value.pid_key.addr_type);
            BT_LOGD("static_addr: %02X-%02X-%02X-%02X-%02X-%02X",
                p_data->ble_key.key_value.pid_key.static_addr[0],
                p_data->ble_key.key_value.pid_key.static_addr[1],
                p_data->ble_key.key_value.pid_key.static_addr[2],
                p_data->ble_key.key_value.pid_key.static_addr[3],
                p_data->ble_key.key_value.pid_key.static_addr[4],
                p_data->ble_key.key_value.pid_key.static_addr[5]);
            break;
        case BSA_LE_KEY_PCSRK:
            BT_LOGD("\t key_type: BTM_LE_KEY_PCSRK(%d)", p_data->ble_key.key_type);
            BT_LOGD("counter: 0x%x:", p_data->ble_key.key_value.pcsrk_key.counter);
            APP_DUMP("CSRK", p_data->ble_key.key_value.pcsrk_key.csrk, 16);
            BT_LOGD("sec_level: %d:", p_data->ble_key.key_value.pcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LCSRK:
            BT_LOGD("\t key_type: BTM_LE_KEY_LCSRK(%d)", p_data->ble_key.key_type);
            BT_LOGD("counter: 0x%x:", p_data->ble_key.key_value.lcsrk_key.counter);
            BT_LOGD("div: %d:", p_data->ble_key.key_value.lcsrk_key.div);
            BT_LOGD("sec_level: 0x%x:", p_data->ble_key.key_value.lcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LENC:
            BT_LOGD("\t key_type: BTM_LE_KEY_LENC(%d)", p_data->ble_key.key_type);
            BT_LOGD("div: 0x%x:", p_data->ble_key.key_value.lenc_key.div);
            BT_LOGD("key_size: %d:", p_data->ble_key.key_value.lenc_key.key_size);
            BT_LOGD("sec_level: 0x%x:", p_data->ble_key.key_value.lenc_key.sec_level);
            break;
        case BSA_LE_KEY_LID:
            BT_LOGD("\t key_type: BTM_LE_KEY_LID(%d)", p_data->ble_key.key_type);
            break;
        default:
            BT_LOGD("\t key_type: Unknown key(%d)", p_data->ble_key.key_type);
            break;
        }

        /* Read the Remote device xml file to have a fresh view */
        app_mgr_read_remote_devices();

        app_xml_update_device_type_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->ble_key.bd_addr,
                BT_DEVICE_TYPE_BLE);

        app_xml_update_ble_key_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                p_data->ble_key.bd_addr,
                p_data->ble_key.key_value,
                p_data->ble_key.key_type);

        status = app_mgr_write_remote_devices();
        if (status < 0)
        {
            BT_LOGE("app_mgr_write_remote_devices failed:%d", status);
        }
        break;

    case BSA_SEC_BLE_PASSKEY_REQ_EVT:
        BT_LOGI("BSA_SEC_BLE_PASSKEY_REQ_EVT (Passkey Request) received from:");
        BT_LOGI("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->ble_passkey_req.bd_addr[0], p_data->ble_passkey_req.bd_addr[1],
                p_data->ble_passkey_req.bd_addr[2], p_data->ble_passkey_req.bd_addr[3],
                p_data->ble_passkey_req.bd_addr[4], p_data->ble_passkey_req.bd_addr[5]);

        memset(&g_pin_code_reply, 0, sizeof(g_pin_code_reply));
        bdcpy(g_pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
        break;
#endif

    case BSA_SEC_RSSI_EVT:
        BT_LOGI("BSA_SEC_RSSI_EVT received for BdAddr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->sig_strength.bd_addr[0], p_data->sig_strength.bd_addr[1],
                p_data->sig_strength.bd_addr[2], p_data->sig_strength.bd_addr[3],
                p_data->sig_strength.bd_addr[4], p_data->sig_strength.bd_addr[5]);
        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RSSI)
        {
            if (p_data->sig_strength.link_type == BSA_TRANSPORT_BR_EDR)
            {
                /* For BasicRate & EnhancedDataRate links, the RSSI is an attenuation (dB) */
                BT_LOGI("\tRSSI: %d (dB)", p_data->sig_strength.rssi_value);
            }
            else
            {
                /* For Low Energy links, the RSSI is a strength (dBm) */
                BT_LOGI("\tRSSI: %d (dBm)", p_data->sig_strength.rssi_value);
            }
        }

        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RAW_RSSI)
        {
            /* Raw RSSI is always a strength (dBm) */
            BT_LOGI("\tRaw RSSI: %d (dBm)", p_data->sig_strength.raw_rssi_value);
        }

        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_LINK_QUALITY)
        {
            BT_LOGI("\tLink Quality: %d", p_data->sig_strength.link_quality_value);
        }
        break;
    case BSA_SEC_UNKNOWN_LINKKEY_EVT:
        BT_LOGI("BSA_SEC_UNKNOWN_LINKKEY_EVT received for BdAddr: %02x:%02x:%02x:%02x:%02x:%02x",
                  p_data->lost_link_key.bd_addr[0], p_data->lost_link_key.bd_addr[1],
                  p_data->lost_link_key.bd_addr[2], p_data->lost_link_key.bd_addr[3],
                  p_data->lost_link_key.bd_addr[4], p_data->lost_link_key.bd_addr[5]);
        break;

    default:
        BT_LOGE("unknown event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_send_pincode
 **
 ** Description      Sends simple pairing passkey to server
 **
 ** Parameters       passkey
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_send_pincode(tBSA_SEC_PIN_CODE_REPLY pin_code_reply)
{
    int status;
    bdcpy(pin_code_reply.bd_addr, g_pin_code_reply.bd_addr);
    status = BSA_SecPinCodeReply(&pin_code_reply);
    if (status)
    {
        BT_LOGE("BSA_SecPinCodeReply ERROR:%d", status);
    }
}


/*******************************************************************************
 **
 ** Function         app_mgr_send_passkey
 **
 ** Description      Sends simple pairing passkey to server
 **
 ** Parameters       passkey
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_send_passkey(UINT32 passkey)
{
    g_passkey_reply.passkey = passkey;
    BSA_SecSpPasskeyReply(&g_passkey_reply);
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_set_sp_cap
 **
 ** Description      Set simple pairing capabilities
 **
 ** Parameters       sp_cap: simple pairing capabilities
 **
 ** Returns          status
 **
 *******************************************************************************/
int app_mgr_sec_set_sp_cap(tBSA_SEC_IO_CAP sp_cap)
{
    int status;
    tBSA_SEC_SET_SECURITY set_security;

    BSA_SecSetSecurityInit(&set_security);
    set_security.simple_pairing_io_cap = sp_cap;
    set_security.sec_cback = app_mgr_security_callback;
    status = BSA_SecSetSecurity(&set_security);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecSetSecurity failed:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_set_security
 **
 ** Description      Security configuration
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_sec_set_security(void)
{
    int status;
    tBSA_SEC_SET_SECURITY set_security;

    BT_LOGD("");

    BSA_SecSetSecurityInit(&set_security);
    set_security.simple_pairing_io_cap = app_xml_config.io_cap;
    set_security.sec_cback = app_mgr_security_callback;
    status = BSA_SecSetSecurity(&set_security);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecSetSecurity failed:%d", status);
        return -1;
    }
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_sp_cfm_reply
 **
 ** Description      Function used to accept/refuse Simple Pairing
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_mgr_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_SP_CFM_REPLY app_sec_sp_cfm_reply;

    BT_LOGD("");

    BSA_SecSpCfmReplyInit(&app_sec_sp_cfm_reply);
    app_sec_sp_cfm_reply.accept = accept;
    bdcpy(app_sec_sp_cfm_reply.bd_addr, bd_addr);
    app_sec_sp_cfm_reply.is_ble = app_sec_is_ble;
    status = BSA_SecSpCfmReply(&app_sec_sp_cfm_reply);
    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecSpCfmReply failed:%d", status);
        return -1;
    }
    return 0;
}

tAPP_XML_REM_DEVICE *app_find_rem_device_by_bdaddr(BD_ADDR bd_addr)
{
    int device_index;
    for (device_index = 0; device_index < APP_MAX_NB_REMOTE_STORED_DEVICES; device_index++) {
        if ((app_xml_remote_devices_db[device_index].in_use) && bdcmp(app_xml_remote_devices_db[device_index].bd_addr, bd_addr) == 0) {
            return &app_xml_remote_devices_db[device_index];
        }
    }
    return NULL;
}

tBSA_DISC_DEV *app_disc_find_device(BD_ADDR bda)
{
    int index;

    if (_global_bt_ctx) {
        for (index = 0; index < BT_DISC_NB_DEVICES; index++)
        {
            if ((_global_bt_ctx->discovery_devs[index].in_use != FALSE) &&
                (bdcmp(_global_bt_ctx->discovery_devs[index].device.bd_addr, bda) == 0))
            {
                return &_global_bt_ctx->discovery_devs[index];
            }
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function         app_mgr_sec_unpair
 **
 ** Description      Unpair a device
 **
 ** Parameters
 **
 ** Returns          0 if success / -1 if error
 **
 *******************************************************************************/
int app_mgr_sec_unpair(BD_ADDR bd_addr)
{
    int status;
    tBSA_SEC_REMOVE_DEV sec_remove;
    tAPP_XML_REM_DEVICE *rem_dev;

    BT_LOGI("app_mgr_sec_unpair");

    /* Remove the device from Security database (BSA Server) */
    BSA_SecRemoveDeviceInit(&sec_remove);

    /* Read the XML file which contains all the bonded devices */
    app_read_xml_remote_devices();

    rem_dev = app_find_rem_device_by_bdaddr(bd_addr);
    if (rem_dev == NULL) {
        BT_LOGE("not find in database");
        return 0;
    }

    bdcpy(sec_remove.bd_addr, bd_addr);
    status = BSA_SecRemoveDevice(&sec_remove);

    if (status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_SecRemoveDevice Operation Failed with status [%d]",status);

        return -1;
    }
    else
    {
        /* delete the device from database */
        rem_dev->in_use = FALSE;
        app_write_xml_remote_devices();
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_discoverable
 **
 ** Description      Set the device discoverable for a specific time
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_discoverable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.discoverable = 1;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_non_discoverable
 **
 ** Description      Set the device non discoverable
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_non_discoverable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.discoverable = 0;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_connectable
 **
 ** Description      Set the device connectable for a specific time
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_connectable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.connectable = 1;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_set_non_connectable
 **
 ** Description      Set the device non connectable
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_non_connectable(void)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }
    bsa_dm_set_config.connectable = 0;
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

/*******************************************************************************
**
** Function         app_mgr_set_local_di
**
** Description      Set local device Id
**
** Parameters
**
** Returns          void
**
*******************************************************************************/
int app_mgr_set_local_di(void)
{
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_DI di;

    di.vendor_id = 0x000F;
    di.product_id = 0x4356;
    di.primary = TRUE;
    di.vendor_id_source = 0x0001; // 0x0001 - Bluetooth SIG, 0x0002 -- USB Forum
    di.version = 0x0B00; // xJJMN for version JJ.M.N (JJ:major,M:minor,N:sub-minor)

    /* Start Discovery Device Info */
    bsa_status = BSA_DmSetLocalDiRecord(&di);

    return bsa_status;
}

/*******************************************************************************
 **
 ** Function         app_mgr_get_local_di
 **
 ** Description      This function is used to read local primary DI record
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_get_local_di(void)
{
    tBSA_DM_GET_DI bsa_read_di;
    tBSA_STATUS bsa_status;
    char *tmpstr;

    bsa_status = BSA_DmGetLocalDiRecord(&bsa_read_di);
    if (bsa_status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmGetLocalDiRecord failed status:%d", bsa_status);
        return(-1);
    }

    BT_LOGI("Local Primary Device ID --");
    BT_LOGI("\tVendor ID: 0x%04x", bsa_read_di.vendor_id);
    BT_LOGI("\tProduct ID: 0x%04x", bsa_read_di.product_id);
    BT_LOGI("\tMajor version: %d", (bsa_read_di.version & 0xFF00) >> 8);
    BT_LOGI("\tMinor version: %d", (bsa_read_di.version & 0x00F0) >> 4);
    BT_LOGI("\tSub-minor version: %d", bsa_read_di.version & 0x000F);
    BT_LOGI("\t%s a primary record", bsa_read_di.primary ? "is" : "not");
    if (bsa_read_di.vendor_id_source == 0x0001)
        tmpstr = "Bluetooth SIG";
    else if (bsa_read_di.vendor_id_source == 0x0002)
        tmpstr = "USB Forum";
    else
        tmpstr = "unknown";
    BT_LOGI("\tVendor ID source is %s", tmpstr);

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_get_cod
 **
 ** Description      Get a class of device from the user
 **
 ** Parameters       COD return value
 **
 ** Returns          status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_get_cod(DEV_CLASS cod)
{
    DEV_CLASS cods[7] = {
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x04, 0x24 },   /* STB */
        { 0x00, 0x01, 0x00 },   /* HID host (computer) */
        { 0x00, 0x05, 0x40 },   /* HID KB */
        { 0x00, 0x05, 0x80 },   /* HID mpuse */
        { 0x00, 0x02, 0x00 },   /* phone */
        { 0x00, 0x04, 0x18 }    /* headset */
    };

    /* set top box (default), hid host, hid device, phone, headset, etc. */
    printf("Ener COD for local device:\n");
    printf("1 - Set top box (default) \n");
    printf("2 - HID host\n");
    printf("3 - HID keyboard\n");
    printf("4 - HID mouse\n");
    printf("5 - Phone\n");
    printf("6 - Headset\n");
    printf("7 - Custom\n");

    int i = (app_get_choice("Select=>"));
    if (i == 7)
    {
        char cod_str[20];
        unsigned int cod_0, cod_1, cod_2;
        memset(cod_str, 0, sizeof(cod_str));
        printf("Enter 3 byte COD, 1 byte for each in hex format:\n");
        if (2 != app_get_string("Service class",cod_str, 3))
            return -1;
        if (2 != app_get_string("Major device class",&(cod_str[2]), 3))
            return -1;
        if (2 != app_get_string("Minor device class",&(cod_str[4]), 3))
            return -1;

        if (3 != sscanf(cod_str,"%02x%02x%02x", &cod_0, &cod_1, &cod_2))
            return -1;
        cod[0] = (UINT8)cod_0;
        cod[1] = (UINT8)cod_1;
        cod[2] = (UINT8)cod_2;
        return 0;
    }
    if (i <= 0 || i > 6)
        return -1;
    cod[0] = cods[i][0];
    cod[1] = cods[i][1];
    cod[2] = cods[i][2];
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_mgr_config
 **
 ** Description      Configure the BSA server
 **
 ** Parameters       None
 **
 ** Returns          Status of the operation
 **
 *******************************************************************************/
int app_mgr_config(const char *name)
{
    int                 status;
    int                 index;
    BD_ADDR             local_bd_addr = APP_DEFAULT_BD_ADDR;
    DEV_CLASS           local_class_of_device = APP_DEFAULT_CLASS_OF_DEVICE;
    tBSA_SEC_ADD_DEV    bsa_add_dev_param;
    tBSA_SEC_ADD_SI_DEV bsa_add_si_dev_param;

    /*
     * The rest of the initialization function must be done
     * for application boot and when Bluetooth is restarted
     */

    /* Example of function to read the XML file which contains the
     * Local Bluetooth configuration
     * */
    status = app_mgr_read_config();
    if (status < 0)
    {
        BT_LOGI("Creating default XML config file");
        app_xml_config.enable = TRUE;
        app_xml_config.discoverable = TRUE;
        app_xml_config.connectable = TRUE;
        strncpy((char *)app_xml_config.name, DEFAULT_BT_NAME, sizeof(app_xml_config.name));
        app_xml_config.name[sizeof(app_xml_config.name) - 1] = '\0';
        bdcpy(app_xml_config.bd_addr, local_bd_addr);
        /* let's use a random number for the last two bytes of the BdAddr */
 //       gettimeofday(&tv, NULL);
 //       rand_seed = tv.tv_sec * tv.tv_usec * getpid();
 //       app_xml_config.bd_addr[4] = rand_r(&rand_seed);
 //       app_xml_config.bd_addr[5] = rand_r(&rand_seed);
        memcpy(app_xml_config.class_of_device, local_class_of_device, sizeof(DEV_CLASS));
        strncpy(app_xml_config.root_path, APP_DEFAULT_ROOT_PATH, sizeof(app_xml_config.root_path));
        app_xml_config.root_path[sizeof(app_xml_config.root_path) - 1] = '\0';

        strncpy((char *)app_xml_config.pin_code, APP_DEFAULT_PIN_CODE, APP_DEFAULT_PIN_LEN);
        /* The following code will not work if APP_DEFAULT_PIN_LEN is 16 bytes */
        app_xml_config.pin_code[APP_DEFAULT_PIN_LEN] = '\0';
        app_xml_config.pin_len = APP_DEFAULT_PIN_LEN;
        app_xml_config.io_cap = APP_SEC_IO_CAPABILITIES;

        status = app_mgr_write_config();
        if (status < 0)
        {
            BT_LOGE("Unable to Create default XML config file");
            app_mgt_close();
            return status;
        }
    }
    if (name) {
        snprintf((char *)app_xml_config.name, sizeof(app_xml_config.name), "%s", name);
    }

    /* Example of function to read the database of remote devices */
    status = app_mgr_read_remote_devices();
    if (status < 0)
    {
        BT_LOGE("No remote device database found");
    }
    else
    {
        app_xml_display_devices(app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));
    }

     /* Example of function to get the Local Bluetooth configuration */
    app_mgr_get_bt_config();

    /* Example of function to set the Bluetooth Security */
    status = app_mgr_sec_set_security();
    if (status < 0)
    {
        BT_LOGE("app_mgr_sec_set_security failed:%d", status);
        app_mgt_close();
        return status;
    }

    /* Add every devices found in remote device database */
    /* They will be able to connect to our device */
    BT_LOGI("Add all devices found in database");
    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_remote_devices_db) ; index++)
    {
        if (app_xml_remote_devices_db[index].in_use != FALSE)
        {
            BT_LOGI("Adding:%s", app_xml_remote_devices_db[index].name);
            BSA_SecAddDeviceInit(&bsa_add_dev_param);
            bdcpy(bsa_add_dev_param.bd_addr,
                    app_xml_remote_devices_db[index].bd_addr);
            memcpy(bsa_add_dev_param.class_of_device,
                    app_xml_remote_devices_db[index].class_of_device,
                    sizeof(DEV_CLASS));
            memcpy(bsa_add_dev_param.link_key,
                    app_xml_remote_devices_db[index].link_key,
                    sizeof(LINK_KEY));
            bsa_add_dev_param.link_key_present = app_xml_remote_devices_db[index].link_key_present;
            bsa_add_dev_param.trusted_services = app_xml_remote_devices_db[index].trusted_services;
            bsa_add_dev_param.is_trusted = TRUE;
            bsa_add_dev_param.key_type = app_xml_remote_devices_db[index].key_type;
            bsa_add_dev_param.io_cap = app_xml_remote_devices_db[index].io_cap;
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
            bsa_add_dev_param.ble_addr_type = app_xml_remote_devices_db[index].ble_addr_type;
            bsa_add_dev_param.device_type = app_xml_remote_devices_db[index].device_type;
            bsa_add_dev_param.inq_result_type = app_xml_remote_devices_db[index].inq_result_type;
            if(app_xml_remote_devices_db[index].ble_link_key_present)
            {
                bsa_add_dev_param.ble_link_key_present = TRUE;

                /* Fill PENC Key */
                memcpy(bsa_add_dev_param.le_penc_key.ltk,
                    app_xml_remote_devices_db[index].penc_ltk,
                    sizeof(app_xml_remote_devices_db[index].penc_ltk));
                memcpy(bsa_add_dev_param.le_penc_key.rand,
                    app_xml_remote_devices_db[index].penc_rand,
                    sizeof(app_xml_remote_devices_db[index].penc_rand));
                bsa_add_dev_param.le_penc_key.ediv = app_xml_remote_devices_db[index].penc_ediv;
                bsa_add_dev_param.le_penc_key.sec_level = app_xml_remote_devices_db[index].penc_sec_level;
                bsa_add_dev_param.le_penc_key.key_size = app_xml_remote_devices_db[index].penc_key_size;

                /* Fill PID Key */
                memcpy(bsa_add_dev_param.le_pid_key.irk,
                    app_xml_remote_devices_db[index].pid_irk,
                    sizeof(app_xml_remote_devices_db[index].pid_irk));
                bsa_add_dev_param.le_pid_key.addr_type = app_xml_remote_devices_db[index].pid_addr_type;
                memcpy(bsa_add_dev_param.le_pid_key.static_addr,
                    app_xml_remote_devices_db[index].pid_static_addr,
                    sizeof(app_xml_remote_devices_db[index].pid_static_addr));

                /* Fill PCSRK Key */
                bsa_add_dev_param.le_pcsrk_key.counter = app_xml_remote_devices_db[index].pcsrk_counter;
                memcpy(bsa_add_dev_param.le_pcsrk_key.csrk,
                    app_xml_remote_devices_db[index].pcsrk_csrk,
                    sizeof(app_xml_remote_devices_db[index].pcsrk_csrk));
                bsa_add_dev_param.le_pcsrk_key.sec_level = app_xml_remote_devices_db[index].pcsrk_sec_level;

                /* Fill LCSRK Key */
                bsa_add_dev_param.le_lcsrk_key.counter = app_xml_remote_devices_db[index].lcsrk_counter;
                bsa_add_dev_param.le_lcsrk_key.div = app_xml_remote_devices_db[index].lcsrk_div;
                bsa_add_dev_param.le_lcsrk_key.sec_level = app_xml_remote_devices_db[index].lcsrk_sec_level;

                /* Fill LENC Key */
                bsa_add_dev_param.le_lenc_key.div = app_xml_remote_devices_db[index].lenc_div;
                bsa_add_dev_param.le_lenc_key.key_size = app_xml_remote_devices_db[index].lenc_key_size;
                bsa_add_dev_param.le_lenc_key.sec_level = app_xml_remote_devices_db[index].lenc_sec_level;
            }
#endif
            BSA_SecAddDevice(&bsa_add_dev_param);
        }
    }

    /* Add stored SI devices to BSA server */
    app_read_xml_si_devices();
    for (index = 0 ; index < APP_NUM_ELEMENTS(app_xml_si_devices_db) ; index++)
    {
        if (app_xml_si_devices_db[index].in_use)
        {
            BSA_SecAddSiDevInit(&bsa_add_si_dev_param);
            bdcpy(bsa_add_si_dev_param.bd_addr, app_xml_si_devices_db[index].bd_addr);
            bsa_add_si_dev_param.platform = app_xml_si_devices_db[index].platform;
            BSA_SecAddSiDev(&bsa_add_si_dev_param);
        }
    }

    /* Example of function to set the Local Bluetooth configuration */
    app_mgr_set_bt_config(app_xml_config.enable);

    return 0;
}

int app_get_module_addr(BTAddr addr)
{
    bdcpy(addr, app_xml_config.bd_addr);
    return 0;
}
/*******************************************************************************
 **
 ** Function         app_mgt_set_cod
 **
 ** Description      Configure local class of device
 **
 ** Parameters       None
 **
 ** Returns          Status of the operation
 **
 *******************************************************************************/
int app_mgt_set_cod()
{
    DEV_CLASS cod = APP_DEFAULT_CLASS_OF_DEVICE;

    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return -1;
    }

    memcpy(bsa_dm_set_config.class_of_device, cod, sizeof(DEV_CLASS));
    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_DEV_CLASS_MASK;
    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_mgr_get_dual_stack_mode_desc
 **
 ** Description     Get Dual Stack Mode description
 **
 ** Parameters      None
 **
 ** Returns         Description string
 **
 *******************************************************************************/
char *app_mgr_get_dual_stack_mode_desc(void)
{
    switch(app_mgr_cb.dual_stack_mode)
    {
    case BSA_DM_DUAL_STACK_MODE_BSA:
        return "DUAL_STACK_MODE_BSA";
    case BSA_DM_DUAL_STACK_MODE_MM:
        return "DUAL_STACK_MODE_MM/Kernel";
    case BSA_DM_DUAL_STACK_MODE_BTC:
        return "DUAL_STACK_MODE_BTC";
    default:
        return "Unknown Dual Stack Mode";
    }
}

/*******************************************************************************
 **
 ** Function         app_mgr_read_version
 **
 ** Description      This function is used to read BSA and FW version
 **
 ** Parameters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_mgr_read_version(void)
{
    tBSA_TM_READ_VERSION bsa_read_version;
    tBSA_STATUS bsa_status;

    bsa_status = BSA_TmReadVersionInit(&bsa_read_version);
    bsa_status = BSA_TmReadVersion(&bsa_read_version);
    if (bsa_status != BSA_SUCCESS)
    {
        BT_LOGE("BSA_TmReadVersion failed status:%d", bsa_status);
        return(-1);
    }

    BT_LOGI("Server status:%d", bsa_read_version.status);
    BT_LOGI("FW Version:%d.%d.%d.%d",
            bsa_read_version.fw_version.major,
            bsa_read_version.fw_version.minor,
            bsa_read_version.fw_version.build,
            bsa_read_version.fw_version.config);
    BT_LOGI("BSA Server Version:%s", bsa_read_version.bsa_server_version);
    return 0;
}


/*******************************************************************************
 **
 ** Function         app_mgr_set_link_policy
 **
 ** Description      Set the device link policy
 **                  This function sets/clears the link policy mask to the given
 **                  bd_addr.
 **                  If clearing the sniff or park mode mask, the link is put
 **                  in active mode.
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_mgr_set_link_policy(BD_ADDR bd_addr, tBSA_DM_LP_MASK policy_mask, BOOLEAN set)
{
    tBSA_DM_SET_CONFIG bsa_dm_set_config;
    if (BSA_DmSetConfigInit(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfigInit failed");
        return;
    }

    bsa_dm_set_config.config_mask = BSA_DM_CONFIG_LINK_POLICY_MASK;

    bdcpy(bsa_dm_set_config.policy_param.link_bd_addr, bd_addr);
    bsa_dm_set_config.policy_param.policy_mask = policy_mask;
    bsa_dm_set_config.policy_param.set = set;

    if (BSA_DmSetConfig(&bsa_dm_set_config) != BSA_SUCCESS)
    {
        BT_LOGE("BSA_DmSetConfig failed");
        return;
    }
}

int app_mgr_set_bt_name(const char *name)
{
    if (!name || strlen(name) >= sizeof(app_xml_config.name))
    {
        BT_LOGE("Invalid bt name");
        return -1;
    }
    snprintf((char *)app_xml_config.name, sizeof(app_xml_config.name), "%s", name);

    return app_mgr_set_bt_config(TRUE);
}


/*****************************************************************************
 **
 **  Name:           app_sec.c
 **
 **  Description:    Bluetooth Security functions
 **
 **  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include "app_sec.h"

#include <bsa_rokid/bsa_api.h>

#include "app_xml_param.h"
#include "app_xml_utils.h"
#include "app_utils.h"
#include "app_disc.h"
#include "app_link.h"
#include "app_services.h"
#include "app_manager.h"

tAPP_SECURITY_CB app_sec_cb;

void app_sec_generic_cback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data);

/*******************************************************************************
 **
 ** Function         app_sec_bond
 **
 ** Description      Bond a device
 **
 ** Parameters
 **     BD_ADDR bdaddr: bluetooth device address of remote device
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_bond(BD_ADDR bdaddr)
{
    tBSA_STATUS status;
    tBSA_SEC_BOND sec_bond;

    BSA_SecBondInit(&sec_bond);
    bdcpy(sec_bond.bd_addr, bdaddr);

    status = BSA_SecBond(&sec_bond);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Bond status:%d", status);
        return -1;
    }
    else
    {
        APP_INFO0("bonding ongoing...");
        return 0;
    }
}

/*******************************************************************************
 **
 ** Function         app_sec_bond_cancel
 **
 ** Description      Cancel a bonding procedure
 **
 ** Parameters
 **     BD_ADDR bdaddr: bluetooth device address of remote device
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_bond_cancel(BD_ADDR bdaddr)
{
    tBSA_STATUS status;
    tBSA_SEC_BOND_CANCEL sec_bond_cancel;

    BSA_SecBondCancelInit(&sec_bond_cancel);
    bdcpy(sec_bond_cancel.bd_addr, bdaddr);
    status = BSA_SecBondCancel(&sec_bond_cancel);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecBondCancel failed:%d", status);
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_sec_sp_cfm_reply
 **
 ** Description      Function used to accept/refuse Simple Pairing
 **
 ** Parameters
 **     BOOLEAN accept: accept/reject boolean
 **     BD_ADDR bd_addr: bluetooth device address of remote device
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr)
{
    tBSA_STATUS status;
    tBSA_SEC_SP_CFM_REPLY   app_sec_sp_cfm_reply;

    APP_INFO0("Reply to Simple Pairing");

    BSA_SecSpCfmReplyInit(&app_sec_sp_cfm_reply);
    app_sec_sp_cfm_reply.accept = accept;
    bdcpy(app_sec_sp_cfm_reply.bd_addr, bd_addr);
    app_sec_sp_cfm_reply.is_ble = app_sec_cb.is_ble;
    status = BSA_SecSpCfmReply(&app_sec_sp_cfm_reply);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Reply to SP Req status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_sec_set_security
 **
 ** Description      Security callback
 **
 ** Parameters
 **     tAPP_SEC_CUSTOM_CBACK* app_sec_cback: application custom callback.
 **                                 If NULL callback will not be invoked
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_set_security(tAPP_SEC_CUSTOM_CBACK *app_sec_cback)
{
    tBSA_STATUS             status;
    int                     ret_val;
    tBSA_SEC_SET_SECURITY   set_security;
    tAPP_XML_CONFIG xml_local_config;

    APP_INFO0("Set Bluetooth Security");

    /* Read IO cap configuration */
    ret_val = app_read_xml_config(&xml_local_config);
    if (ret_val < 0)
    {
        APP_INFO1("app_read_xml_config failed: %d", ret_val);
        return -1;
    }

    /* Update security control block */
    strncpy((char*) app_sec_cb.pin_code,(char*) xml_local_config.pin_code,sizeof(app_sec_cb.pin_code));
    app_sec_cb.pin_code[sizeof(app_sec_cb.pin_code)-1] = 0;
    app_sec_cb.pin_len = xml_local_config.pin_len;

    BSA_SecSetSecurityInit(&set_security);
    set_security.simple_pairing_io_cap = xml_local_config.io_cap;

    set_security.sec_cback = app_sec_generic_cback;

    if (app_sec_cback != NULL)
    {
        app_sec_cb.sec_custom_cback = app_sec_cback;
    }
    set_security.ssp_debug = app_sec_cb.ssp_debug;

    status = BSA_SecSetSecurity(&set_security);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_SecSetSecurity failed: Unable to Set Security status:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_sec_set_ssp_debug_mode
 **
 ** Description     Set ssp debug mode
 **
 ** Parameters
 **         debug_mode: flag that enables/disable ssp debug mode
 **
 ** Returns
 **         0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_set_ssp_debug_mode(BOOLEAN debug_mode)
{
    if(debug_mode == TRUE)
    {
        APP_INFO0("Enable SPP debug mode");
    }
    else
    {
        APP_INFO0("Disable SPP debug mode");
    }

    app_sec_cb.ssp_debug = debug_mode;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_sec_set_default_pin
 **
 ** Description     Set default pin code and length in xml config file
 **
 ** Parameters
 **         pin_code: pin code to set
 **         pin_len: pin length to set
 **
 ** Returns
 **         0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_set_default_pin(PIN_CODE pin_code,UINT8 pin_len)
{
    tAPP_XML_CONFIG xml_local_config;
    int status;

    APP_INFO0("Set Bluetooth Security");

    /* Read IO cap configuration */
    status = app_read_xml_config(&xml_local_config);
    if (status < 0)
    {
        APP_INFO1("app_read_xml_config failed: %d", status);
        return -1;
    }

    /* Update xml config */
    strncpy((char*) xml_local_config.pin_code, (char*) pin_code,sizeof(xml_local_config.pin_code));
    xml_local_config.pin_code[sizeof(xml_local_config.pin_code)-1] = 0;
    xml_local_config.pin_len = pin_len;

    if (app_write_xml_config(&xml_local_config) < 0)
    {
        APP_INFO0("app_write_xml_config failed: Unable to Write XML config file");
        return -1;
    }
    else
    {
        /* Update security control block */
        strncpy((char*) app_sec_cb.pin_code, (char*) xml_local_config.pin_code,
                sizeof(app_sec_cb.pin_code));
        app_sec_cb.pin_code[sizeof(app_sec_cb.pin_code)-1] = 0;
        app_sec_cb.pin_len = xml_local_config.pin_len;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_sec_get_pin
 **
 ** Description     Wait for a pin code from user
 **
 ** Parameters
 **     UINT8* pin_code: pointer on pin code that will be set
 **     UINT8* pin_code_len: pointer on pin length that will be set
 **
 ** Returns
 **             0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_sec_get_pin(UINT8* pin_code,UINT8* pin_code_len)
{
    char string_pincode[50];
    int pin_attempt = 0;
    int pin_len;

    while(pin_attempt < 3)
    {
        pin_len = app_get_string("Enter PIN code(16 digit max)", string_pincode, sizeof(string_pincode));
        if (pin_len < 0)
        {
            APP_ERROR0("app_get_string failed");
            return -1;
        }
        if (pin_len > 16)
        {
            APP_ERROR0("pin length must be 16 digits long maximum");
            pin_attempt++;
        }
        else
        {
            APP_INFO1("PIN Code = %s", string_pincode);

            strncpy((char *)pin_code, string_pincode, pin_len);
            pin_code[PIN_CODE_LEN-1] = 0;
            *pin_code_len = pin_len;
            break;
        }
    }

    if (pin_attempt >= 3)
    {
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_sec_set_pairing_mode
 **
 ** Description      Set the pairing mode (auto/manual reply)
 **
 ** Parameters
 **         tAPP_SEC_PAIRING_MODE mode: pairing mode (auto or manual reply)
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_set_pairing_mode(tAPP_SEC_PAIRING_MODE mode)
{
    int status = 0;

    switch(mode)
    {
    case APP_SEC_AUTO_PAIR_MODE:
        app_sec_cb.auto_accept_pairing = TRUE;
        break;

    case APP_SEC_MANUAL_PAIR_MODE:
        app_sec_cb.auto_accept_pairing = FALSE;
        break;

    default:
        APP_ERROR1("Bad pairing mode value %d",mode);
        status = -1;
        break;
    }

    return status;
}

/*******************************************************************************
 **
 ** Function         app_sec_pin_reply
 **
 ** Description      Pin reply function
 **
 ** Parameters
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_pin_reply(void)
{
    tBSA_SEC_PIN_CODE_REPLY pin_code_reply;
    tBSA_STATUS bsa_status;

    if (app_sec_get_pin(app_sec_cb.pin_code, &app_sec_cb.pin_len) < 0)
    {
        APP_ERROR0("app_sec_get_pin failed");
        return -1;
    }
    BSA_SecPinCodeReplyInit(&pin_code_reply);
    bdcpy(pin_code_reply.bd_addr, app_sec_cb.sec_bd_addr);
    pin_code_reply.pin_len = app_sec_cb.pin_len;
    strncpy((char *)pin_code_reply.pin_code, (char*) app_sec_cb.pin_code,
            sizeof(pin_code_reply.pin_code));
    /* note that this code will not work if pin_len = 16 */
    pin_code_reply.pin_code[PIN_CODE_LEN-1] = '\0';
    bsa_status = BSA_SecPinCodeReply(&pin_code_reply);
    if (bsa_status != BSA_SUCCESS)
    {
        return -1;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_sec_generic_cback
 **
 ** Description      Generic Security callback
 **
 ** Parameters
 **     tBSA_SEC_EVT event: security event
 **     tBSA_SEC_MSG *p_data: data
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_sec_generic_cback(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data)
{
    int status;
    tBSA_DISC_DEV *disc_dev;
    tBSA_SEC_PIN_CODE_REPLY pin_code_reply;
    tBSA_SEC_AUTH_REPLY autorize_reply;
    tAPP_XML_CONFIG xml_local_config;
    BOOLEAN exit_generic_cback= FALSE;

    if (app_sec_cb.sec_custom_cback != NULL)
    {
        exit_generic_cback = app_sec_cb.sec_custom_cback(event, p_data);
    }

    if (exit_generic_cback != FALSE)
    {
        return;
    }

    switch(event)
    {
    case BSA_SEC_LINK_UP_EVT:       /* A device is physically connected (for info) */
        APP_INFO1("BSA_SEC_LINK_UP_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->link_up.bd_addr[0], p_data->link_up.bd_addr[1],
                p_data->link_up.bd_addr[2], p_data->link_up.bd_addr[3],
                p_data->link_up.bd_addr[4], p_data->link_up.bd_addr[5]);
        APP_INFO1(" ClassOfDevice:%02x:%02x:%02x => %s",
                p_data->link_up.class_of_device[0],
                p_data->link_up.class_of_device[1],
                p_data->link_up.class_of_device[2],
                app_get_cod_string(p_data->link_up.class_of_device));
        app_link_add(p_data->link_up.bd_addr, &p_data->link_up.class_of_device[0]);
        break;

    case BSA_SEC_LINK_DOWN_EVT:     /* A device is physically disconnected (for info) */
        APP_INFO1("BSA_SEC_LINK_DOWN_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->link_down.bd_addr[0], p_data->link_down.bd_addr[1],
                p_data->link_down.bd_addr[2], p_data->link_down.bd_addr[3],
                p_data->link_down.bd_addr[4], p_data->link_down.bd_addr[5]);
        APP_INFO1(" Reason: %d (0x%x)", p_data->link_down.status, p_data->link_down.status);
        app_link_remove(p_data->link_up.bd_addr);
        break;

    case BSA_SEC_PIN_REQ_EVT:
        APP_INFO0("BSA_SEC_PIN_REQ_EVT (Pin Code Request) received from:");
        APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->pin_req.bd_addr[0], p_data->pin_req.bd_addr[1],
                p_data->pin_req.bd_addr[2], p_data->pin_req.bd_addr[3],
                p_data->pin_req.bd_addr[4],p_data->pin_req.bd_addr[5]);

        status = app_read_xml_config(&xml_local_config);
        if (status < 0)
        {
            APP_ERROR0("app_read_xml_config failed: Unable to Read XML config file");
            return;
        }

        if (app_sec_cb.auto_accept_pairing == FALSE)
        {
            bdcpy(app_sec_cb.sec_bd_addr, p_data->pin_req.bd_addr);
            APP_DEBUG0("    You must set pin code using Pin Code Reply menu ");
        }
        else
        {
            APP_INFO1 ("App calls BSA_SecPinCodeReply pin:%s len:%d", xml_local_config.pin_code,
                    xml_local_config.pin_len);
            BSA_SecPinCodeReplyInit(&pin_code_reply);
            bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
            pin_code_reply.pin_len = xml_local_config.pin_len;
            memcpy(pin_code_reply.pin_code, xml_local_config.pin_code, sizeof(PIN_CODE));
            status = BSA_SecPinCodeReply(&pin_code_reply);
            if (status != BSA_SUCCESS)
            {
                APP_ERROR1("BSA_SecPinCodeReply Failed %d",status);
            }
        }
        break;

    case BSA_SEC_AUTH_CMPL_EVT:
        APP_INFO0("BSA_SEC_AUTH_CMPL_EVT received");
        APP_INFO1("\tName:%s", p_data->auth_cmpl.bd_name);
        if (p_data->auth_cmpl.success == 0)
        {
            APP_INFO1("\tSuccess:%d => FAIL failure_reason:%d", p_data->auth_cmpl.success,
                    p_data->auth_cmpl.fail_reason);
        }
        else
        {
            APP_INFO1("\tSuccess:%d => OK", p_data->auth_cmpl.success);
        }
        APP_INFO1("\tbd_addr:%02x:%02x:%02x:%02x:%02x:%02x",
                p_data->auth_cmpl.bd_addr[0], p_data->auth_cmpl.bd_addr[1],
                p_data->auth_cmpl.bd_addr[2], p_data->auth_cmpl.bd_addr[3],
                p_data->auth_cmpl.bd_addr[4], p_data->auth_cmpl.bd_addr[5]);
        if (p_data->auth_cmpl.key_present != FALSE)
            APP_INFO1("\tLinkKey:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                    p_data->auth_cmpl.key[0], p_data->auth_cmpl.key[1],
                    p_data->auth_cmpl.key[2], p_data->auth_cmpl.key[3],
                    p_data->auth_cmpl.key[4], p_data->auth_cmpl.key[5],
                    p_data->auth_cmpl.key[6], p_data->auth_cmpl.key[7],
                    p_data->auth_cmpl.key[8], p_data->auth_cmpl.key[9],
                    p_data->auth_cmpl.key[10], p_data->auth_cmpl.key[11],
                    p_data->auth_cmpl.key[12], p_data->auth_cmpl.key[13],
                    p_data->auth_cmpl.key[14], p_data->auth_cmpl.key[15]);

        /* If success */
        if (p_data->auth_cmpl.success != 0)
        {
            /* Read the Remote device xml file to have a fresh view */
            app_read_xml_remote_devices();

            app_xml_update_name_db(app_xml_remote_devices_db,
                                   APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                   p_data->auth_cmpl.bd_addr,
                                   p_data->auth_cmpl.bd_name);

            if (p_data->auth_cmpl.key_present != FALSE)
            {
                app_xml_update_key_db(app_xml_remote_devices_db,
                                      APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                      p_data->auth_cmpl.bd_addr,
                                      p_data->auth_cmpl.key,
                                      p_data->auth_cmpl.key_type);
            }

            /* Unfortunately, the BSA_SEC_AUTH_CMPL_EVT does not contain COD */
            /* so update the XML with info found during inquiry */
            disc_dev = app_disc_find_device(p_data->auth_cmpl.bd_addr);
            if (disc_dev != NULL)
            {
                app_xml_update_cod_db(app_xml_remote_devices_db,
                                      APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                                      p_data->auth_cmpl.bd_addr,
                                      disc_dev->device.class_of_device);
            }

            status = app_write_xml_remote_devices();
            if (status < 0)
                APP_ERROR0("app_security_callback fail to store remote devices database!!!");
            else
                APP_INFO0("app_security_callback updated remote devices database");
        }
        break;

    case BSA_SEC_BOND_CANCEL_CMPL_EVT:
        APP_INFO1("BSA_SEC_BOND_CANCEL_CMPL_EVT status=%d",
                   p_data->bond_cancel.status);
        break;

    case BSA_SEC_AUTHORIZE_EVT:  /* Authorization request */
        APP_INFO0("BSA_SEC_AUTHORIZE_EVT received");
        APP_INFO1("\tRemote device:%s ", p_data->authorize.bd_name);
        APP_INFO1("bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->authorize.bd_addr[0], p_data->authorize.bd_addr[1],
                p_data->authorize.bd_addr[2], p_data->authorize.bd_addr[3],
                p_data->authorize.bd_addr[4],p_data->authorize.bd_addr[5]);
        APP_INFO1("\tRequest access to service:%x (%s)",
                (int)p_data->authorize.service,
                app_service_id_to_string(p_data->authorize.service));

        status = BSA_SecAuthorizeReplyInit(&autorize_reply);
        bdcpy(autorize_reply.bd_addr, p_data->authorize.bd_addr);
        autorize_reply.trusted_service = p_data->authorize.service;
        autorize_reply.auth = app_sec_cb.auth;
        {
            switch(autorize_reply.auth)
            {
            case BSA_SEC_NOT_AUTH:
                APP_INFO1("Access Refused (%d)", autorize_reply.auth);
                break;
            case BSA_SEC_AUTH_TEMP:
                APP_INFO1("Access Temporary Accepted (%d)", autorize_reply.auth);
                break;
            case BSA_SEC_AUTH_PERM:
                APP_INFO1("Access Accepted (%d)", autorize_reply.auth);
                break;
            default:
                APP_INFO1("Unknown Access response (%d)", autorize_reply.auth);
                break;
            }
        }
        status = BSA_SecAuthorizeReply(&autorize_reply);
        /*
         * Update XML database
         */
        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();
        /* Add AV service for this devices in XML database */
        app_xml_add_trusted_services_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->authorize.bd_addr,
                1 << p_data->authorize.service);

        if (strlen((char *)p_data->authorize.bd_name) > 0)
            app_xml_update_name_db(app_xml_remote_devices_db,
                    APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                    p_data->authorize.bd_addr, p_data->authorize.bd_name);

        /* Update database => write on disk */
        status = app_write_xml_remote_devices();
        if (status < 0)
            APP_ERROR0("app_security_callback fail to store remote devices database!!!");
        else
            APP_INFO0("app_security_callback updated remote devices database");
        break;

    case BSA_SEC_SP_CFM_REQ_EVT:     /* Simple Pairing confirm request */
        APP_INFO0("BSA_SEC_SP_CFM_REQ_EVT received");
        APP_INFO1("\tRemote device:%s ", p_data->cfm_req.bd_name);
        APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->cfm_req.bd_addr[0], p_data->cfm_req.bd_addr[1],
                p_data->cfm_req.bd_addr[2], p_data->cfm_req.bd_addr[3],
                p_data->cfm_req.bd_addr[4], p_data->cfm_req.bd_addr[5]);
        APP_INFO1("\tClassOfDevice:%02x:%02x:%02x => %s", p_data->cfm_req.class_of_device[0],
                p_data->cfm_req.class_of_device[1], p_data->cfm_req.class_of_device[2],
                app_get_cod_string(p_data->cfm_req.class_of_device));
        APP_INFO1("\tJust Work:%s", p_data->cfm_req.just_works == TRUE ? "TRUE" : "FALSE");
        APP_INFO1("\tNumeric Value:%d", p_data->cfm_req.num_val);
        app_sec_cb.is_ble = p_data->cfm_req.is_ble;
        if (app_sec_cb.auto_accept_pairing == FALSE)
        {
            APP_DEBUG0("    You must accept/refuse using Simple pairing Accept/Refuse menus");
        }
        else
        {
            APP_INFO0("\tAccept it automatically");
            app_sec_sp_cfm_reply(TRUE, p_data->cfm_req.bd_addr);
        }

        bdcpy(app_sec_cb.sec_bd_addr, p_data->cfm_req.bd_addr);
        break;

    case BSA_SEC_SP_KEY_NOTIF_EVT: /* Simple Pairing Passkey Notification */
        APP_INFO0("BSA_SEC_SP_KEY_NOTIF_EVT received");
        APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->key_notif.bd_addr[0], p_data->key_notif.bd_addr[1],
                p_data->key_notif.bd_addr[2], p_data->key_notif.bd_addr[3],
                p_data->key_notif.bd_addr[4], p_data->key_notif.bd_addr[5]);
        APP_INFO1("\tClassOfDevice:%02x:%02x:%02x => %s", p_data->key_notif.class_of_device[0],
                p_data->key_notif.class_of_device[1], p_data->key_notif.class_of_device[2],
                app_get_cod_string(p_data->key_notif.class_of_device));
        APP_INFO1("\tNumeric Value:%d", p_data->key_notif.passkey);
        APP_INFO0("\t => You must enter this value on peer device's keyboard");
        break;

    case BSA_SEC_SP_KEYPRESS_EVT: /* Simple Pairing Key press notification event. */
        APP_INFO1("BSA_SEC_SP_KEYPRESS_EVT received type:%d", p_data->key_press.notif_type);
        APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->key_press.bd_addr[0], p_data->key_press.bd_addr[1],
                p_data->key_press.bd_addr[2], p_data->key_press.bd_addr[3],
                p_data->key_press.bd_addr[4], p_data->key_press.bd_addr[5]);
        break;

    case BSA_SEC_SP_RMT_OOB_EVT: /* Simple Pairing Remote OOB Data request. */
        APP_INFO0("BSA_SEC_SP_RMT_OOB_EVT received");
        APP_ERROR0("Not Yet Implemented");
        break;

    case BSA_SEC_SUSPENDED_EVT: /* Connection Suspended */
        APP_INFO1("BSA_SEC_SUSPENDED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->suspended.bd_addr[0], p_data->suspended.bd_addr[1],
                p_data->suspended.bd_addr[2], p_data->suspended.bd_addr[3],
                p_data->suspended.bd_addr[4], p_data->suspended.bd_addr[5]);
        app_link_set_suspended(p_data->suspended.bd_addr, TRUE);
        break;

    case BSA_SEC_RESUMED_EVT: /* Connection Resumed */
        APP_INFO1("BSA_SEC_RESUMED_EVT bd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->resumed.bd_addr[0], p_data->resumed.bd_addr[1],
                p_data->resumed.bd_addr[2], p_data->resumed.bd_addr[3],
                p_data->resumed.bd_addr[4], p_data->resumed.bd_addr[5]);
        app_link_set_suspended(p_data->suspended.bd_addr, FALSE);
        break;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    case BSA_SEC_BLE_KEY_EVT: /* BLE KEY event */
        APP_INFO0("BSA_SEC_BLE_KEY_EVT");
        switch (p_data->ble_key.key_type)
        {
        case BSA_LE_KEY_PENC:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PENC(%d)", p_data->ble_key.key_type);
            APP_DUMP("LTK", p_data->ble_key.key_value.penc_key.ltk, 16);
            APP_DUMP("RAND", p_data->ble_key.key_value.penc_key.rand, 8);
            APP_DEBUG1("ediv: 0x%x:", p_data->ble_key.key_value.penc_key.ediv);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.penc_key.sec_level);
            APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.penc_key.key_size);
            break;
        case BSA_LE_KEY_PID:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PID(%d)", p_data->ble_key.key_type);
            APP_DUMP("IRK", p_data->ble_key.key_value.pid_key.irk, 16);
            APP_DEBUG1("addr_type: 0x%x:", p_data->ble_key.key_value.pid_key.addr_type);
            APP_DEBUG1("static_addr: %02X-%02X-%02X-%02X-%02X-%02X",
                p_data->ble_key.key_value.pid_key.static_addr[0],
                p_data->ble_key.key_value.pid_key.static_addr[1],
                p_data->ble_key.key_value.pid_key.static_addr[2],
                p_data->ble_key.key_value.pid_key.static_addr[3],
                p_data->ble_key.key_value.pid_key.static_addr[4],
                p_data->ble_key.key_value.pid_key.static_addr[5]);
            break;
        case BSA_LE_KEY_PCSRK:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_PCSRK(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.pcsrk_key.counter);
            APP_DUMP("CSRK", p_data->ble_key.key_value.pcsrk_key.csrk, 16);
            APP_DEBUG1("sec_level: %d:", p_data->ble_key.key_value.pcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LCSRK:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LCSRK(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("counter: 0x%x:", p_data->ble_key.key_value.lcsrk_key.counter);
            APP_DEBUG1("div: %d:", p_data->ble_key.key_value.lcsrk_key.div);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lcsrk_key.sec_level);
            break;
        case BSA_LE_KEY_LENC:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LENC(%d)", p_data->ble_key.key_type);
            APP_DEBUG1("div: 0x%x:", p_data->ble_key.key_value.lenc_key.div);
            APP_DEBUG1("key_size: %d:", p_data->ble_key.key_value.lenc_key.key_size);
            APP_DEBUG1("sec_level: 0x%x:", p_data->ble_key.key_value.lenc_key.sec_level);
            break;
        case BSA_LE_KEY_LID:
            APP_DEBUG1("\t key_type: BTM_LE_KEY_LID(%d)", p_data->ble_key.key_type);
            break;
        default:
            APP_DEBUG1("\t key_type: Unknown key(%d)", p_data->ble_key.key_type);
            break;
        }
#if 0
        /* Read the Remote device xml file to have a fresh view */
        app_read_xml_remote_devices();

        app_xml_update_device_type_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db), p_data->ble_key.bd_addr,
                BT_DEVICE_TYPE_BLE);

        app_xml_update_ble_key_db(app_xml_remote_devices_db,
                APP_NUM_ELEMENTS(app_xml_remote_devices_db),
                p_data->ble_key.bd_addr,
                p_data->ble_key.key_value,
                p_data->ble_key.key_type);

        status = app_write_xml_remote_devices();
        if (status < 0)
        {
            APP_ERROR1("app_xml_write_remote_devices failed:%d", status);
        }
#endif
        break;

    case BSA_SEC_BLE_PASSKEY_REQ_EVT:
        APP_INFO0("BSA_SEC_BLE_PASSKEY_REQ_EVT (Passkey Request) received from:");
        APP_INFO1("\tbd_addr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->ble_passkey_req.bd_addr[0], p_data->ble_passkey_req.bd_addr[1],
                p_data->ble_passkey_req.bd_addr[2], p_data->ble_passkey_req.bd_addr[3],
                p_data->ble_passkey_req.bd_addr[4], p_data->ble_passkey_req.bd_addr[5]);

        status = app_read_xml_config(&xml_local_config);
        if (status < 0)
        {
            APP_ERROR0("app_read_xml_config failed: Unable to Read XML config file");
            return;
        }

        BSA_SecPinCodeReplyInit(&pin_code_reply);
        bdcpy(pin_code_reply.bd_addr, p_data->pin_req.bd_addr);
        pin_code_reply.is_ble = TRUE;
        pin_code_reply.ble_accept = TRUE;
        pin_code_reply.ble_passkey = app_get_choice("Enter Passkey on peer device");
        status = BSA_SecPinCodeReply(&pin_code_reply);
        if (status != BSA_SUCCESS)
        {
            APP_ERROR1("BSA_SecPinCodeReply Failed %d",status);
        }
        break;
#endif

    case BSA_SEC_RSSI_EVT:
        APP_INFO1("BSA_SEC_RSSI_EVT received for BdAddr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->sig_strength.bd_addr[0], p_data->sig_strength.bd_addr[1],
                p_data->sig_strength.bd_addr[2], p_data->sig_strength.bd_addr[3],
                p_data->sig_strength.bd_addr[4], p_data->sig_strength.bd_addr[5]);
        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RSSI)
        {
            if (p_data->sig_strength.link_type == BSA_TRANSPORT_BR_EDR)
            {
                /* For BasicRate & EnhancedDataRate links, the RSSI is an attenuation (dB) */
                APP_INFO1("\tRSSI: %d (dB)", p_data->sig_strength.rssi_value);
            }
            else
            {
                /* For Low Energy links, the RSSI is a strength (dBm) */
                APP_INFO1("\tRSSI: %d (dBm)", p_data->sig_strength.rssi_value);
            }
        }

        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_RAW_RSSI)
        {
            /* Raw RSSI is always a strength (dBm) */
            APP_INFO1("\tRaw RSSI: %d (dBm)", p_data->sig_strength.raw_rssi_value);
        }

        if (p_data->sig_strength.mask & BSA_SEC_SIG_STRENGTH_LINK_QUALITY)
        {
            APP_INFO1("\tLink Quality: %d", p_data->sig_strength.link_quality_value);
        }
        break;

    case BSA_SEC_UNKNOWN_LINKKEY_EVT:
        APP_INFO1("BSA_SEC_UNKNOWN_LINKKEY_EVT received for BdAddr: %02x:%02x:%02x:%02x:%02x:%02x",
                p_data->lost_link_key.bd_addr[0], p_data->lost_link_key.bd_addr[1],
                p_data->lost_link_key.bd_addr[2], p_data->lost_link_key.bd_addr[3],
                p_data->lost_link_key.bd_addr[4], p_data->lost_link_key.bd_addr[5]);
        break;

    default:
        APP_ERROR1("unknown event:%d", event);
        break;
    }
}

/*******************************************************************************
 **
 ** Function         app_sec_get_ssp_key_desc
 **
 ** Description      Get SSP Key Notification description
 **
 ** Parameters
 **     tBSA_SEC_SP_KEY_TYPE notification: key notification received
 **
 ** Returns          Key Notification description
 **
 *******************************************************************************/
char *app_sec_get_ssp_key_desc(tBSA_SEC_SP_KEY_TYPE notification)
{
    switch(notification)
    {
    case BSA_SP_KEY_STARTED:
        return "BSA_SP_KEY_STARTED";
    case BSA_SP_KEY_ENTERED:
        return "BSA_SP_KEY_ENTERED";
    case BSA_SP_KEY_ERASED:
        return "BSA_SP_KEY_ERASED";
    case BSA_SP_KEY_CLEARED:
        return "BSA_SP_KEY_CLEARED";
    case BSA_SP_KEY_COMPLETE:
        return "BSA_SP_KEY_COMPLETE";
    default:
        return "Unknown Key notification";
    }
}

/*******************************************************************************
 **
 ** Function         app_sec_set_auth
 **
 ** Description      Set the authorization to return on BT stack request
 **
 ** Parameters
 **     auth: authorization to return upon authorization request
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_set_auth(tBSA_SEC_AUTH_RESP auth)
{
    if (auth > BTA_DM_NOT_AUTH)
    {
        return -1;
    }
    app_sec_cb.auth = auth;

    return 0;
}

/*******************************************************************************
 **
 ** Function         app_sec_get_auth
 **
 ** Description      Get the authorization to return on BT stack request
 **
 ** Parameters
 **
 ** Returns          The authorization currently configured
 **
 *******************************************************************************/
tBSA_SEC_AUTH_RESP app_sec_get_auth(void)
{
    return (app_sec_cb.auth);
}


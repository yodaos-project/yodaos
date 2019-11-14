/*****************************************************************************
**
**  Name:           app_sec.h
**
**  Description:    Application Security Functions
**
**  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef APP_SEC_H
#define APP_SEC_H

#include <bsa_rokid/bsa_api.h>

/* Security custom Callback */
typedef BOOLEAN (tAPP_SEC_CUSTOM_CBACK)(tBSA_SEC_EVT event, tBSA_SEC_MSG *p_data);

typedef enum
{
    APP_SEC_AUTO_PAIR_MODE,
    APP_SEC_MANUAL_PAIR_MODE
} tAPP_SEC_PAIRING_MODE;

typedef struct
{
    BOOLEAN auto_accept_pairing;
    PIN_CODE pin_code; /* PIN Code */
    UINT8 pin_len; /* PIN Length */
    BD_ADDR sec_bd_addr; /* BdAddr of peer device requesting SP */
    tAPP_SEC_CUSTOM_CBACK *sec_custom_cback; /* Application security callback */
    BOOLEAN ssp_debug;
    tBSA_SEC_AUTH_RESP auth;
    BOOLEAN is_ble;
} tAPP_SECURITY_CB;

extern tAPP_SECURITY_CB app_sec_cb;

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
 *
 *******************************************************************************/
int app_sec_bond(BD_ADDR bdaddr);

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
int app_sec_bond_cancel(BD_ADDR bdaddr);

/*******************************************************************************
 **
 ** Function         app_sec_sp_cfm_reply
 **
 ** Description      Function used to accept/refuse Simple Pairing
 **
 ** Parameters
 **
 ** Returns          0 if success/-1 otherwise
 **
 *******************************************************************************/
int app_sec_sp_cfm_reply(BOOLEAN accept, BD_ADDR bd_addr);

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
int app_sec_set_security(tAPP_SEC_CUSTOM_CBACK *app_sec_cback);

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
int app_sec_set_ssp_debug_mode(BOOLEAN debug_mode);

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
int app_sec_pin_reply(void);

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
int app_sec_set_pairing_mode(tAPP_SEC_PAIRING_MODE mode);

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
int app_sec_set_auth(tBSA_SEC_AUTH_RESP auth);

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
tBSA_SEC_AUTH_RESP app_sec_get_auth(void);

#endif

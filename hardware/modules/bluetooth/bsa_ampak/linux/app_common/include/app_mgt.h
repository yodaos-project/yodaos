/*****************************************************************************
**
**  Name:           app_mgt.h
**
**  Description:    Bluetooth Management function
**
**  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

/* idempotency */
#ifndef APP_MGT_H
#define APP_MGT_H

/* self-sufficiency */
#include <bsa_rokid/bsa_mgt_api.h>

/*
 * Definitions
 */
#ifndef APP_DEFAULT_UIPC_PATH
#define APP_DEFAULT_UIPC_PATH "/data/bluetooth/"
#endif

/* Management Custom Callback */
typedef BOOLEAN (tAPP_MGT_CUSTOM_CBACK)(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data);

/*******************************************************************************
 **
 ** Function        app_mgt_init
 **
 ** Description     Init management
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_mgt_init(void);

/*******************************************************************************
 **
 ** Function        app_mgt_open
 **
 ** Description     Open communication to BSA Server
 **
 ** Parameters      uipc_path: path to UIPC channels (if NULL, points to CWD)
 **                 p_mgt_callback: Application's custom callback
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_mgt_open(const char *p_uipc_path, tAPP_MGT_CUSTOM_CBACK *p_mgt_callback);

/*******************************************************************************
 **
 ** Function        app_mgt_close
 **
 ** Description     This function is used to closes the BSA connection
 **
 ** Parameters      None
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_mgt_close(void);

/*******************************************************************************
 **
 ** Function         app_mgt_kill_server
 **
 ** Description      This function is used to kill the server
 **
 ** Parameters       None
 **
 ** Returns          status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_mgt_kill_server(void);
#endif /* APP_MGT_H_ */

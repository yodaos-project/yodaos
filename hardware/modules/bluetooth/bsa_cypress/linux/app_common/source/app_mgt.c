/*****************************************************************************
**
**  Name:           app_mgt.c
**
**  Description:    Bluetooth Management function
**
**  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/


#include <unistd.h>

#include "app_mgt.h"
#include "app_utils.h"


/*
 * Global variables
 */
struct
{
    tAPP_MGT_CUSTOM_CBACK* mgt_custom_cback;
} app_mgt_cb;

/*******************************************************************************
 **
 ** Function        app_mgt_generic_callback
 **
 ** Description     This is an example of Generic Management callback function.
 **                 The Management Callback function is called in case of server
 **                 disconnection (e.g. server crashes) or when the Bluetooth
 **                 status changes (enable/disable)
 **
 ** Parameters      event: the event received (Status or Disconnect event)
 **                 p_data:associated data
 **
 ** Returns         void
 **
 *******************************************************************************/
static void app_mgt_generic_callback(tBSA_MGT_EVT event, tBSA_MGT_MSG *p_data)
{
    BOOLEAN exit_generic_cback= FALSE;

    /*  If Application provided its own custom callback */
    if(app_mgt_cb.mgt_custom_cback != NULL)
    {
        /* Call it */
        exit_generic_cback = app_mgt_cb.mgt_custom_cback(event, p_data);
    }

    /* If custom callback indicates that does not need the generic callback to execute */
    if(exit_generic_cback != FALSE)
    {
        return;
    }

    switch(event)
    {
    case BSA_MGT_STATUS_EVT:
        APP_INFO0("app_mgt_generic_callback BSA_MGT_STATUS_EVT");
        if (p_data->status.enable == FALSE)
        {
            APP_INFO0("\tBluetooth Stopped");
        }
        else
        {
            APP_INFO0("\tBluetooth restarted");
        }
        break;

    case BSA_MGT_DISCONNECT_EVT:
        /* Connection with the Server lost => Application will have to reconnect */
        APP_INFO1("app_mgt_generic_callback BSA_MGT_DISCONNECT_EVT reason:%d", p_data->disconnect.reason);
        break;
    }
}

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
int app_mgt_init(void)
{
    memset(&app_mgt_cb, 0, sizeof(app_mgt_cb));
    return 0;
}

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
int app_mgt_open(const char *p_uipc_path, tAPP_MGT_CUSTOM_CBACK *p_mgt_callback)
{
    tBSA_MGT_OPEN bsa_open_param;
    tBSA_STATUS bsa_status;
    int i;

    BSA_MgtOpenInit(&bsa_open_param);

    /* If the application passed a NULL UIPC path, use the default one */
    if (p_uipc_path == NULL)
    {
        strncpy(bsa_open_param.uipc_path, APP_DEFAULT_UIPC_PATH, sizeof(bsa_open_param.uipc_path) - 1);
        bsa_open_param.uipc_path[sizeof(bsa_open_param.uipc_path) - 1] = '\0';
    }
    /* Else, use the given path */
    else
    {
        strncpy(bsa_open_param.uipc_path, p_uipc_path, sizeof(bsa_open_param.uipc_path) - 1);
        bsa_open_param.uipc_path[sizeof(bsa_open_param.uipc_path) - 1] = '\0';
    }

    /* Use the Generic Callback */
    bsa_open_param.callback = app_mgt_generic_callback;

    /* Save the Application's custom callback */
    app_mgt_cb.mgt_custom_cback = p_mgt_callback;

    /* Let's try to connect several time */
    for (i = 0 ; i < 5 ; i++)
    {
        /* Connect to BSA Server */
        bsa_status = BSA_MgtOpen(&bsa_open_param);
        if (bsa_status == BSA_SUCCESS)
        {
            break;
        }
        APP_ERROR1("Connection to server unsuccessful (%d), retrying...", i);
        sleep(1);
    }
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR0("Unable to connect to server");
        return -1;
    }

    return 0;
}


/*******************************************************************************
 **
 ** Function        app_mgt_close
 **
 ** Description     This function is used to closes the BSA connection
 **
 ** Returns         The status of the operation
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_mgt_close(void)
{
    tBSA_MGT_CLOSE  bsa_close_param;
    tBSA_STATUS bsa_status;

    BSA_MgtCloseInit(&bsa_close_param);
    bsa_status = BSA_MgtClose(&bsa_close_param);
    if(bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_MgtClose failed status:%d", bsa_status);
        return -1;
    }
    return 0;
}

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
int app_mgt_kill_server(void)
{
    tBSA_MGT_KILL_SERVER param;

    if(BSA_MgtKillServerInit(&param) == BSA_SUCCESS)
    {
        if(BSA_MgtKillServer(&param) == BSA_SUCCESS)
        {
            return 0;
        }
    }

    return -1;
}

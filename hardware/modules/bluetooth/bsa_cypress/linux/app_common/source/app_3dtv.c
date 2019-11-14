/*****************************************************************************
 **
 **  Name:           app_3dtv.c
 **
 **  Description:    3D TV utility functions for applications
 **
 **  Copyright (c) 2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#include <bsa_rokid/bsa_api.h>
#include "app_utils.h"
#include "app_3dtv.h"

/*
 * Definitions
 */
/* Broadcom VSC which enable/disable VSync detection (for manufacturer tests) */
#define BRCM_HCI_VSC_3D_ENABLE_VSYNC_DETECT     0x00E3

/* Broadcom VSC which controls Dual Mode  */
#define BRCM_HCI_VSC_3D_SET_DUAL_MODE           0x00F4

/* Broadcom VSC which controls 3D Broadcasted information  */
#define BRCM_HCI_VSC_3D_SET_OFFSET              0x00D6

/*
 * Global variables
 */
tAPP_3DTV_CB app_3dtv_cb;


/*******************************************************************************
 **
 ** Function        app_3dtv_callback
 **
 ** Description     DM Callback for 3DG Sync profile.
 **
 ** Returns         void
 **
 *******************************************************************************/
static void app_3dtv_callback(tBSA_DM_EVT event, tBSA_DM_MSG *p_data)
{
    APP_DEBUG0("app_3dtv_callback for 3DG sync profile");

    switch(event)
    {
    case BSA_DM_3D_ANNOUCEMENT_EVT:
        APP_INFO1("BSA_DM_3D_ANNOUCEMENT_EVT BdAddr:%02X-%02X-%02X-%02X-%02X-%02X",
                p_data->announcement.bd_addr[0],
                p_data->announcement.bd_addr[1],
                p_data->announcement.bd_addr[2],
                p_data->announcement.bd_addr[3],
                p_data->announcement.bd_addr[4],
                p_data->announcement.bd_addr[5]);
        if (p_data->announcement.type & BSA_3DS_ANNOUNC_TYPE_ASSOS_NOTIF_MASK)
        {
            APP_INFO1("\t3DG Association Notification Type:0x%X", p_data->announcement.type);
        }
        else if (p_data->announcement.type & BSA_3DS_ANNOUNC_TYPE_BAT_LVL_RPT_MASK)
        {
            APP_INFO1("\t3DG Battery Report Type:0x%X", p_data->announcement.type);
        }
        else
        {
            APP_INFO1("\t3DG Connection Announcement Type:0x%X", p_data->announcement.type);
        }
        APP_INFO1("\tBattery Level:0x%X(%d)",
                p_data->announcement.battery_level,
                p_data->announcement.battery_level);
        if (p_data->announcement.additional_data_len)
        {
            APP_INFO1("\tAdditionnal Data Length:%d", p_data->announcement.additional_data_len);
            APP_DUMP("\tAdditionnal Data",
                    p_data->announcement.additional_data,
                    p_data->announcement.additional_data_len);
        }
        break;

    default:
        /* Ignore other events */
        break;

    }
}

/*******************************************************************************
 **
 ** Function        app_3dtv_init
 **
 ** Description     Initialize 3DTV with default values
 **
 ** Parameters      none
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_init(void)
{
    app_3dtv_cb.path_loss_threshold = APP_3DTV_PATH_LOSS_THRESHOLD;
    app_3dtv_cb.dtv_mode = APP_3DTV_MODE_IDLE;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_default_pathloss_threshold
 **
 ** Description     Set the default path loss threshold for all mode changes
 **
 ** Parameters      path_loss_threshold: new path loss threshold value
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_default_pathloss_threshold(UINT8 path_loss_threshold)
{
    app_3dtv_cb.path_loss_threshold = path_loss_threshold;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_dual_mode_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_DUAL_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_dual_mode_init(tAPP_3DTV_DUAL_MODE *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    p_req->enable = TRUE;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_dual_mode
 **
 ** Description     Configure appropriately the dual mode
 **
 ** Parameters      p_dual_mode: pointer to the tAPP_3DTV_DUAL_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_dual_mode(tAPP_3DTV_DUAL_MODE *p_dual_mode)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;
    UINT8 dual_mode = 0;

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_3D_SET_DUAL_MODE; /* Configure Dual mode */
    bsa_vsc.length = 1;         /* 1 byte */
    p_data = bsa_vsc.data;
    if (p_dual_mode->enable)
    {
        dual_mode |= APP_3DTV_DUAL_VIEW_VIDEO;
    }

    /* Add here other dual_mode values (e.g. dual_listen) */

    /* Write dual_mode parameter*/
    UINT8_TO_STREAM(p_data, dual_mode);

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_TmVsc failed:%d", bsa_status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_idle_mode_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_IDLE_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_idle_mode_init(tAPP_3DTV_IDLE_MODE *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    p_req->path_loss_threshold = app_3dtv_cb.path_loss_threshold;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_idle_mode
 **
 ** Description     Configure appropriately the idle mode
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_IDLE_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_idle_mode(tAPP_3DTV_IDLE_MODE *p_req)
{
    int status = -1;
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_CONFIG  bt_config;

    if (p_req == NULL)
    {
        APP_ERROR0("Null p_req parameter");
        return status;
    }

    APP_DEBUG1("BrcmMask:x%x", p_req->brcm_mask);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Indicate the specific entries to configure */
    bt_config.config_mask = 0;

    /* Obviously Enable Bluetooth */
    bt_config.enable = TRUE;

    /* Set device BD name */
    bt_config.config_mask |= BSA_DM_CONFIG_NAME_MASK;
    strncpy((char *)bt_config.name, "BSA IdleTV", sizeof(bt_config.name) - 1);
    bt_config.name[sizeof(bt_config.name) - 1] = '\0';

    /* Configure the Broadcom specific entries */
    bt_config.config_mask |= BSA_DM_CONFIG_BRCM_MASK;

    /* The pass loss threshold configuration (beyond this loss, should not connect) */
    bt_config.path_loss_threshold = p_req->path_loss_threshold;

    /* Remove Master and Slave bits of the requested mode */
    bt_config.brcm_mask = p_req->brcm_mask;
    bt_config.brcm_mask &= ~BSA_DM_3DTV_MASTER_MASK;
    bt_config.brcm_mask &= ~BSA_DM_3DTV_SLAVE_MASK;

    /* Set callback */
    bt_config.callback = app_3dtv_callback;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    /* 3DTV is in Idle mode */
    app_3dtv_cb.dtv_mode = APP_3DTV_MODE_IDLE;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_master_mode_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_MASTER_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_master_mode_init(tAPP_3DTV_MASTER_MODE *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    p_req->path_loss_threshold = app_3dtv_cb.path_loss_threshold;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_master_mode
 **
 ** Description     Function example to configure 3DTV in Master mode
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_MASTER_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_master_mode(tAPP_3DTV_MASTER_MODE *p_req)
{
    int status = -1;
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_CONFIG  bt_config;

    if (p_req == NULL)
    {
        APP_ERROR0("Null p_req parameter");
        return status;
    }

    APP_DEBUG1("BrcmMask:x%x", p_req->brcm_mask);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Indicate the specific entries to configure */
    bt_config.config_mask = 0;

    /* Obviously Enable Bluetooth */
    bt_config.enable = TRUE;

    /* Set device BD name */
    bt_config.config_mask |= BSA_DM_CONFIG_NAME_MASK;
    strncpy((char *)bt_config.name, "BSA MasterTV", sizeof(bt_config.name) - 1);
    bt_config.name[sizeof(bt_config.name) - 1] = '\0';

    /* Configure the Broadcom specific entries */
    bt_config.config_mask |= BSA_DM_CONFIG_BRCM_MASK;

    /* The pass loss threshold configuration (beyond this loss, should not connect) */
    bt_config.path_loss_threshold = p_req->path_loss_threshold;

    /* Remove Slave and Set Master bits of the requested mode */
    bt_config.brcm_mask = p_req->brcm_mask;
    bt_config.brcm_mask &= ~BSA_DM_3DTV_SLAVE_MASK;
    bt_config.brcm_mask |= BSA_DM_3DTV_MASTER_MASK;

    /* Set callback */
    bt_config.callback = app_3dtv_callback;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    /* 3DTV is in Master mode */
    app_3dtv_cb.dtv_mode = APP_3DTV_MODE_MASTER;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_slave_mode_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_SLAVE_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_slave_mode_init(tAPP_3DTV_SLAVE_MODE *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    p_req->path_loss_threshold = app_3dtv_cb.path_loss_threshold;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_slave_mode
 **
 ** Description     Function example to configure 3DTV in Slave mode
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_SLAVE_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_slave_mode(tAPP_3DTV_SLAVE_MODE *p_req)
{
    int status = -1;
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_CONFIG  bt_config;

    if (p_req == NULL)
    {
        APP_ERROR0("Null p_req parameter");
        return status;
    }

    APP_DEBUG1("BrcmMask:x%x BdAddr:%02X:%02X:%02X:%02X:%02X:%02X", p_req->brcm_mask,
            p_req->bd_addr_to_lock[0], p_req->bd_addr_to_lock[1],
            p_req->bd_addr_to_lock[2], p_req->bd_addr_to_lock[3],
            p_req->bd_addr_to_lock[4], p_req->bd_addr_to_lock[5]);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Indicate the specific entries to configure */
    bt_config.config_mask = 0;

    /* Obviously Enable Bluetooth*/
    bt_config.enable = TRUE;

    /* Set device BD name */
    bt_config.config_mask |= BSA_DM_CONFIG_NAME_MASK;
    strncpy((char *)bt_config.name, "BSA SlaveTV", sizeof(bt_config.name) - 1);
    bt_config.name[sizeof(bt_config.name) - 1] = '\0';

    /* Configure the Broadcom specific entries */
    bt_config.config_mask |= BSA_DM_CONFIG_BRCM_MASK;

    /* The pass loss threshold configuration (beyond this loss, should not connect) */
    bt_config.path_loss_threshold = p_req->path_loss_threshold;

    /* Set Slave and Reset Master bits of the requested mode */
    bt_config.brcm_mask = p_req->brcm_mask;
    bt_config.brcm_mask |= BSA_DM_3DTV_SLAVE_MASK;
    bt_config.brcm_mask &= ~BSA_DM_3DTV_MASTER_MASK;

    /* Copy the Master BdAddr to lock/connect to */
    bdcpy(bt_config.master_3d_bd_addr, p_req->bd_addr_to_lock);

    /* Set callback */
    bt_config.callback = app_3dtv_callback;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    /* 3DTV is in Slave mode */
    app_3dtv_cb.dtv_mode = APP_3DTV_MODE_SLAVE;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_send_tx_data_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_SLAVE_MODE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_send_tx_data_init(tAPP_3DTV_TX_DATA *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_send_tx_data
 **
 ** Description     Function example to Send 3D Data (offset)
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_TX_DATA
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_send_tx_data(tAPP_3DTV_TX_DATA *p_req)
{
    int status = -1;
    tBSA_STATUS bsa_status;
    tBSA_DM_SET_CONFIG  set_config;

    if (p_req == NULL)
    {
        APP_ERROR0("Null p_req parameter");
        return status;
    }

    APP_DEBUG1("LeftOpen:%d LeftClose:%d RightOpen:%d RightCode:%d Delay:%d DualView:0x%02X",
            p_req->left_open, p_req->left_close,
            p_req->right_open, p_req->right_close,
            p_req->delay, p_req->dual_view_mode);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&set_config);

    /* Obviously Enable Bluetooth*/
    set_config.enable = TRUE;

    /* Configure the Broadcom specific entries */
    set_config.config_mask = BSA_DM_CONFIG_BRCM_MASK;

    /* Indicate that we want to Send Tx Data only (do not change any other 3D setting) */
    set_config.brcm_mask = BSA_DM_3D_TX_DATA_ONLY_MASK;

    set_config.tx_data_3d.left_open_offset = p_req->left_open;
    set_config.tx_data_3d.left_close_offset = p_req->left_close;
    set_config.tx_data_3d.right_open_offset = p_req->right_open;
    set_config.tx_data_3d.right_close_offset = p_req->right_close;
    set_config.tx_data_3d.delay = p_req->delay;
    set_config.tx_data_3d.dual_view = p_req->dual_view_mode;

    /* Set callback */
    set_config.callback = app_3dtv_callback;

    bsa_status = BSA_DmSetConfig(&set_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_beacon_init
 **
 ** Description     Initialize structure with default values
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_SET_BEACON
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_beacon_init(tAPP_3DTV_SET_BEACON *p_req)
{
    memset(p_req, 0, sizeof(*p_req));
    /* Use some default values for test purpose */
    p_req->display_id = 0x11;
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_set_beacon
 **
 ** Description     Configure 3DTV Beacon parameters (Broadcasted)
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_SET_BEACON
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_set_beacon(tAPP_3DTV_SET_BEACON *p_req)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p_data;

    if (p_req == NULL)
    {
        APP_ERROR0("Null p_req parameter");
        return -1;
    }

    APP_DEBUG1("LeftOpen:%d LeftClose:%d RightOpen:%d RightClose:%d Delay:%d",
            p_req->left_open, p_req->left_close,
            p_req->right_open, p_req->right_close,
            p_req->delay);

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = BRCM_HCI_VSC_3D_SET_OFFSET; /* 3D Offset and Delay */
    p_data = bsa_vsc.data;
#ifdef APP_3DTV_OLD_OFFSET_VSC
    bsa_vsc.length = sizeof(UINT16) * 5; /* 5 words of 16 bits */
    UINT16_TO_STREAM(p_data, p_req->left_open);
    UINT16_TO_STREAM(p_data, p_req->left_close);
    UINT16_TO_STREAM(p_data, p_req->right_open);
    UINT16_TO_STREAM(p_data, p_req->right_close);
    UINT16_TO_STREAM(p_data, p_req->delay);
#else
    APP_DEBUG1("DisplayId:%d DualView:%d", p_req->display_id, p_req->dual_view_mode);

    bsa_vsc.length = 12;

    /* DisplayId */
    UINT8_TO_STREAM(p_data, p_req->display_id);

    /* Left Open Offset */
    UINT16_TO_STREAM(p_data, p_req->left_open);

    /* Left Close Offset */
    UINT16_TO_STREAM(p_data, p_req->left_close);

    /* Right Open Offset */
    UINT16_TO_STREAM(p_data, p_req->right_open);

    /* Right Close Offset */
    UINT16_TO_STREAM(p_data, p_req->right_close);

    /* Delay */
    UINT16_TO_STREAM(p_data, p_req->delay);

    /* DualView */
    UINT8_TO_STREAM(p_data, p_req->dual_view_mode);

#endif
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_TmVsc failed status:%d", bsa_status);
        return(-1);
    }

    if ((bsa_vsc.status != BSA_SUCCESS) || (bsa_vsc.data[0] != HCI_SUCCESS))
    {
        APP_ERROR1("Unable to Set 3D offsets status:%d hic_status:%d",
                bsa_vsc.status, bsa_vsc.data[0]);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_3dtv_enable_vsync_detection
 **
 ** Description     Enable vsync detection
 **
 ** Parameters      enable_detection: TRUE/FALSE
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_enable_vsync_detection(BOOLEAN enable_detection)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;
    UINT8 *p = &bsa_vsc.data[0];

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);

    /* Enable_3dmode_vsync_detection OpCode */
    bsa_vsc.opcode = BRCM_HCI_VSC_3D_ENABLE_VSYNC_DETECT;

    /* Length of the parameters (1 byte) */
    bsa_vsc.length = 1;

    if (enable_detection == FALSE)
    {
        /* Write the boolean parameter to disable */
        UINT8_TO_STREAM(p, 0x00);
    }
    else
    {
        /* Write the boolean parameter to enable */
        UINT8_TO_STREAM(p, 0x01);
    }

    /* Send the VSC */
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    if ((bsa_vsc.status != BSA_SUCCESS) || (bsa_vsc.data[0] != HCI_SUCCESS))
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }

    return (0);
}



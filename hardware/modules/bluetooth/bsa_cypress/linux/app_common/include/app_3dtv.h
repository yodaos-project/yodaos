/*****************************************************************************
 **
 **  Name:           app_3dtv.h
 **
 **  Description:    3D TV utility functions for applications
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* idempotence guarantee */
#ifndef APP_3DTV_H
#define APP_3DTV_H

/* Self sufficiency */
#include <bsa_rokid/data_types.h>

/*
 * Definitions
 */
#ifndef APP_3DTV_PATH_LOSS_THRESHOLD
#define APP_3DTV_PATH_LOSS_THRESHOLD 60
#endif

#ifndef APP_3DTV_VSYNC_DELAY
#define APP_3DTV_VSYNC_DELAY 0
#endif

#define APP_3DTV_DUAL_VIEW_VIDEO        0x01
#define APP_3DTV_DUAL_VIEW_AUDIO_USED   0x80
#define APP_3DTV_DUAL_VIEW_AUDIO_ON     0x40


/* 3DTV modes */
typedef enum
{
    APP_3DTV_MODE_IDLE = 0, /* DTV is in Idle mode */
    APP_3DTV_MODE_MASTER, /* DTV is in Master mode */
    APP_3DTV_MODE_SLAVE /* DTV is in Slave mode */
} tAPP_3DTV_MODE;

typedef struct
{
    UINT8 path_loss_threshold; /* Path Loss Threshold*/
    tAPP_3DTV_MODE dtv_mode; /* Current DTV Mode */
} tAPP_3DTV_CB;

typedef struct
{
    BOOLEAN enable;
} tAPP_3DTV_DUAL_MODE;

typedef struct
{
    tBSA_DM_MASK brcm_mask; /* Broadcom Specific Mask */
    UINT8 path_loss_threshold; /* Path Loss Threshold*/
} tAPP_3DTV_IDLE_MODE;

typedef struct
{
    tBSA_DM_MASK brcm_mask; /* Broadcom Specific Mask */
    UINT8 path_loss_threshold; /* Path Loss Threshold*/
} tAPP_3DTV_MASTER_MODE;

typedef struct
{
    tBSA_DM_MASK brcm_mask; /* Broadcom Specific Mask */
    BD_ADDR bd_addr_to_lock;
    UINT8 path_loss_threshold; /* Path Loss Threshold*/
} tAPP_3DTV_SLAVE_MODE;

typedef struct
{
    UINT16 left_open; /* Left Open Offset */
    UINT16 left_close; /* Left Close Offset */
    UINT16 right_open; /* Right Open Offset */
    UINT16 right_close; /* Right Close Offset */
    UINT16 delay; /* VSync Delay */
    UINT8 dual_view_mode; /* Dual View */
    UINT8 display_id; /* Display Identification */
} tAPP_3DTV_SET_BEACON;

typedef struct
{
    UINT16 left_open; /* Left Open Offset */
    UINT16 left_close; /* Left Close Offset */
    UINT16 right_open; /* Right Open Offset */
    UINT16 right_close; /* Right Close Offset */
    UINT16 delay; /* VSync Delay */
    UINT8 dual_view_mode; /* Dual View */
} tAPP_3DTV_TX_DATA;
/*
 * Global variables
 */
extern tAPP_3DTV_CB app_3dtv_cb;

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
int app_3dtv_init(void);

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
int app_3dtv_set_default_pathloss_threshold(UINT8 path_loss_threshold);

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
int app_3dtv_set_dual_mode_init(tAPP_3DTV_DUAL_MODE *p_req);

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
int app_3dtv_set_dual_mode(tAPP_3DTV_DUAL_MODE *p_dual_mode);

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
int app_3dtv_set_idle_mode_init(tAPP_3DTV_IDLE_MODE *p_req);

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
int app_3dtv_set_idle_mode(tAPP_3DTV_IDLE_MODE *p_req);

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
int app_3dtv_set_master_mode_init(tAPP_3DTV_MASTER_MODE *p_req);

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
int app_3dtv_set_master_mode(tAPP_3DTV_MASTER_MODE *p_req);

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
int app_3dtv_set_slave_mode_init(tAPP_3DTV_SLAVE_MODE *p_req);

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
int app_3dtv_set_slave_mode(tAPP_3DTV_SLAVE_MODE *p_req);

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
int app_3dtv_send_tx_data_init(tAPP_3DTV_TX_DATA *p_req);

/*******************************************************************************
 **
 ** Function        app_3dtv_set_tx_data
 **
 ** Description     Function example to Send 3D Data (offset)
 **
 ** Parameters      p_req: pointer to the tAPP_3DTV_TX_DATA
 **
 ** Returns         0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_3dtv_send_tx_data(tAPP_3DTV_TX_DATA *p_req);

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
int app_3dtv_set_beacon_init(tAPP_3DTV_SET_BEACON *p_req);

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
int app_3dtv_set_beacon(tAPP_3DTV_SET_BEACON *p_req);

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
int app_3dtv_enable_vsync_detection(BOOLEAN enable_detection);

#endif /* APP_3DTV_H */

/*****************************************************************************
 **
 **  Name:               app_dm.h
 **
 **  Description:        Device Management utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/
#ifndef APP_DM_H
#define APP_DM_H

#include <bsa_rokid/bsa_api.h>
#include <bsa_rokid/data_types.h>

/*******************************************************************************
 **
 ** Function         app_dm_get_local_bt_config
 **
 ** Description      Get the Bluetooth configuration
 **
 ** Parameters
 **
 ** Returns
 **             1 if BT enabled/0 if BT disabled/-1 in case of error
 **
 *******************************************************************************/
int app_dm_get_local_bt_config(void);

/*******************************************************************************
 **
 ** Function        app_dm_set_local_bt_config
 **
 ** Description     Set the Bluetooth basic configuration
 **
 ** Parameters      enable: 0 to disable Bluetooth; 1 to enable it
 **
 ** Returns         0 if success /-1 in case of error
 **
 *******************************************************************************/
int app_dm_set_local_bt_config(BOOLEAN enable);

/*******************************************************************************
 **
 ** Function         app_dm_set_channel_map
 **
 ** Description      This function is used to avoid an AFH channel range
 **
 ** Parameters
 **             first_afh_ch: first afh channel to avoid
 **             last_afh_ch: last afh channel to avoid
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_channel_map(int first_afh_ch, int last_afh_ch);

/*******************************************************************************
 **
 ** Function         app_dm_set_tx_class
 **
 ** Description      This function sets the Tx Power Class
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_dm_set_tx_class(UINT8 tx_power);

/*******************************************************************************
 **
 ** Function        app_dm_set_page_scan_param
 **
 ** Description     Configure the Page Scan parameters (interval & window)
 **
 ** Parameters      Interval: PageScan Interval (in slot)
 **                 windows: PageScan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_page_scan_param(UINT16 interval, UINT16 window);

/*******************************************************************************
 **
 ** Function        app_dm_set_inquiry_scan_param
 **
 ** Description     Configure the Inquiry Scan parameters (interval & window)
 **
 ** Parameters      Interval: InquiryScan Interval (in slot)
 **                 windows: InquiryScan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_inquiry_scan_param(UINT16 interval, UINT16 window);

/*******************************************************************************
 **
 ** Function        app_dm_set_device_class
 **
 ** Description     Set the Device Class
 **
 ** Parameters      services: Service Class
 **                 major: Major Class
 **                 minor: Minor Class
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_device_class(UINT16 services, UINT8 major, UINT8 minor);

/*******************************************************************************
 **
 ** Function        app_dm_set_visibility
 **
 ** Description     Set the Device Visibility parameters (InquiryScan & PageScan)
 **
 ** Parameters      discoverable: FALSE if not discoverable
 **                 connectable: FALSE if not connectable
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_visibility(BOOLEAN discoverable, BOOLEAN connectable);

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function        app_dm_set_ble_visibility
 **
 ** Description     Set the Device BLE Visibility parameters
 **
 ** Parameters      discoverable: FALSE if not discoverable
 **                 connectable: FALSE if not connectable
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_visibility(BOOLEAN discoverable, BOOLEAN connectable);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_local_privacy
 **
 ** Description     Set the Device BLE privacy on the local device
 **
 ** Parameters      privacy_enable: enable/disable BLE local privacy
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_local_privacy(BOOLEAN privacy_enable);
#endif

/*******************************************************************************
 **
 ** Function        app_dm_set_dual_stack_mode
 **
 ** Description     Switch Stack
 **
 ** Parameters      Switch Stack mode
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_dual_stack_mode(tBSA_DM_DUAL_STACK_MODE dual_stack_mode);

/*******************************************************************************
 **
 ** Function        app_dm_get_chip_id
 **
 ** Description     Get BT Chip ID
 **
 ** Parameters      none
 **
 ** Returns         Chip ID if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_get_chip_id(void);

/*******************************************************************************
 **
 ** Function        app_dm_get_dual_stack_mode
 **
 ** Description     Get current Switch Stack mode
 **
 ** Parameters      none
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_get_dual_stack_mode(void);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_bg_conn_type
 **
 ** Description     Configure BLE Background Connection type
 **                 to make automatic connection to BLE device
 **
 ** Parameters      type : 0 - None, 1 - Auto
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_bg_conn_type(int type);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_scan_param
 **
 ** Description     Configure the BLE Scan parameters (interval & window)
 **
 ** Parameters      Interval: BLE Scan Interval (in slot)
 **                 windows: BLE Scan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_scan_param(UINT16 interval, UINT16 window);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_conn_param
 **
 ** Description     Configure the BLE Prefer Conn parameters
 **
 ** Parameters      pointer to tBSA_DM_BLE_CONN_PARAM
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_conn_param(tBSA_DM_BLE_CONN_PARAM *p_req);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_conn_scan_param
 **
 ** Description     Configure the BLE Connecton Scan parameters (interval & window)
 **
 ** Parameters      Interval: BLE Scan Interval (in slot)
 **                 windows: BLE Scan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_conn_scan_param(UINT16 interval, UINT16 window);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_adv_data
 **
 ** Description     Configure the BLE advertisng data
 **
 ** Parameters      pointer to tBSA_DM_BLE_ADV_CONFIG
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_adv_data(tBSA_DM_BLE_ADV_CONFIG *p_data);

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_adv_param
 **
 ** Description     Configure the BLE Prefer Advertisement parameters
 **
 ** Parameters      pointer to tBSA_DM_BLE_ADV_PARAM
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_adv_param(tBSA_DM_BLE_ADV_PARAM *p_req);

/*******************************************************************************
 **
 ** Function        app_dm_monitor_rssi
 **
 ** Description     Enables/Disables periodic monitoring of the RSSI
 **                 for all connected devices
 **
 ** Parameters      enable : 1 - Enable, 0 - Disable
 **                 period : Measurement period in seconds
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_monitor_rssi(BOOLEAN enable, UINT16 period);
#endif

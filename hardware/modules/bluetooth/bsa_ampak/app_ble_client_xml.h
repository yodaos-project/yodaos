/*****************************************************************************
 **
 **  Name:           app_ble_client_xml.h
 **
 **  Description:    This module contains utility functions to access BLE
 **                  saved parameters
 **
 **  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#ifndef APP_BLE_CLINET_XML_H
#define APP_BLE_CLINET_XML_H

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_init
 **
 ** Description     Initialize XML parameter system (nothing for the moment)
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_ble_client_xml_init(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_read
 **
 ** Description     Load the BLE data base from  XML file
 **
 ** Parameters      p_app_cfg: Application Configuration buffer
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_ble_client_xml_read(tAPP_BLE_CLIENT_DB_ELEMENT *p_element);


/*******************************************************************************
 **
 ** Function        app_ble_client_xml_write
 **
 ** Description     Writes the database file in file system or XML, creates one if it doesn't exist
 **
 ** Parameters      p_app_cfg: Application Configuration to save to file
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_ble_client_xml_write(tAPP_BLE_CLIENT_DB_ELEMENT *p_element);

#endif

/*****************************************************************************
 **
 **  Name:           app_xml_utils.h
 **
 **  Description:    Bluetooth XML utility functions
 **
 **  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_XML_UTILS_H
#define APP_XML_UTILS_H

#include "app_xml_param.h"

//#ifndef APP_MAX_NB_REMOTE_STORED_DEVICES
#define APP_MAX_NB_REMOTE_STORED_DEVICES 20
//#endif

#ifndef APP_MAX_SI_DEVICES
#define APP_MAX_SI_DEVICES 10
#endif

/*
 * Global Variables
 */
extern tAPP_XML_REM_DEVICE app_xml_remote_devices_db[APP_MAX_NB_REMOTE_STORED_DEVICES];
extern tAPP_XML_SI_DEVICE app_xml_si_devices_db[APP_MAX_SI_DEVICES];

#define APP_XML_CONFIG_FILE_PATH            "/data/bluetooth/bt_config.xml"
#define APP_XML_REM_DEVICES_FILE_PATH       "/data/bluetooth/bt_devices.xml"
#define APP_XML_LINK_STATUS_FILE_PATH       "/data/bluetooth/bt_link.xml"
#define APP_XML_SI_DEVICES_FILE_PATH        "/data/bluetooth/si_devices.xml"
#ifndef tAPP_BLE_CLIENT_XML_FILENAME
#define tAPP_BLE_CLIENT_XML_FILENAME 		"/data/bluetooth/bt_ble_client_devices.xml"
#endif
#ifndef tAPP_HH_XML_FILENAME
#define tAPP_HH_XML_FILENAME "/data/bluetooth/bt_hh_devices.xml"
#endif


/*******************************************************************************
 **
 ** Function        app_read_xml_config
 **
 ** Description     This function is used to read the XML Bluetooth configuration file
 **
 ** Parameters      p_xml_config: pointer to the structure to fill
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_read_xml_config(tAPP_XML_CONFIG *p_xml_config);

/*******************************************************************************
 **
 ** Function        app_write_xml_config
 **
 ** Description     This function is used to write the XML Bluetooth configuration file
 **
 ** Parameters      p_xml_config: pointer to the structure to write
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_write_xml_config(const tAPP_XML_CONFIG *p_xml_config);

/*******************************************************************************
 **
 ** Function        app_read_xml_remote_devices
 **
 ** Description     This function is used to read the XML Bluetooth remote device file
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_read_xml_remote_devices(void);

/*******************************************************************************
 **
 ** Function        app_write_xml_remote_devices
 **
 ** Description     This function is used to write the XML Bluetooth remote device file
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_write_xml_remote_devices(void);

/*******************************************************************************
 **
 ** Function        app_xml_is_device_index_valid
 **
 ** Description     This function is used to check if a device index is valid
 **
 ** Parameters      index: index to test in the XML remote device list
 **
 ** Returns         1 if device index is valid and in use, 0 otherwise
 **
 *******************************************************************************/
int app_xml_is_device_index_valid(int index);

/*******************************************************************************
 **
 ** Function        app_read_xml_si_devices
 **
 ** Description     This function is used to read the XML Bluetooth SI device file
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_read_xml_si_devices(void);

/*******************************************************************************
 **
 ** Function        app_write_xml_si_devices
 **
 ** Description     This function is used to write the XML Bluetooth SI device file
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, -1 otherwise
 **
 *******************************************************************************/
int app_write_xml_si_devices(void);

/*******************************************************************************
 **
 ** Function        app_xml_open_tag
 **
 ** Description     Open an XML tag
 **
 ** Parameters      fd: file descriptor to write to
 **                 tag: XML tag name
 **                 cr: indicate if a CR shall be added after the tag
 **
 ** Returns         status (negative code if error, positive if successful)

 **
 *******************************************************************************/
int app_xml_open_tag(int fd, const char *tag, BOOLEAN cr);

/*******************************************************************************
 **
 ** Function        app_hh_xml_close_tag
 **
 ** Description     Close an XML tag
 **
 ** Parameters      fd: file descriptor to write to
 **                 tag: XML tag name
 **                 cr: indicate if there was a CR before the tag close
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_xml_close_tag(int fd, const char *tag, BOOLEAN cr);

/*******************************************************************************
 **
 ** Function        app_xml_open_tag_with_value
 **
 ** Description     Open an XML tag with a value attribute
 **
 ** Parameters      fd: file descriptor to write to
 **                 tag: XML tag name
 **                 value: numerical value to write in value attribute
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_xml_open_tag_with_value(int fd, char *tag, int value);

/*******************************************************************************
 **
 ** Function        app_xml_open_close_tag_with_value
 **
 ** Description     Open and close an XML tag with a value attribute
 **
 ** Parameters      fd: file descriptor to write to
 **                 tag: XML tag name
 **                 value: numerical value to write in value attribute
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_xml_open_close_tag_with_value(int fd, char *tag, int value);

/*******************************************************************************
 **
 ** Function        app_xml_write_data
 **
 ** Description     Write binary data in hex format in an XML file
 **
 ** Parameters      fd: file descriptor to write to
 **                 p_buf: data buffer
 **                 length: data buffer length
 **                 cr: indicate if a CR must be added at the end of the data
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_xml_write_data(int fd, const void *p_buf, unsigned int length, BOOLEAN cr);

/*******************************************************************************
 **
 ** Function        app_xml_read_data
 **
 ** Description     Read binary data in hex format from a buffer
 **
 ** Parameters      p_datalen : returned data length
 **                 p_buf: buffer containing the human formatted data
 **                 length: buffer length
 **
 ** Returns         Allocated buffer containing the data
 **
 *******************************************************************************/
char *app_xml_read_data(unsigned int *p_datalen, const char *p_buf, unsigned int length);

/*******************************************************************************
 **
 ** Function        app_xml_read_data_length
 **
 ** Description     Read known length binary data in hex format from a buffer
 **
 ** Parameters      p_data: pointer to the data buffer to fill
 **                 datalen: length of the data buffer
 **                 p_buf: buffer containing the human formatted data
 **                 length: buffer length
 **
 ** Returns         TRUE if the data was correctly filled, FALSE otherwise
 **
 *******************************************************************************/
BOOLEAN app_xml_read_data_length(void *p_data, unsigned int datalen, const char *p_buf, unsigned int length);

/*******************************************************************************
 **
 ** Function        app_xml_read_string
 **
 ** Description     Read string from a buffer
 **
 ** Parameters      p_string: pointer to the data buffer to fill
 **                 stringlen: length of the data buffer
 **                 p_buf: buffer containing XML string
 **                 length: buffer length
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_xml_read_string(void *p_string, int stringlen, const char *p_buf, int length);

/*******************************************************************************
 **
 ** Function        app_xml_read_value
 **
 ** Description     Read a value from a string
 **
 ** Parameters      p_buf: data buffer
 **                 length: data buffer length
 **
 ** Returns         Read value
 **
 *******************************************************************************/
int app_xml_read_value(const char *p_buf, int length);

/*******************************************************************************
 **
 ** Function         app_xml_get_cod_from_bd
 **
 ** Description      Get the class of the device with the bd address
 **                  passed in parameter
 **
 ** Parameters
 **         bd_addr: bd address of the device to get the COD
 **         p_cod: pointer on class of device
 **
 ** Returns          0 if device in database / -1 otherwise
 **
 *******************************************************************************/
int app_xml_get_cod_from_bd(BD_ADDR bd_addr, DEV_CLASS* p_cod);
#endif


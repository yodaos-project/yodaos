/*****************************************************************************
 **
 **  Name:           app_hh_xml.h
 **
 **  Description:    This module contains utility functions to access HID
 **                  devices saved parameters
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#ifndef APP_HH_XML_H
#define APP_HH_XML_H

/* For the database element type */
#include "app_hh_db.h"

/*******************************************************************************
 **
 ** Function        app_hh_xml_init
 **
 ** Description     Initialize XML parameter system
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hh_xml_init(void);

/*******************************************************************************
 **
 ** Function        app_hh_xml_read
 **
 ** Description     Load the HH devices database from the XML file
 **
 ** Parameters      pp_app_hh_db_element: Pointer to the HH devices chained list
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_hh_xml_read(tAPP_HH_DB_ELEMENT **pp_app_hh_db_element);

/*******************************************************************************
 **
 ** Function        app_hh_xml_write
 **
 ** Description     Load the HH devices database to the XML file
 **
 ** Parameters      p_app_hh_db_element: HH devices chained list
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_hh_xml_write(tAPP_HH_DB_ELEMENT *p_app_hh_db_element);

#endif

/*****************************************************************************
 **
 **  Name:           app_hh_db.h
 **
 **  Description:    This module contains utility functions to access HID
 **                  devices database
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#ifndef APP_HH_DB_H
#define APP_HH_DB_H

/* For the types */
#include <bsa_rokid/bt_types.h>
#include <bsa_rokid/bsa_hh_api.h>

typedef struct app_hh_db_element
{
    struct app_hh_db_element *next;
    BD_ADDR bd_addr;
    UINT16 ssr_max_latency;
    UINT16 ssr_min_tout;
    UINT16 supervision_tout;
    unsigned int descriptor_size;
    char *descriptor;
    unsigned int brr_size;
    char *brr;
    UINT8 sub_class;
    tBSA_HH_ATTR_MASK attr_mask;
} tAPP_HH_DB_ELEMENT;

/*******************************************************************************
 Globals
 *******************************************************************************/

/*******************************************************************************
 **
 ** Function        app_hh_db_init
 **
 ** Description     Initialize database system
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hh_db_init(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_exit
 **
 ** Description     Exit the database system
 **
 ** Parameters      None
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_hh_db_exit(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_clear
 **
 ** Description     Clear the database
 **
 ** Parameters      None
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_hh_db_clear(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_reload
 **
 ** Description     Reload the database
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hh_db_reload(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_display
 **
 ** Description     Display the database contents
 **
 ** Parameters      None
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_hh_db_display(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_save
 **
 ** Description     Save the database
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hh_db_save(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_find_by_bda
 **
 ** Description     Find an element in the database if it exists
 **
 ** Parameters      bda: the BD address of the element to look for
 **
 ** Returns         Pointer to the element found, NULL if not found
 **
 *******************************************************************************/
tAPP_HH_DB_ELEMENT *app_hh_db_find_by_bda(BD_ADDR bda);

/*******************************************************************************
 **
 ** Function        app_hh_db_remove_by_bda
 **
 ** Description     Remove element from the database
 **
 ** Parameters      bda: the BD address of the element to remove
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_hh_db_remove_by_bda(BD_ADDR bda);

/*******************************************************************************
 **
 ** Function        app_hh_db_remove_db
 **
 ** Description     Remove the full database
 **
 ** Parameters      None
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_hh_db_remove_db(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_alloc_element
 **
 ** Description     Allocate a new DB element
 **
 ** Parameters      None
 **
 ** Returns         Pointer to the new element found, NULL if error
 **
 *******************************************************************************/
tAPP_HH_DB_ELEMENT *app_hh_db_alloc_element(void);

/*******************************************************************************
 **
 ** Function        app_hh_db_free_element
 **
 ** Description     Free an element
 **
 ** Parameters      p_element: Element to free
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_hh_db_free_element(tAPP_HH_DB_ELEMENT *p_element);

/*******************************************************************************
 **
 ** Function        app_hh_db_add_element
 **
 ** Description     Add a database element to the database
 **
 ** Parameters      p_element: Element to add (allocated by app_hh_db_alloc_element)
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_hh_db_add_element(tAPP_HH_DB_ELEMENT *p_element);

#endif

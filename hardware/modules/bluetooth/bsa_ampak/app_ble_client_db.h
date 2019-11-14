/*****************************************************************************
 **
 **  Name:           app_ble_db.h
 **
 **  Description:    Application Bluetooth BLE Database
 **
 **  Copyright (c) 2013, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_BLE_CLIENT_DB_H
#define APP_BLE_CLIENT_DB_H

/*
 * Global Variables
 */
typedef struct app_ble_client_db_attr
{
    tBT_UUID                 attr_UUID;
    UINT16                   handle;
    UINT8                    attr_type;
    UINT8                    id;
    UINT8                    prop;
    BOOLEAN                  is_primary;
    struct app_ble_client_db_attr *next;
} tAPP_BLE_CLIENT_DB_ATTR;


typedef struct app_ble_client_db_element
{
    tAPP_BLE_CLIENT_DB_ATTR          *p_attr;
    BD_ADDR                     bd_addr;
    UINT16                      app_handle; /* APP Handle */
    struct app_ble_client_db_element *next;
} tAPP_BLE_CLIENT_DB_ELEMENT;


/*******************************************************************************
 **
 ** Function        app_ble_client_db_init
 **
 ** Description     Initialize database system
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_ble_client_db_init(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_reload
 **
 ** Description     Reload the database
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_ble_client_db_reload(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_save
 **
 ** Description     Save the database
 **
 ** Parameters      None
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_ble_client_db_save(void);

/*******************************************************************************
 **
 ** Function        app_ble_db_clear
 **
 ** Description     Clear the database
 **
 ** Parameters      None
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_ble_client_db_clear(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_clear_by_bda
 **
 ** Description    Finds an element in the database and clears
 **
 ** Parameters      bda: the BD address of the element to clear from db
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_ble_client_db_clear_by_bda(BD_ADDR bda);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_alloc_element
 **
 ** Description     Allocate a new DB element
 **
 ** Parameters      None
 **
 ** Returns         Pointer to the new element found, NULL if error
 **
 *******************************************************************************/
tAPP_BLE_CLIENT_DB_ELEMENT *app_ble_client_db_alloc_element(void);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_free_element
 **
 ** Description     Free an element
 **
 ** Parameters      p_element: Element to free
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_ble_client_db_free_element(tAPP_BLE_CLIENT_DB_ELEMENT *p_element);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_free_attribute
 **
 ** Description     Free an element
 **
 ** Parameters      p_element: Element to free
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_ble_client_db_free_attribute(tAPP_BLE_CLIENT_DB_ATTR *p_attribute);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_add_element
 **
 ** Description     Add a database element to the database
 **
 ** Parameters      p_element: Element to add (allocated by app_hh_db_alloc_element)
 **
 ** Returns         None
 **
 *******************************************************************************/
void app_ble_client_db_add_element(tAPP_BLE_CLIENT_DB_ELEMENT *p_element);

/*******************************************************************************
 **
 ** Function        app_ble_db_find_by_bda
 **
 ** Description     Find an element in the database if it exists
 **
 ** Parameters      bda: the BD address of the element to look for
 **
 ** Returns         Pointer to the element found, NULL if not found
 **
 *******************************************************************************/
tAPP_BLE_CLIENT_DB_ELEMENT *app_ble_client_db_find_by_bda(BD_ADDR bda);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_find_by_handle
 **
 ** Description     Find an element in the database if it exists
 **
 ** Parameters      bda: the BD address of the element to look for
 **
 ** Returns         Pointer to the element found, NULL if not found
 **
 *******************************************************************************/
tAPP_BLE_CLIENT_DB_ATTR *app_ble_client_db_find_by_handle(tAPP_BLE_CLIENT_DB_ELEMENT *p_element,
     UINT16 handle);

/*******************************************************************************
 **
 ** Function        app_ble_client_db_alloc_attr
 **
 ** Description     Find an element in the database if it exists
 **
 ** Parameters      bda: the BD address of the element to look for
 **
 ** Returns         Pointer to the element found, NULL if not found
 **
 *******************************************************************************/
tAPP_BLE_CLIENT_DB_ATTR *app_ble_client_db_alloc_attr(tAPP_BLE_CLIENT_DB_ELEMENT *p_element);

#endif

/*****************************************************************************
 **
 **  Name:           app_ble_client_db.c
 **
 **  Description:    Bluetooth BLE Database utility functions
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <bsa_rokid/bsa_api.h>

#include <bsa_rokid/bsa_trace.h>
#include "app_utils.h"
//#include "ble_client.h"
#include "app_ble_client_db.h"
#include "app_ble_client_xml.h"


/* Structure to hold Database data */
typedef struct
{
    tAPP_BLE_CLIENT_DB_ELEMENT *first;
} tAPP_BLE_CLIENT_DB;


tAPP_BLE_CLIENT_DB app_ble_client_db;

/*
 * Local functions
 */

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
int app_ble_client_db_init(void)
{
    /* By default, clear the entire structure */
    memset(&app_ble_client_db, 0, sizeof(app_ble_client_db));

    /* Reload and save the database to clean it at startup */
    app_ble_client_db_reload();
    app_ble_client_db_save();

    return 0;
}

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
int app_ble_client_db_reload(void)
{
    /* Free up the current database */
    app_ble_client_db_clear();

    /* Reload the XML */
    return app_ble_client_xml_read(app_ble_client_db.first);
}

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
int app_ble_client_db_save(void)
{
    /* Save the XML */
    return app_ble_client_xml_write(app_ble_client_db.first);
}

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
void app_ble_client_db_clear(void)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_next, *p_element = app_ble_client_db.first;

    app_ble_client_db.first = NULL;

    while (p_element != NULL)
    {
        p_next = p_element->next;
        app_ble_client_db_free_element(p_element);
        p_element = p_next;
    }
}

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
void app_ble_client_db_clear_by_bda(BD_ADDR bda)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_element = app_ble_client_db.first;
    tAPP_BLE_CLIENT_DB_ELEMENT *p_last = NULL;

    p_element = app_ble_client_db.first;

    APP_INFO0("app_ble_client_db_clear_by_bda p_element");
    while(p_element != NULL)
    {
        if (bdcmp(bda, p_element->bd_addr) == 0)
        {
            break;
        }
        p_last = p_element;
        p_element = p_element->next;
    }

    if(p_element == NULL)
    {
        APP_INFO0("Device not found");
        return;
    }
    if (p_last == NULL)
    {
        APP_INFO0("Need to remove first item in DB");
        app_ble_client_db.first = p_element->next;
    }
    else
    {
        APP_INFO0("Delete the element");
        p_last->next = p_element->next;
    }

    app_ble_client_db_free_element(p_element);
}

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
tAPP_BLE_CLIENT_DB_ELEMENT *app_ble_client_db_alloc_element(void)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_element;

    p_element = calloc(1, sizeof(*p_element));

    /* Initialize element with default values */
    p_element->p_attr = NULL;
    p_element->app_handle = 0xffff;
    p_element->next = NULL;

    return p_element;
}


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
void app_ble_client_db_free_element(tAPP_BLE_CLIENT_DB_ELEMENT *p_element)
{
    tAPP_BLE_CLIENT_DB_ATTR *p_current, *p_next;

    if (p_element->p_attr != NULL)
    {
        p_current = p_element->p_attr;
        while (p_current != NULL)
        {
            p_next = p_current->next;
            app_ble_client_db_free_attribute(p_current);
            p_current = p_next;
        }
    }
    free(p_element);
}

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
void app_ble_client_db_free_attribute(tAPP_BLE_CLIENT_DB_ATTR *p_attribute)
{
    free(p_attribute);
}

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
void app_ble_client_db_add_element(tAPP_BLE_CLIENT_DB_ELEMENT *p_element)
{
    p_element->next = app_ble_client_db.first;
    app_ble_client_db.first = p_element;

    APP_INFO1("added ele on db BDA:%02X:%02X:%02X:%02X:%02X:%02X, client_if= %d ", 
        app_ble_client_db.first->bd_addr[0], app_ble_client_db.first->bd_addr[1],
        app_ble_client_db.first->bd_addr[2], app_ble_client_db.first->bd_addr[3],
        app_ble_client_db.first->bd_addr[4], app_ble_client_db.first->bd_addr[5],
        app_ble_client_db.first->app_handle);
}

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
tAPP_BLE_CLIENT_DB_ELEMENT *app_ble_client_db_find_by_bda(BD_ADDR bda)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_element;

    p_element = app_ble_client_db.first;

    while (p_element != NULL)
    {
        if (bdcmp(bda, p_element->bd_addr) == 0)
        {           
            return p_element;
        }
        p_element = p_element->next;
    }

    return NULL;
}

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
     UINT16 handle)
{
    tAPP_BLE_CLIENT_DB_ATTR *p_attribute;

    p_attribute = p_element->p_attr;
    while (p_attribute != NULL)
    {
        if (p_attribute->handle == handle)
        {
            return p_attribute;
        }
        p_attribute = p_attribute->next;
    }
    return NULL;
}

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
tAPP_BLE_CLIENT_DB_ATTR *app_ble_client_db_alloc_attr(tAPP_BLE_CLIENT_DB_ELEMENT *p_element)
{
    tAPP_BLE_CLIENT_DB_ATTR *p_attribute, *n_attribute, *prev_attr;

    p_attribute = p_element->p_attr;
    if(p_attribute == NULL)
    {
         p_attribute = malloc(sizeof(tAPP_BLE_CLIENT_DB_ATTR));
         p_attribute->next = NULL;
         p_element->p_attr = p_attribute;
         return p_attribute;
    }
    else
    {
        n_attribute = p_attribute->next;
        prev_attr = p_attribute;
        while (n_attribute != NULL)
        {
            prev_attr = prev_attr->next;
            n_attribute = n_attribute->next;
        }
        n_attribute = malloc(sizeof(tAPP_BLE_CLIENT_DB_ATTR));
        prev_attr->next = n_attribute;
        n_attribute->next = NULL;
        return n_attribute;
    }

    return NULL;
}



/*****************************************************************************
 **
 **  Name:           app_hh_db.c
 **
 **  Description:    This module contains utility functions to access HID
 **                  devices database
 **
 **  Copyright (c) 2011-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/
/* for free */
#include <stdlib.h>

#include "app_hh_db.h"
#include "app_hh_xml.h"
#include "app_utils.h"
#include <bsa_rokid/bd.h>
#include <bsa_rokid/bsa_hh_api.h>

typedef struct
{
    /* Pointer to the first element of the chained list of DB elements */
    tAPP_HH_DB_ELEMENT *first;
} tAPP_HH_DB;


/*******************************************************************************
 Globals
 *******************************************************************************/
tAPP_HH_DB app_hh_db;

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
int app_hh_db_init(void)
{
    /* By default, clear the entire structure */
    memset(&app_hh_db, 0, sizeof(app_hh_db));

    app_hh_xml_init();

    /* Reload and save the database to clean it at startup */
    app_hh_db_reload();
    app_hh_db_save();

    return 0;
}

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
void app_hh_db_exit(void)
{
    app_hh_db_clear();
}

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
void app_hh_db_clear(void)
{
    tAPP_HH_DB_ELEMENT *p_next, *p_element = app_hh_db.first;

    app_hh_db.first = NULL;

    while (p_element != NULL)
    {
        p_next = p_element->next;
        app_hh_db_free_element(p_element);
        p_element = p_next;
    }
}

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
int app_hh_db_reload(void)
{
    /* Free up the current database */
    app_hh_db_clear();

    /* Reload the XML */
    return app_hh_xml_read(&app_hh_db.first);
}

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
void app_hh_db_display(void)
{
    tAPP_HH_DB_ELEMENT *p_element = app_hh_db.first;

    APP_INFO0("HH database:");
    while (p_element != NULL)
    {
        APP_INFO1("  - device : %02X:%02X:%02X:%02X:%02X:%02X",
                p_element->bd_addr[0], p_element->bd_addr[1], p_element->bd_addr[2],
                p_element->bd_addr[3], p_element->bd_addr[4], p_element->bd_addr[5]);

        APP_INFO1("    + ssr_max_latency : %d", p_element->ssr_max_latency);
        APP_INFO1("    + ssr_min_tout : %d", p_element->ssr_min_tout);
        APP_INFO1("    + supervision_tout : %d", p_element->supervision_tout);
        APP_INFO1("    + brr : size=%d buf=%p", p_element->brr_size, p_element->brr);
        if (p_element->brr != NULL)
            APP_INFO1("    + brr : data = %02X:%02X...", (unsigned char)p_element->brr[0], (unsigned char)p_element->brr[1]);
        APP_INFO1("    + descriptor : size=%d buf=%p", p_element->descriptor_size, p_element->descriptor);
        if (p_element->descriptor != NULL)
            APP_INFO1("    + descriptor : data = %02X:%02X...", (unsigned char)p_element->descriptor[0],
                    (unsigned char)p_element->descriptor[1]);

        p_element = p_element->next;
    }
    APP_INFO0("HH database end");
}

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
int app_hh_db_save(void)
{
    /* Save the XML */
    return app_hh_xml_write(app_hh_db.first);
}

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
tAPP_HH_DB_ELEMENT *app_hh_db_find_by_bda(BD_ADDR bda)
{
    tAPP_HH_DB_ELEMENT *p_element;

    if (app_hh_db_reload() != 0)
    {
        APP_ERROR0("app_hh_db_reload failed");
        return NULL;
    }

    p_element = app_hh_db.first;

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
 ** Function        app_hh_db_remove_by_bda
 **
 ** Description     Remove element from the database
 **
 ** Parameters      bda: the BD address of the element to remove
 **
 ** Returns         status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_hh_db_remove_by_bda(BD_ADDR bda)
{
    tAPP_HH_DB_ELEMENT **pp_element, *p_element;

    if (app_hh_db_reload() != 0)
    {
        APP_ERROR0("app_hh_db_reload failed");
        return -1;
    }

    pp_element = &app_hh_db.first;

    while (*pp_element != NULL)
    {
        if (bdcmp(bda, (*pp_element)->bd_addr) == 0)
        {
            p_element = *pp_element;
            *pp_element = p_element->next;
            app_hh_db_free_element(p_element);
        }
        else
        {
            pp_element = &((*pp_element)->next);
        }
    }

    app_hh_db_save();
    return 0;
}

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
int app_hh_db_remove_db(void)
{
    tAPP_HH_DB_ELEMENT **pp_element, *p_element;

    if (app_hh_db_reload() != 0)
    {
        APP_ERROR0("app_hh_db_reload failed");
        return -1;
    }

    pp_element = &app_hh_db.first;

    while (*pp_element != NULL)
    {
        p_element = *pp_element;
        *pp_element = p_element->next;
        app_hh_db_free_element(p_element);
    }

    app_hh_db_save();
    return 0;
}

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
tAPP_HH_DB_ELEMENT *app_hh_db_alloc_element(void)
{
    tAPP_HH_DB_ELEMENT *p_element;

    p_element = calloc(1, sizeof(*p_element));

    /* Initialize element with default values */
    p_element->ssr_max_latency = BTA_HH_SSR_PARAM_INVALID;
    p_element->ssr_min_tout = BTA_HH_SSR_PARAM_INVALID;
    p_element->supervision_tout = BTA_HH_STO_PARAM_INVALID;

    return p_element;
}

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
void app_hh_db_free_element(tAPP_HH_DB_ELEMENT *p_element)
{
    if (p_element->brr != NULL)
    {
        free(p_element->brr);
    }
    if (p_element->descriptor != NULL)
    {
        free(p_element->descriptor);
    }
    free(p_element);
}

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
void app_hh_db_add_element(tAPP_HH_DB_ELEMENT *p_element)
{
    p_element->next = app_hh_db.first;
    app_hh_db.first = p_element;
}


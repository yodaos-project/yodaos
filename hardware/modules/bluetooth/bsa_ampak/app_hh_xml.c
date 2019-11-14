/*****************************************************************************
 **
 **  Name:           app_hh_xml.c
 **
 **  Description:    This module contains utility functions to access HID
 **                  devices saved parameters
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/
#include <fcntl.h>
#include <unistd.h>

#include "app_hh_xml.h"

#include "nanoxml.h"
#include "app_xml_utils.h"
#include "app_utils.h"

#ifndef tAPP_HH_XML_FILENAME
#define tAPP_HH_XML_FILENAME "/data/bluetooth/bt_hh_devices.xml"
#endif

#define APP_HH_XML_TAG_ROOT "Broadcom_Bluetooth_HID_Devices"
#define APP_HH_XML_TAG_DEVS "hh_devices"
#define APP_HH_XML_TAG_DEV "device"
#define APP_HH_XML_TAG_BDADDR "bd_addr"
#define APP_HH_XML_TAG_SSR_MAX_LATENCY "ssr_max_latency"
#define APP_HH_XML_TAG_SSR_MIN_TIMEOUT "ssr_min_timeout"
#define APP_HH_XML_TAG_SUPERVISION_TIMEOUT "supervision_timeout"
#define APP_HH_XML_TAG_BRR "brr"
#define APP_HH_XML_TAG_DESCRIPTOR "descriptor"
#define APP_HH_XML_TAG_SUB_CLASS "sub_class"
#define APP_HH_XML_TAG_ATTR_MASK "attribute_mask"

typedef enum
{
    HH_XML_TAG_DEVICE,
    HH_XML_TAG_BDADDR,
    HH_XML_TAG_SSR_MAX_LATENCY,
    HH_XML_TAG_SSR_MIN_TIMEOUT,
    HH_XML_TAG_SUPERVISION_TIMEOUT,
    HH_XML_TAG_BRR,
    HH_XML_TAG_DESCRIPTOR,
    HH_XML_TAG_SUB_CLASS,
    HH_XML_TAG_ATTR_MASK
} tAPP_HH_XML_TAG;

typedef struct
{
    /* Pointer to the current db element */
    tAPP_HH_DB_ELEMENT *p_elmt;
    /* Pointer to the next db element */
    tAPP_HH_DB_ELEMENT **pp_elmt;
    tAPP_HH_XML_TAG tag; /* Current tag */
} APP_HH_XML_CB;


static void app_hh_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_hh_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_hh_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);
static void app_hh_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);
static void app_hh_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);

/*
 * Global variables
 */
APP_HH_XML_CB app_hh_xml_cb;

/*******************************************************************************
 **
 ** Function        app_hh_xml_init
 **
 ** Description     Initialize XML parameter system
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_hh_xml_init(void)
{
    return 0;
}

/*
 * Code based on NANOXML library
 */

/*******************************************************************************
 **
 ** Function        app_hh_xml_read
 **
 ** Description     Load the MDL data base from  XML file
 **
 ** Parameters      p_app_cfg: Application Configuration buffer
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_hh_xml_read(tAPP_HH_DB_ELEMENT **pp_app_hh_db_element)
{
    nxml_t nxmlHandle;
    nxml_settings nxmlSettings;
    char *endp = NULL;
    int rc;
    int maxLen;
    char *file_buffer;
    int fd;
    int len;
    BOOLEAN create_empty = FALSE;

    /* Check that the head is NULL to prevent losing chain */
    if (*pp_app_hh_db_element != NULL)
    {
        APP_ERROR1("chained list is not empty (%p != NULL)", *pp_app_hh_db_element);
        return -1;
    }

    /* Open the Config file */
    if ((fd = open(tAPP_HH_XML_FILENAME, O_RDONLY)) >= 0)
    {
        /* Get the length of the device file */
        maxLen = app_file_size(fd);
        if (maxLen == 0)
        {
            APP_ERROR1("file: %s is empty", tAPP_HH_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }
        else if (maxLen < 0)
        {
            APP_ERROR1("cannot get file size of:%s", tAPP_HH_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
        {
            file_buffer = malloc(maxLen + 1);
            if (file_buffer == NULL)
            {
                APP_ERROR1("cannot alloc:%d bytes", maxLen);
                close(fd);
                create_empty = TRUE;
            }
        }

        if (create_empty == FALSE)
        {
            /* read the XML file */
            len = read(fd, (void *) file_buffer, maxLen);
            file_buffer[maxLen] = '\0';

            close(fd);

            if (len != maxLen)
            {
                free(file_buffer);
                APP_ERROR1("not able to read complete file:%d/%d",
                        len, maxLen);
                create_empty = TRUE;
            }
        }
    }
    else
    {
        create_empty = TRUE;
        APP_DEBUG1("cannot open:%s in read mode", tAPP_HH_XML_FILENAME);
    }

    /* If something has been read */
    if (create_empty == FALSE)
    {
        /* set callback function handlers */
        nxmlSettings.tag_begin = app_hh_xml_tagBeginCallbackFunc;
        nxmlSettings.tag_end = app_hh_xml_tagEndCallbackFunc;
        nxmlSettings.attribute_begin = app_hh_xml_attrBeginCallbackFunc;
        nxmlSettings.attribute_value = app_hh_xml_attrValueCallbackFunc;
        nxmlSettings.data = app_hh_xml_dataCallbackFunc;

        if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
        {
            free(file_buffer);
            APP_ERROR1("cannot open Nanoxml :%d", rc);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
        {
            app_hh_xml_cb.pp_elmt = pp_app_hh_db_element;

            /* Push our data into the nanoxml parser, it will then call our callback funcs */
            rc = xmlWrite(nxmlHandle, file_buffer, maxLen, &endp);
            if (rc != 1)
            {
                APP_ERROR1("xmlWrite returns :%d", rc);
                create_empty = TRUE;
            }

            xmlClose(nxmlHandle);
            free(file_buffer);
        }
    }

    /* in case of failure => create an empty database */
    if (create_empty)
    {
        APP_DEBUG0("Create an empty HL database");
        if (app_hh_xml_write(*pp_app_hh_db_element) < 0)
        {
            APP_ERROR0("Unable to create an empty HL database");
            return -1;
        }
    }

    return 0;
}


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
int app_hh_xml_write(tAPP_HH_DB_ELEMENT *p_app_hh_db_element)
{
    int fd;
    const tAPP_HH_DB_ELEMENT *p_hh_db_elmt = p_app_hh_db_element;

    if ((fd = open(tAPP_HH_XML_FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", tAPP_HH_XML_FILENAME);
        return -1;
    }

    app_xml_open_tag(fd, APP_HH_XML_TAG_ROOT, TRUE);
    app_xml_open_tag(fd, APP_HH_XML_TAG_DEVS, TRUE);

    /* For every Application in device database */
    while (p_hh_db_elmt != NULL)
    {
        app_xml_open_tag(fd, APP_HH_XML_TAG_DEV, TRUE);
        app_xml_open_tag(fd, APP_HH_XML_TAG_BDADDR, FALSE);
        app_xml_write_data(fd, p_hh_db_elmt->bd_addr, sizeof(p_hh_db_elmt->bd_addr), FALSE);
        app_xml_close_tag(fd, APP_HH_XML_TAG_BDADDR, FALSE);

        app_xml_open_close_tag_with_value(fd, APP_HH_XML_TAG_SSR_MAX_LATENCY,
                p_hh_db_elmt->ssr_max_latency);

        app_xml_open_close_tag_with_value(fd, APP_HH_XML_TAG_SSR_MIN_TIMEOUT,
                p_hh_db_elmt->ssr_min_tout);

        app_xml_open_close_tag_with_value(fd, APP_HH_XML_TAG_SUPERVISION_TIMEOUT,
                p_hh_db_elmt->supervision_tout);

        app_xml_open_tag(fd, APP_HH_XML_TAG_BRR, TRUE);
        app_xml_write_data(fd, p_hh_db_elmt->brr, p_hh_db_elmt->brr_size, TRUE);
        app_xml_close_tag(fd, APP_HH_XML_TAG_BRR, TRUE);

        app_xml_open_close_tag_with_value(fd, APP_HH_XML_TAG_SUB_CLASS,
                 p_hh_db_elmt->sub_class);

        app_xml_open_close_tag_with_value(fd, APP_HH_XML_TAG_ATTR_MASK,
                 p_hh_db_elmt->attr_mask);

        app_xml_open_tag(fd, APP_HH_XML_TAG_DESCRIPTOR, TRUE);
        app_xml_write_data(fd, p_hh_db_elmt->descriptor, p_hh_db_elmt->descriptor_size, TRUE);
        app_xml_close_tag(fd, APP_HH_XML_TAG_DESCRIPTOR, TRUE);

        app_xml_close_tag(fd, APP_HH_XML_TAG_DEV, TRUE);

        /* Move to next DB element */
        p_hh_db_elmt = p_hh_db_elmt->next;
    }

    app_xml_close_tag(fd, APP_HH_XML_TAG_DEVS, TRUE);
    app_xml_close_tag(fd, APP_HH_XML_TAG_ROOT, TRUE);

    close(fd);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_hh_xml_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hh_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    tAPP_HH_DB_ELEMENT *p_element;

    if (strncmp(tagName, APP_HH_XML_TAG_ROOT, strlen(APP_HH_XML_TAG_ROOT)) == 0)
    {
        /* nothing to do */
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_DEVS, strlen(APP_HH_XML_TAG_DEVS)) == 0)
    {
        /* nothing to do */
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_DEV, strlen(APP_HH_XML_TAG_DEV)) == 0)
    {
        /* Allocate a new element */
        p_element = app_hh_db_alloc_element();
        if (p_element == NULL)
        {
            APP_ERROR0("app_hh_db_alloc_element failed");
        }
        else
        {
            /* Append the new element */
            *app_hh_xml_cb.pp_elmt = p_element;
            app_hh_xml_cb.pp_elmt = &p_element->next;
            /* Save the current element */
            app_hh_xml_cb.p_elmt = p_element;
        }

        app_hh_xml_cb.tag = HH_XML_TAG_DEVICE;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_BDADDR, strlen(APP_HH_XML_TAG_BDADDR)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_BDADDR;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_SSR_MAX_LATENCY, strlen(APP_HH_XML_TAG_SSR_MAX_LATENCY)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_SSR_MAX_LATENCY;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_SSR_MIN_TIMEOUT, strlen(APP_HH_XML_TAG_SSR_MIN_TIMEOUT)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_SSR_MIN_TIMEOUT;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_SUPERVISION_TIMEOUT, strlen(APP_HH_XML_TAG_SUPERVISION_TIMEOUT)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_SUPERVISION_TIMEOUT;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_BRR, strlen(APP_HH_XML_TAG_BRR)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_BRR;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_DESCRIPTOR, strlen(APP_HH_XML_TAG_DESCRIPTOR)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_DESCRIPTOR;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_SUB_CLASS, strlen(APP_HH_XML_TAG_BRR)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_SUB_CLASS;
    }
    else if (strncmp(tagName, APP_HH_XML_TAG_ATTR_MASK, strlen(APP_HH_XML_TAG_BRR)) == 0)
    {
        app_hh_xml_cb.tag = HH_XML_TAG_ATTR_MASK;
    }
    else
    {
        APP_ERROR1("Unknown Tag:%s", tagName);
    }
}

/*******************************************************************************
 **
 ** Function        app_hh_xml_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hh_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_hh_xml_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hh_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
    /* in HH the only attribute supported is "value" */
    if (strncmp(attrName, "value", len) != 0)
    {
        APP_ERROR1("Unsupported attribute (%s)", attrName);
    }
}

/*******************************************************************************
 **
 ** Function        app_hh_xml_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hh_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete attribute value, probably malformed XML file: %s", attrValue);
    }

    switch (app_hh_xml_cb.tag)
    {
    case HH_XML_TAG_SSR_MAX_LATENCY:
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->ssr_max_latency = app_xml_read_value(attrValue, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_SSR_MAX_LATENCY while no device instance");
        }
        break;

    case HH_XML_TAG_SSR_MIN_TIMEOUT:
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->ssr_min_tout = app_xml_read_value(attrValue, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_SSR_MIN_TIMEOUT while no device instance");
        }
        break;

    case HH_XML_TAG_SUPERVISION_TIMEOUT:
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->supervision_tout = app_xml_read_value(attrValue, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_SUPERVISION_TIMEOUT while no device instance");
        }
        break;

    case HH_XML_TAG_SUB_CLASS:
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->sub_class = app_xml_read_value(attrValue, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_SUPERVISION_TIMEOUT while no device instance");
        }
        break;

    case HH_XML_TAG_ATTR_MASK:
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->attr_mask = app_xml_read_value(attrValue, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_SUPERVISION_TIMEOUT while no device instance");
        }
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_hh_xml_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_hh_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete data, probably malformed XML file: %s", data);
    }

    switch (app_hh_xml_cb.tag)
    {
    case HH_XML_TAG_BDADDR:
        /* Make sure that the current element was allocated correctly */
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_xml_read_data_length(app_hh_xml_cb.p_elmt->bd_addr,
                    sizeof(app_hh_xml_cb.p_elmt->bd_addr), data, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_BDADDR while no device instance");
        }
        break;

    case HH_XML_TAG_BRR:
        /* Make sure that the current element was allocated correctly */
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->brr = app_xml_read_data(&app_hh_xml_cb.p_elmt->brr_size,
                    data, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_BRR while no device instance");
        }
        break;

    case HH_XML_TAG_DESCRIPTOR:
        /* Make sure that the current element was allocated correctly */
        if (app_hh_xml_cb.p_elmt != NULL)
        {
            app_hh_xml_cb.p_elmt->descriptor = app_xml_read_data(&app_hh_xml_cb.p_elmt->descriptor_size,
                    data, len);
        }
        else
        {
            APP_ERROR0("HH_XML_TAG_DESCRIPTOR while no device instance");
        }
        break;

    default:
        break;
    }
}


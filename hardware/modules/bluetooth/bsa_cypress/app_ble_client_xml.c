/*****************************************************************************
 **
 **  Name:           app_ble_xml.c
 **
 **  Description:    This module contains utility functions to access BLE
 **                  saved parameters
 **
 **  Copyright (c) 2014, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <bsa_rokid/bsa_api.h>
#include <hardware/bt_log.h>
//#include "ble_client.h"
#include "app_ble_client_db.h"
#include "app_ble_client_xml.h"
#include "app_xml_utils.h"
#include "app_utils.h"

#include "nanoxml.h"

#ifndef tAPP_BLE_CLIENT_XML_FILENAME
#define tAPP_BLE_CLIENT_XML_FILENAME "/data/bluetooth/bt_ble_client_devices.xml"
#endif

#define APP_BLE_CLIENT_XML_TAG_ROOT "Broadcom_Bluetooth_BLE_Devices"
#define APP_BLE_CLIENT_XML_TAG_DEVS "ble_devices"
#define APP_BLE_CLIENT_XML_TAG_DEV "device"
#define APP_BLE_CLIENT_XML_TAG_BDADDR "bd_addr"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE "attribute"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE "handle"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE "type"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID "uuid"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN "len_uuid"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_ID "id"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP "prop"
#define APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY "IsPrimary"
#define APP_BLE_CLIENT_XML_UUID_16_CHARACTER_LEN    (LEN_UUID_16*2  + 2) /* plus 2 because 0x prefix in the cache file*/
#define APP_BLE_CLIENT_XML_UUID_32_CHARACTER_LEN    (LEN_UUID_32*2  + 2)
#define APP_BLE_CLIENT_XML_UUID_128_CHARACTER_LEN   (LEN_UUID_128*2 + 2)

typedef enum
{
    BLE_CLIENT_XML_TAG_DEVICE,
    BLE_CLIENT_XML_TAG_BDADDR,
    BLE_CLIENT_XML_TAG_ATTRIBUTE,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_ID,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP,
    BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY,
} tAPP_BLE_CLIENT_XML_TAG;

typedef struct
{
    tAPP_BLE_CLIENT_XML_TAG tag;           /* current tag */
    tAPP_BLE_CLIENT_DB_ELEMENT *p_app_cfg; /* Location to save the read database */
    tAPP_BLE_CLIENT_DB_ATTR *p_attr;       /* current attribute */
} APP_BLE_CLIENT_XML_CB;


/*
 * Global variables
 */
APP_BLE_CLIENT_XML_CB app_ble_client_xml_cb;

static void app_ble_client_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);

static void app_ble_client_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);

static void app_ble_client_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);

static void app_ble_client_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);

static void app_ble_client_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);
static UINT16 app_ble_client_xml_hex2int(char t);


/*******************************************************************************
 **
 ** Function        app_ble_client_xml_init
 **
 ** Description     Initialize XML parameter system (nothing for the moment)
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_ble_client_xml_init(void)
{
    return 0;
}

/*
 * Code based on NANOXML library
 */

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
int app_ble_client_xml_read(tAPP_BLE_CLIENT_DB_ELEMENT *p_element)
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

    /* Open the Config file */
    if ((fd = open(tAPP_BLE_CLIENT_XML_FILENAME, O_RDONLY)) >= 0)
    {
        /* Get the length of the device file */
        maxLen = app_file_size(fd);
        if (maxLen == 0)
        {
            APP_ERROR1("file: %s is empty", tAPP_BLE_CLIENT_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }
        else if (maxLen < 0)
        {
            APP_ERROR1("cannot get file size of: %s", tAPP_BLE_CLIENT_XML_FILENAME);
            close(fd);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
        {
            file_buffer = malloc(maxLen + 1);
            if (file_buffer == NULL)
            {
                APP_ERROR1("cannot alloc: %d bytes", maxLen);
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
        APP_DEBUG1("cannot open:%s in read mode", tAPP_BLE_CLIENT_XML_FILENAME);
    }

    /* If something has been read */
    if (create_empty == FALSE)
    {
        /* set callback function handlers */
        nxmlSettings.tag_begin = app_ble_client_xml_tagBeginCallbackFunc;
        nxmlSettings.tag_end = app_ble_client_xml_tagEndCallbackFunc;
        nxmlSettings.attribute_begin = app_ble_client_xml_attrBeginCallbackFunc;
        nxmlSettings.attribute_value = app_ble_client_xml_attrValueCallbackFunc;
        nxmlSettings.data = app_ble_client_xml_dataCallbackFunc;
        if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
        {
            free(file_buffer);
            APP_ERROR1("cannot open Nanoxml :%d", rc);
            create_empty = TRUE;
        }

        if (create_empty == FALSE)
        {
            app_ble_client_xml_cb.p_app_cfg = p_element;

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
        APP_DEBUG0("Create an empty BLE Client database");
        if (app_ble_client_xml_write(p_element) < 0)
        {
            APP_ERROR0("Unable to create an empty BLE Client database");
            return -1;
        }
    }

    return 0;
}


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
int app_ble_client_xml_write(tAPP_BLE_CLIENT_DB_ELEMENT *p_element)
{
    int fd;
    tAPP_BLE_CLIENT_DB_ELEMENT *element;
    tAPP_BLE_CLIENT_DB_ATTR *attr;
    int i;
    if ((fd = open(tAPP_BLE_CLIENT_XML_FILENAME, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", tAPP_BLE_CLIENT_XML_FILENAME);
        return -1;
    }

    app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ROOT, TRUE);
    app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_DEVS, TRUE);

    element = p_element;

    /* For every Application in device database */
    while (element != NULL)
    {
        if(element->bd_addr[0]|element->bd_addr[1]|element->bd_addr[2]|
           element->bd_addr[3]|element->bd_addr[4]|element->bd_addr[5])
        {
            app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_DEV, TRUE);

            BT_LOGD("save #2 BDA:%02X:%02X:%02X:%02X:%02X:%02X, client_if= %d ",
                  element->bd_addr[0], element->bd_addr[1],
                  element->bd_addr[2], element->bd_addr[3],
                  element->bd_addr[4], element->bd_addr[5],
                  element->app_handle);

            app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_BDADDR, FALSE);
            app_xml_write_data(fd, element->bd_addr, sizeof(element->bd_addr), FALSE);
            app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_BDADDR, FALSE);
            attr = element->p_attr;
            while(attr != NULL)
            {
                BT_LOGD("attr save handle:0x%x type:0x%x uuid:0x%04X, len:%d id:0x%x, prop:0x%x, primary:%d",
                  attr->handle, attr->attr_type, attr->attr_UUID.uu.uuid16,
                  attr->attr_UUID.len, attr->id, attr->prop, attr->is_primary);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE, TRUE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE, FALSE);
                dprintf(fd, "%d", (int) attr->handle);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE, FALSE);
                dprintf(fd, "%d", (int) attr->attr_type);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID, FALSE);
                if (attr->attr_UUID.len == LEN_UUID_16)
                    dprintf(fd, "0x%04X", (int) attr->attr_UUID.uu.uuid16);
                else if (attr->attr_UUID.len ==LEN_UUID_32)
                    dprintf(fd,"0x%08X",(int) attr->attr_UUID.uu.uuid32);
                else if (attr->attr_UUID.len == LEN_UUID_128)
                {
                     dprintf(fd, "0x%02X", (int) attr->attr_UUID.uu.uuid128[LEN_UUID_128-1]);
                     for (i=LEN_UUID_128-2 ; i >= 0;i--)
                         dprintf(fd, "%02X", (int) attr->attr_UUID.uu.uuid128[i]);
                }
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN, FALSE);
                dprintf(fd, "%d", (int) attr->attr_UUID.len);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_ID, FALSE);
                dprintf(fd, "%d", (int) attr->id);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_ID, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP, FALSE);
                dprintf(fd, "%d", (int) attr->prop);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP, FALSE);

                app_xml_open_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY, FALSE);
                dprintf(fd, "%d", (int) attr->is_primary);
                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY, FALSE);

                app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE, TRUE);

                attr = attr->next;
            }
            app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_DEV, TRUE);
        }
        /* Move to next DB element */
        element = element->next;
    }

    app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_DEVS, TRUE);
    app_xml_close_tag(fd, APP_BLE_CLIENT_XML_TAG_ROOT, TRUE);

    close(fd);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_ble_client_xml_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    tAPP_BLE_CLIENT_DB_ELEMENT *p_element;
    tAPP_BLE_CLIENT_DB_ATTR *p_attr;

    if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ROOT, strlen(APP_BLE_CLIENT_XML_TAG_ROOT)) == 0)
    {
        /* nothing to do */
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_DEVS, strlen(APP_BLE_CLIENT_XML_TAG_DEVS)) == 0)
    {
        /* nothing to do */
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_DEV, strlen(APP_BLE_CLIENT_XML_TAG_DEV)) == 0)
    {
        /* Allocate a new element */
        p_element = app_ble_client_db_alloc_element();
        if (p_element == NULL)
        {
            APP_ERROR0("app_ble_db_alloc_element failed");
        }
        else
        {
            app_ble_client_db_add_element(p_element);
            app_ble_client_xml_cb.p_app_cfg = p_element;
        }

        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_DEVICE;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_BDADDR, strlen(APP_BLE_CLIENT_XML_TAG_BDADDR)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_BDADDR;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE)) == 0)
    {
        /* Allocate a new attribute */
        p_attr = app_ble_client_db_alloc_attr(app_ble_client_xml_cb.p_app_cfg);
        if (p_attr == NULL)
        {
            APP_ERROR0("app_ble_db_alloc_attr failed");
        }
        else
        {
            app_ble_client_xml_cb.p_attr = p_attr;
        }

        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_ID, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_ID)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_ID;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP;
    }
    else if (strncmp(tagName, APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY, strlen(APP_BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY)) == 0)
    {
        app_ble_client_xml_cb.tag = BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY;
    }
    else
    {
        APP_ERROR1("Unknown Tag:%s", tagName);
    }
}

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_ble_client_xml_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{

}

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_ble_client_xml_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
    /* the only attribute supported is "value" */
    if (strncmp(attrName, "value", len) != 0)
    {
        APP_ERROR1("Unsupported attribute (%s)", attrName);
    }
}

/*******************************************************************************
 **
 ** Function        app_ble_client_xml_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_ble_client_xml_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete attribute value, probably malformed XML file: %s", attrValue);
    }

    switch (app_ble_client_xml_cb.tag)
    {

    default:
        break;
    }
}
/*******************************************************************************
**
** Function        app_ble_client_xml_hex2int
**
** Description     hex to int
**
** Returns         int
**
*****************************************************************************/

static UINT16 app_ble_client_xml_hex2int(char t)
{
    switch (t)
    {
        case '0':return 0;
        case '1':return 1;
        case '2':return 2;
        case '3':return 3;
        case '4':return 4;
        case '5':return 5;
        case '6':return 6;
        case '7':return 7;
        case '8':return 8;
        case '9':return 9;
        case 'A':return 10;
        case 'B':return 11;
        case 'C':return 12;
        case 'D':return 13;
        case 'E':return 14;
        case 'F':return 15;
        default :return 0;  /* should not go default */
    }
}
/*******************************************************************************
 **
 ** Function        app_ble_client_xml_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_ble_client_xml_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete data, probably malformed XML file: %s", data);
    }

    switch (app_ble_client_xml_cb.tag)
    {
    case BLE_CLIENT_XML_TAG_BDADDR:
        /* Make sure that the current element was allocated correctly */
        if (app_ble_client_xml_cb.p_app_cfg != NULL)
        {
            app_xml_read_data_length(app_ble_client_xml_cb.p_app_cfg->bd_addr,
                    sizeof(app_ble_client_xml_cb.p_app_cfg->bd_addr), data, len);
        }
        else
        {
            APP_ERROR0("BLE_CLIENT_XML_TAG_BDADDR while no device instance");
        }
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE:
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_HANDLE:
        app_ble_client_xml_cb.p_attr->handle = app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_TYPE:
        app_ble_client_xml_cb.p_attr->attr_type = app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID:
        if (len == APP_BLE_CLIENT_XML_UUID_128_CHARACTER_LEN)
        {
           char tmp[40];
           unsigned int i,j;
           app_xml_read_string(&tmp,sizeof(tmp),data,len);
           for (i=2,j=15;i<len;i+=2,j--)
               app_ble_client_xml_cb.p_attr->attr_UUID.uu.uuid128[j] = (app_ble_client_xml_hex2int(tmp[i])<<4)+app_ble_client_xml_hex2int( tmp[i+1] );
        }
        else if (len == APP_BLE_CLIENT_XML_UUID_16_CHARACTER_LEN)
           app_ble_client_xml_cb.p_attr->attr_UUID.uu.uuid16 = app_xml_read_value(data, len);
        else if (len == APP_BLE_CLIENT_XML_UUID_32_CHARACTER_LEN)
           app_ble_client_xml_cb.p_attr->attr_UUID.uu.uuid32 =(UINT32)app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_UUID_LEN:
        app_ble_client_xml_cb.p_attr->attr_UUID.len = app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_ID:
        app_ble_client_xml_cb.p_attr->id = app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_PROP:
        app_ble_client_xml_cb.p_attr->prop = app_xml_read_value(data, len);
        break;

    case BLE_CLIENT_XML_TAG_ATTRIBUTE_PRIMARY:
        app_ble_client_xml_cb.p_attr->is_primary = app_xml_read_value(data, len);
        break;

    default:
        break;
    }
}


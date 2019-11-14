/*****************************************************************************
 **
 **  Name:           app_xml_utils.c
 **
 **  Description:    Bluetooth XML utility functions
 **
 **  Copyright (c) 2010-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "app_xml_utils.h"
#include "app_utils.h"

/*
 * Definitions
 */
#define APP_XML_TAG_OPEN_PREFIX "<"
#define APP_XML_TAG_CLOSE_PREFIX "</"
#define APP_XML_TAG_SUFFIX ">"
#define APP_XML_CR "\n"
#define APP_XML_TAG_SUFFIX_CR APP_XML_TAG_SUFFIX APP_XML_CR
#define APP_XML_TAG_OPEN_CLOSE_SUFFIX "/" APP_XML_TAG_SUFFIX_CR

//#define APP_XML_CONFIG_FILE_PATH            "./bt_config.xml"
//#define APP_XML_REM_DEVICES_FILE_PATH       "./bt_devices.xml"
//#define APP_XML_SI_DEVICES_FILE_PATH        "./si_devices.xml"

/*
 * Global Variables
 */
tAPP_XML_REM_DEVICE app_xml_remote_devices_db[APP_MAX_NB_REMOTE_STORED_DEVICES];
tAPP_XML_SI_DEVICE app_xml_si_devices_db[APP_MAX_SI_DEVICES];

int treelevel = 0;


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
int app_read_xml_config(tAPP_XML_CONFIG *p_xml_config)
{
    int status;

    status = app_xml_read_cfg(APP_XML_CONFIG_FILE_PATH, p_xml_config);

    if (status < 0)
    {
        APP_ERROR1("app_xml_read_cfg failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG0("app_read_xml_config:");
        APP_DEBUG1("\tEnable:%d", p_xml_config->enable);
        APP_DEBUG1("\tDiscoverable:%d", p_xml_config->discoverable);
        APP_DEBUG1("\tConnectable:%d", p_xml_config->connectable);
        APP_DEBUG1("\tName:%s", p_xml_config->name);
        APP_DEBUG1 ("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                p_xml_config->bd_addr[0], p_xml_config->bd_addr[1],
                p_xml_config->bd_addr[2], p_xml_config->bd_addr[3],
                p_xml_config->bd_addr[4], p_xml_config->bd_addr[5]);
        APP_DEBUG1("\tClassOfDevice:%02x:%02x:%02x => %s", p_xml_config->class_of_device[0],
                p_xml_config->class_of_device[1], p_xml_config->class_of_device[2],
                app_get_cod_string(p_xml_config->class_of_device));
        APP_DEBUG1("\tRootPath:%s", p_xml_config->root_path);
        APP_DEBUG1("\tDefault PIN (%d):%s", p_xml_config->pin_len, p_xml_config->pin_code);
    }
    return 0;
}

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
int app_write_xml_config(const tAPP_XML_CONFIG *p_xml_config)
{
    int status;

    status = app_xml_write_cfg(APP_XML_CONFIG_FILE_PATH, p_xml_config);

    if (status < 0)
    {
        APP_ERROR1("app_xml_write_cfg failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("\tEnable:%d", p_xml_config->enable);
        APP_DEBUG1("\tDiscoverable:%d", p_xml_config->discoverable);
        APP_DEBUG1("\tConnectable:%d", p_xml_config->connectable);
        APP_DEBUG1("\tName:%s", p_xml_config->name);
        APP_DEBUG1 ("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
                p_xml_config->bd_addr[0], p_xml_config->bd_addr[1],
                p_xml_config->bd_addr[2], p_xml_config->bd_addr[3],
                p_xml_config->bd_addr[4], p_xml_config->bd_addr[5]);
        APP_DEBUG1("\tClassOfDevice:%02x:%02x:%02x", p_xml_config->class_of_device[0],
                p_xml_config->class_of_device[1], p_xml_config->class_of_device[2]);
        APP_DEBUG1("\tRootPath:%s", p_xml_config->root_path);
        APP_DEBUG1("\tDefault PIN (%d):%s", p_xml_config->pin_len, p_xml_config->pin_code);
    }
    return 0;
}

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
int app_read_xml_remote_devices(void)
{
    int status;

    memset(app_xml_remote_devices_db, 0, sizeof(app_xml_remote_devices_db));

    status = app_xml_read_db(APP_XML_REM_DEVICES_FILE_PATH, app_xml_remote_devices_db,
            APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_read_db failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("read(%s): OK", APP_XML_REM_DEVICES_FILE_PATH);
    }
    return 0;
}

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
int app_write_xml_remote_devices(void)
{
    int status;

    status = app_xml_write_db(APP_XML_REM_DEVICES_FILE_PATH,
            app_xml_remote_devices_db, APP_NUM_ELEMENTS(app_xml_remote_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_write_db failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("write(%s): OK", APP_XML_REM_DEVICES_FILE_PATH);
    }
    return 0;
}

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
int app_xml_is_device_index_valid(int index)
{
    if ((index >= 0) &&
        (index < APP_NUM_ELEMENTS(app_xml_remote_devices_db)) &&
        (app_xml_remote_devices_db[index].in_use != FALSE))
    {
        return 1;
    }
    return 0;
}

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
int app_read_xml_si_devices(void)
{
    int status;

    memset(app_xml_si_devices_db, 0, sizeof(app_xml_si_devices_db));

    status = app_xml_read_si_db(APP_XML_SI_DEVICES_FILE_PATH, app_xml_si_devices_db,
            APP_NUM_ELEMENTS(app_xml_si_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_read_si_db failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("read(%s): OK", APP_XML_SI_DEVICES_FILE_PATH);
    }
    return 0;
}

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
int app_write_xml_si_devices(void)
{
    int status;

    status = app_xml_write_si_db(APP_XML_SI_DEVICES_FILE_PATH,
            app_xml_si_devices_db, APP_NUM_ELEMENTS(app_xml_si_devices_db));

    if (status < 0)
    {
        APP_ERROR1("app_xml_write_si_db failed: %d", status);
        return -1;
    }
    else
    {
        APP_DEBUG1("write(%s): OK", APP_XML_SI_DEVICES_FILE_PATH);
    }
    return 0;
}

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
int app_xml_open_tag(int fd, const char *tag, BOOLEAN cr)
{
    int i, dummy;

    /* Indentation */
    for (i = 0; i < treelevel; i++)
    {
        dummy = write(fd, "  ", 2);
        (void)dummy;
    }

    /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
    dprintf(fd, APP_XML_TAG_OPEN_PREFIX);
    dummy = write(fd, tag, strlen(tag));
    (void)dummy;
    dprintf(fd, APP_XML_TAG_SUFFIX);
    if (cr == TRUE)
    {
        dprintf(fd, APP_XML_CR);
    }

    /* Increase indentation level */
    treelevel++;

    return 0;
}

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
int app_xml_close_tag(int fd, const char *tag, BOOLEAN cr)
{
    int i, dummy;

    /* Decrease indentation level */
    treelevel--;

    /* Indentation (if at the beginning of the line) */
    if (cr)
    {
        for (i = 0; i < treelevel; i++)
        {
            dummy = write(fd, "  ", 2);
            (void)dummy;
        }
    }

    /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
    dprintf(fd, APP_XML_TAG_CLOSE_PREFIX);
    dummy = write(fd, tag, strlen(tag));
    (void)dummy;
    dprintf(fd, APP_XML_TAG_SUFFIX_CR);

    return 0;
}

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
int app_xml_open_tag_with_value(int fd, char *tag, int value)
{
    int i, dummy;

    /* Indentation */
    for (i = 0; i < treelevel; i++)
    {
        dummy = write(fd, "  ", 2);
        (void)dummy;
    }

    /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
    dprintf(fd, APP_XML_TAG_OPEN_PREFIX);
    dprintf(fd, "%s value = \"%d\"", tag, value);
    dprintf(fd, APP_XML_TAG_SUFFIX_CR);

    /* Increment indentation level */
    treelevel++;

    return 0;
}

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
int app_xml_open_close_tag_with_value(int fd, char *tag, int value)
{
    int i, dummy;

    /* Indentation */
    for (i = 0; i < treelevel; i++)
    {
        dummy = write(fd, "  ", 2);
        (void)dummy;
    }

    /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
    dprintf(fd, APP_XML_TAG_OPEN_PREFIX);
    dprintf(fd, "%s value = \"%d\"", tag, value);
    dprintf(fd, APP_XML_TAG_OPEN_CLOSE_SUFFIX);

    return 0;
}

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
int app_xml_write_data(int fd, const void *p_buf, unsigned int length, BOOLEAN cr)
{
    int dummy;
    unsigned int index;
    const unsigned char *p_data = p_buf;

    for (index = 0; index < length; index++)
    {
        /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
        dprintf(fd, "%02X", p_data[index]);
        /* Add a CR every 32 byte values for readability */
        if (((index % 32) == 31))
        {
            dummy = write(fd, "\n", 1);
            (void)dummy;
        }
        else
        {
            /* Add colon between every bytes, but not for after the last byte */
            if (index != (length - 1))
            {
                dummy = write(fd, ":", 1);
                (void)dummy;
            }
        }
    }

    /* Add a CR if requested and data length not already multiple of 32 bytes */
    if (cr && ((length % 32) != 0))
    {
        dummy = write(fd, "\n", 1);
        (void)dummy;
    }
    return 0;
}

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
char *app_xml_read_data(unsigned int *p_datalen, const char *p_buf, unsigned int length)
{
    char *p_newbuf, *p_databuf, *p_data;
    int valid, allocsize = 1024;
    unsigned int index;

    /* By default length is 0 */
    *p_datalen = 0;

    p_databuf = malloc(allocsize);
    if (p_databuf == NULL)
    {
        APP_ERROR0("Failed allocating buffer");
        return NULL;
    }

    p_data = p_databuf;

    /* Initialize the current data value */
    valid = 0;
    *p_data = 0;

    for (index = 0; index < length; index++)
    {
        /* Check if this a separator */
        if ((p_buf[index] == ':') ||
            (valid && ((p_buf[index] == '\n') || isspace((unsigned)p_buf[index]))))
        {
            p_data++;
            /* Check if the current buffer is full -> realloc new one */
            if ((p_data - p_databuf) == allocsize)
            {
                /* increase the allocation size */
                allocsize += 1024;
                p_newbuf = realloc(p_databuf, allocsize);
                if (p_newbuf == NULL)
                {
                    APP_ERROR0("Failed reallocating buffer");
                    free(p_databuf);
                    return NULL;
                }
                /* Update the current data pointer */
                p_data = &p_newbuf[p_data - p_databuf];

                /* Save the current buffer */
                p_databuf = p_newbuf;
            }
            /* Initialize the current data value */
            valid = 0;
            *p_data = 0;
        }
        else if (isxdigit((unsigned)p_buf[index]))
        {
            /* Update the data value */
            *p_data *= 16;
            if (p_buf[index] <= '9')
                *p_data += p_buf[index]-'0';
            else if (p_buf[index] <= 'F')
                *p_data += p_buf[index]-'A' + 10;
            else
                *p_data += p_buf[index]-'a' + 10;
            /* Mark data valid */
            valid = 1;
        }
        else if (isspace((unsigned)p_buf[index]))
        {
            /* Not an error */
        }
        else
        {
            APP_ERROR1("Unsupported character in data : x%x", p_buf[index]);
        }
    }

    /* Check if there was a trailing decoded byte */
    if (valid)
    {
        p_data++;
    }

    /* Compute the total data length */
    *p_datalen = p_data - p_databuf;

    /* If length is null, free up the memory */
    if (*p_datalen == 0)
    {
        free(p_databuf);
        return NULL;
    }

    /* Resize the buffer to match the exact data len */
    p_newbuf = realloc(p_databuf, *p_datalen);
    if (p_newbuf == NULL)
    {
        APP_ERROR0("Failed resizing buffer");
        *p_datalen = 0;
        free(p_databuf);
        return NULL;
    }
    return p_newbuf;
}

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
BOOLEAN app_xml_read_data_length(void *p_data, unsigned int datalen, const char *p_buf, unsigned int length)
{
    char *p_newbuf;
    unsigned int newlen;
    BOOLEAN result = FALSE;

    /* Parse the buffer */
    p_newbuf = app_xml_read_data(&newlen, p_buf, length);

    /* Check if the parsed length is identical to the expected one */
    if (newlen == datalen)
    {
        memcpy(p_data, p_newbuf, datalen);
        result = TRUE;
    }
    free(p_newbuf);

    return result;
}

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
int app_xml_read_string(void *p_string, int stringlen, const char *p_buf, int length)
{
    char *p_data = p_string;
    int minlen = ((stringlen-1) < length)?(stringlen-1):length;

    /* Copy the string */
    memcpy(p_data, p_buf, minlen);

    /* Add a C string end */
    p_data[minlen] = 0;

    return 0;
}

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
int app_xml_read_value(const char *p_buf, int length)
{
    int value;
    char *p_end;

    value = strtoul(p_buf, &p_end, 0);

    /* Check the whole data was eaten up */
    if ((p_end - p_buf) != length)
    {
        APP_ERROR1("Value parsed did not use all buffer (%s)", p_buf);
    }
    return value;
}

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
int app_xml_get_cod_from_bd(BD_ADDR bd_addr, DEV_CLASS* p_cod)
{
    int index;

    for (index = 0; index < APP_MAX_NB_REMOTE_STORED_DEVICES; index++)
    {
        if ((app_xml_remote_devices_db[index].in_use == TRUE) &&
            (bdcmp(bd_addr,app_xml_remote_devices_db[index].bd_addr) == 0))
        {
            memcpy(p_cod,app_xml_remote_devices_db[index].class_of_device,sizeof(DEV_CLASS));
            return 0;
        }
    }

    return -1;
}

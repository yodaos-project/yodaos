/*****************************************************************************
 **
 **  Name:           app_xml_param.c
 **
 **  Description:    This module contains utility functions to access bluetooth
 **                  parameters (configuration and remote devices) in XML mode.
 **
 **  Copyright (c) 2009-2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.

 *****************************************************************************/
#ifdef QT_APP
#include "buildcfg.h"
#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "app_xml_param.h"

#include "app_xml_utils.h"
#include "app_utils.h"

#include "nanoxml.h"

#define APP_XML_ROOT_KEY                "X_BROADCOM_COM_BluetoothAdapter"

#define APP_XML_CONF_KEY                "bt_config"
#define APP_XML_CONF_ENABLE_KEY         "enable"
#define APP_XML_CONF_DISCOVERABLE_KEY   "visible"
#define APP_XML_CONF_CONNECTABLE_KEY    "connectable"
#define APP_XML_CONF_NAME_KEY           "local_name"
#define APP_XML_CONF_BDADDR_KEY         "bd_addr"
#define APP_XML_CONF_CLASS_KEY          "class_of_device"
#define APP_XML_CONF_PINCODE_KEY        "pin_code"
#define APP_XML_CONF_PINLEN_KEY         "pin_len"
#define APP_XML_CONF_ROOTPATH_KEY       "root_path"
#define APP_XML_CONF_IO_CAP_KEY         "io_capabilities"

#define APP_XML_DEVS_KEY                "bt_devices"
#define APP_XML_DEV_KEY                 "device"
#define APP_XML_DEV_INST_KEY            "instance"
#define APP_XML_DEV_BDADDR_KEY          "bd_addr"
#define APP_XML_DEV_NAME_KEY            "device_name"
#define APP_XML_DEV_CLASS_KEY           "class_of_device"
#define APP_XML_DEV_LINKKEY_KEY         "link_key"
#define APP_XML_DEV_LINKKEYP_KEY        "Link_key_present"
#define APP_XML_DEV_KEYTYPE_KEY         "key_type"
#define APP_XML_DEV_TRUSTED_KEY         "trusted_services"
#define APP_XML_DEV_PID_KEY             "pid"
#define APP_XML_DEV_VID_KEY             "vid"
#define APP_XML_DEV_VERSION_KEY         "version"
#define APP_XML_DEV_FEATURES            "features"
#define APP_XML_DEV_LMP_VERSION         "lmp_version"

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
#define APP_XML_DEV_BLE_ADDR_TYPE       "ble_addr_type"
#define APP_XML_DEV_DEVICE_TYPE         "device_type"
#define APP_XML_DEV_INQ_RESULT_TYPE     "inq_result_type"
#define APP_XML_DEV_BLE_LINKKEYP_KEY    "ble_Link_key_present"
#define APP_XML_DEV_PENC_LTK            "penc_ltk"
#define APP_XML_DEV_PENC_RAND           "penc_rand"
#define APP_XML_DEV_PENC_EDIV           "penc_ediv"
#define APP_XML_DEV_PENC_SEC_LEVEL      "penc_sec_level"
#define APP_XML_DEV_PENC_KEY_SIZE       "penc_key_size"
#define APP_XML_DEV_PID_IRK             "prid_irk"
#define APP_XML_DEV_PID_ADDR_TYPE       "prid_addr_type"
#define APP_XML_DEV_PID_STATIC_ADDR     "prid_static_addr"
#define APP_XML_DEV_PCSRK_COUNTER       "pcsrk_counter"
#define APP_XML_DEV_PCSRK_CSRK          "pcsrk_csrk"
#define APP_XML_DEV_PCSRK_SEC_LEVEL     "pcsrk_sec_level"
#define APP_XML_DEV_LCSRK_COUNTER       "lcsrk_counter"
#define APP_XML_DEV_LCSRK_DIV           "lcsrk_dis"
#define APP_XML_DEV_LCSRK_SEC_LEVEL     "lcsrk_sec_level"
#define APP_XML_DEV_LENC_DIV            "lenc_div"
#define APP_XML_DEV_LENC_KEY_SIZE       "lenc_key_size"
#define APP_XML_DEV_LENC_SEC_LEVEL      "lenc_sec_level"
#endif

#define APP_XML_SI_DEVS_KEY             "si_devices"
#define APP_XML_SI_DEV_KEY              "device"
#define APP_XML_SI_DEV_INST_KEY         "instance"
#define APP_XML_SI_DEV_BDADDR_KEY       "bd_addr"
#define APP_XML_SI_DEV_PLATFORM_KEY     "platform"

#define APP_XML_LINK_ROOT_KEY           "rokid_link_status"
#define APP_XML_LINK_KEY                "link"

enum
{
    /* Configuration states */
    CONF_ENABLE_KEY,
    CONF_DISCOVERABLE_KEY,
    CONF_CONNECTABLE_KEY,
    CONF_NAME_KEY,
    CONF_CLASS_KEY,
    CONF_BDADDR_KEY,
    CONF_PINCODE_KEY,
    CONF_PINLEN_KEY,
    CONF_IOCAP_KEY,
    CONF_ROOTPATH_KEY,

    /* Devices database states */
    DEVS_KEY,
    DEV_KEY,
    DEV_INST_KEY,
    DEV_BDADDR_KEY,
    DEV_NAME_KEY,
    DEV_CLASS_KEY,
    DEV_LINKKEY_KEY,
    DEV_LINKKEYP_KEY,
    DEV_KEYTYPE_KEY,
    DEV_TRUSTED_KEY,
    DEV_PID_KEY,
    DEV_VID_KEY,
    DEV_VERSION_KEY,
    DEV_FEATURES_KEY,
    DEV_LMP_VERSION_KEY,
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    DEV_BLE_ADDR_TYPE_KEY,
    DEV_DEVICE_TYPE_KEY,
    DEV_INQ_RESULT_TYPE,
    DEV_BLE_LINKKEYP_KEY,
    DEV_PENC_LTK_KEY,
    DEV_PENC_RAND_KEY,
    DEV_PENC_EDIV_KEY,
    DEV_PENC_SEC_LEVEL_KEY,
    DEV_PENC_KEY_SIZE_KEY,
    DEV_PID_IRK_KEY,
    DEV_PID_ADDR_TYPE_KEY,
    DEV_PID_STATIC_ADDR_KEY,
    DEV_PCSRK_COUNTER_KEY,
    DEV_PCSRK_CSRK_KEY,
    DEV_PCSRK_SEC_LEVEL_KEY,
    DEV_LCSRK_COUNTER_KEY,
    DEV_LCSRK_DIV_KEY,
    DEV_LCSRK_SEC_LEVEL_KEY,
    DEV_LENC_DIV_KEY,
    DEV_LENC_KEY_SIZE_KEY,
    DEV_LENC_SEC_LEVEL_KEY,
#endif

    /* Special interest device database states */
    SI_DEVS_KEY,
    SI_DEV_KEY,
    SI_DEV_INST_KEY,
    SI_DEV_BDADDR_KEY,
    SI_DEV_PLATFORM_KEY,
};

typedef struct
{
    int cfg_tag;
    int dev_tag;
    int si_dev_tag;
    int dev_instance;
    int si_dev_instance;
    tAPP_XML_CONFIG *config_ptr;
    tAPP_XML_REM_DEVICE *devices_db_ptr;
    tAPP_XML_SI_DEVICE *si_dev_db_ptr;
    int devices_max;
    int si_dev_max;
} tAPP_XML_PARAM_CB;

tAPP_XML_PARAM_CB app_xml_param_cb;

static void app_xml_conf_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_xml_conf_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);
static void app_xml_conf_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);
static void app_xml_conf_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);
static void app_xml_conf_tagEndCallbackFunc(nxml_t handle, const char *tagName,
        unsigned len);

static void app_xml_dev_db_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_xml_dev_db_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);
static void app_xml_dev_db_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);
static void app_xml_dev_db_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);
static void app_xml_dev_db_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);

static void app_xml_si_dev_db_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);
static void app_xml_si_dev_db_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len);
static void app_xml_si_dev_db_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more);
static void app_xml_si_dev_db_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more);
static void app_xml_si_dev_db_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len);


/*******************************************************************************
 **
 ** Function        app_xml_init
 **
 ** Description     Initialize XML parameter system
 **
 ** Returns         0 if successful, otherwise the error number
 **
 *******************************************************************************/
int app_xml_init(void)
{
    return 0;
}

/*
 * Code based on NANOXML lib
 */

/*******************************************************************************
 **
 ** Function        app_xml_read_cfg
 **
 ** Description     Read the Bluetooth configuration from XML file
 **
 ** Returns         Number of read bytes or 0 if read has failed.
 **
 *******************************************************************************/
int app_xml_read_cfg(const char *p_fname, tAPP_XML_CONFIG *p_xml_config)
{
    nxml_t nxmlHandle;
    nxml_settings nxmlSettings;
    char *endp = NULL;
    int rc;
    int maxLen;
    char *file_buffer;
    int fd;
    int len;

    /* Open the Config file */
    if ((fd = open(p_fname, O_RDONLY)) >= 0)
    {
        /* Get the length of the config file */
        maxLen = app_file_size(fd);
        if (maxLen <= 0)
        {
            close(fd);
            return -1;
        }

        file_buffer = malloc(maxLen + 1);
        if (file_buffer == NULL)
        {
            APP_ERROR1("cannot alloc:%d bytes", maxLen);
            close(fd);
            return -1;
        }

        /* read the binary file and write it directly in the config structure */
        len = read(fd, (void *) file_buffer, maxLen);
        file_buffer[maxLen] = '\0';
        close(fd);

        if (len != maxLen)
        {
            free(file_buffer);
            APP_ERROR1("not able to read complete file:%d/%d", len, maxLen);
            return -1;
        }
    }
    else
    {
        return -1;
    }

    /* set callback function handlers */
    nxmlSettings.tag_begin = app_xml_conf_tagBeginCallbackFunc;
    nxmlSettings.attribute_begin = app_xml_conf_attrBeginCallbackFunc;
    nxmlSettings.attribute_value = app_xml_conf_attrValueCallbackFunc;
    nxmlSettings.data = app_xml_conf_dataCallbackFunc;
    nxmlSettings.tag_end = app_xml_conf_tagEndCallbackFunc;

    if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
    {
        free(file_buffer);
        APP_ERROR1("cannot xml :%d", rc);
        return -1;
    }

    app_xml_param_cb.config_ptr = p_xml_config;

    /* Push our data into the nanoxml parser, it will then call our callback funcs */
    rc = xmlWrite(nxmlHandle, file_buffer, maxLen, &endp);
    if (rc != 1)
    {
        free(file_buffer);
        APP_ERROR1("xmlWrite returns :%d", rc);
        return -1;
    }

    free(file_buffer);
    xmlClose(nxmlHandle);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_read_db
 **
 ** Description     Read the remote devices data base from XML file
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_xml_read_db(const char *p_fname, tAPP_XML_REM_DEVICE *p_xml_rem_devices,
        int devices_max)
{
    nxml_t nxmlHandle;
    nxml_settings nxmlSettings;
    char *endp = NULL;
    int rc;
    int maxLen;
    char *file_buffer;
    int fd;
    int len;

    /* Open the config file */
    if ((fd = open(p_fname, O_RDONLY)) >= 0)
    {
        /* Get the length of the device file */
        maxLen = app_file_size(fd);
        if (maxLen <= 0)
        {
            APP_ERROR1("app_fs_file_size(%s) failed", p_fname);
            close(fd);
            return -1;
        }

        file_buffer = malloc(maxLen + 1);
        if (file_buffer == NULL)
        {
            APP_ERROR1("cannot alloc:%d bytes", maxLen);
            close(fd);
            return -1;
        }

        /* read the binary file and write it directly in the config structure */
        len = read(fd, (void *) file_buffer, maxLen);
        file_buffer[maxLen] = '\0';

        close(fd);

        if (len != maxLen)
        {
            free(file_buffer);
            APP_ERROR1("not able to read complete file: %d/%d", len, maxLen);
            return -1;
        }
    }
    else
    {
        APP_ERROR1("open(%s) failed", p_fname);
        return -1;
    }

    /* set callback function handlers */
    nxmlSettings.tag_begin = app_xml_dev_db_tagBeginCallbackFunc;
    nxmlSettings.attribute_begin = app_xml_dev_db_attrBeginCallbackFunc;
    nxmlSettings.attribute_value = app_xml_dev_db_attrValueCallbackFunc;
    nxmlSettings.data = app_xml_dev_db_dataCallbackFunc;
    nxmlSettings.tag_end = app_xml_dev_db_tagEndCallbackFunc;

    if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
    {
        free(file_buffer);
        APP_ERROR1("xmlOpen failed: %d", rc);
        return -1;
    }

    app_xml_param_cb.devices_db_ptr = p_xml_rem_devices;
    app_xml_param_cb.devices_max = devices_max;

    /* Push our data into the nanoxml parser, it will then call our callback funcs */
    rc = xmlWrite(nxmlHandle, file_buffer, maxLen, &endp);
    if (rc != 1)
    {
        free(file_buffer);
        APP_ERROR1("xmlWrite failed: %d", rc);
        return -1;
    }

    xmlClose(nxmlHandle);
    free(file_buffer);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_read_si_db
 **
 ** Description     Read the special interest device data base from XML file
 **
 ** Returns         0 if ok, otherwise -1
 **
 *******************************************************************************/
int app_xml_read_si_db(const char *p_fname, tAPP_XML_SI_DEVICE *p_xml_si_devices,
        int si_dev_max)
{
    nxml_t nxmlHandle;
    nxml_settings nxmlSettings;
    char *endp = NULL;
    int rc;
    int maxLen;
    char *file_buffer;
    int fd;
    int len;

    /* Open the config file */
    if ((fd = open(p_fname, O_RDONLY)) >= 0)
    {
        /* Get the length of the device file */
        maxLen = app_file_size(fd);
        if (maxLen <= 0)
        {
            APP_ERROR1("app_fs_file_size(%s) failed", p_fname);
            close(fd);
            return -1;
        }

        file_buffer = malloc(maxLen + 1);
        if (file_buffer == NULL)
        {
            APP_ERROR1("cannot alloc:%d bytes", maxLen);
            close(fd);
            return -1;
        }

        /* read the binary file and write it directly in the config structure */
        len = read(fd, (void *) file_buffer, maxLen);
        file_buffer[maxLen] = '\0';

        close(fd);

        if (len != maxLen)
        {
            free(file_buffer);
            APP_ERROR1("not able to read complete file: %d/%d", len, maxLen);
            return -1;
        }
    }
    else
    {
        APP_ERROR1("open(%s) failed", p_fname);
        return -1;
    }

    /* set callback function handlers */
    nxmlSettings.tag_begin = app_xml_si_dev_db_tagBeginCallbackFunc;
    nxmlSettings.attribute_begin = app_xml_si_dev_db_attrBeginCallbackFunc;
    nxmlSettings.attribute_value = app_xml_si_dev_db_attrValueCallbackFunc;
    nxmlSettings.data = app_xml_si_dev_db_dataCallbackFunc;
    nxmlSettings.tag_end = app_xml_si_dev_db_tagEndCallbackFunc;

    if ((rc = xmlOpen(&nxmlHandle, &nxmlSettings)) == 0)
    {
        free(file_buffer);
        APP_ERROR1("xmlOpen failed: %d", rc);
        return -1;
    }

    app_xml_param_cb.si_dev_db_ptr = p_xml_si_devices;
    app_xml_param_cb.si_dev_max = si_dev_max;

    /* Push our data into the nanoxml parser, it will then call our callback funcs */
    rc = xmlWrite(nxmlHandle, file_buffer, maxLen, &endp);
    if (rc != 1)
    {
        free(file_buffer);
        APP_ERROR1("xmlWrite failed: %d", rc);
        return -1;
    }

    xmlClose(nxmlHandle);
    free(file_buffer);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_write_cfg
 **
 ** Description     Writes the cfg file in file system or XML, creates one if it
 **                 doesn't exist
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_write_cfg(const char *p_fname, const tAPP_XML_CONFIG *p_xml_config)
{
    int fd, dummy;

    if ((fd = open((char *) p_fname, O_RDWR | O_CREAT, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", p_fname);
        return -1;
    }

    app_xml_open_tag(fd, APP_XML_ROOT_KEY, TRUE);
    app_xml_open_tag(fd, APP_XML_CONF_KEY, TRUE);

    app_xml_open_tag(fd, APP_XML_CONF_ENABLE_KEY, FALSE);
    /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
    dprintf(fd, "%d", p_xml_config->enable);
    app_xml_close_tag(fd, APP_XML_CONF_ENABLE_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_DISCOVERABLE_KEY, FALSE);
    dprintf(fd, "%d", p_xml_config->discoverable);
    app_xml_close_tag(fd, APP_XML_CONF_DISCOVERABLE_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_CONNECTABLE_KEY, FALSE);
    dprintf(fd, "%d", p_xml_config->connectable);
    app_xml_close_tag(fd, APP_XML_CONF_CONNECTABLE_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_NAME_KEY, FALSE);
    dummy = write(fd, (char *) p_xml_config->name, strlen((char *) p_xml_config->name));
    (void)dummy;
    app_xml_close_tag(fd, APP_XML_CONF_NAME_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_BDADDR_KEY, FALSE);
    app_xml_write_data(fd, p_xml_config->bd_addr, sizeof(p_xml_config->bd_addr), FALSE);
    app_xml_close_tag(fd, APP_XML_CONF_BDADDR_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_CLASS_KEY, FALSE);
    app_xml_write_data(fd, p_xml_config->class_of_device, sizeof(p_xml_config->class_of_device), FALSE);
    app_xml_close_tag(fd, APP_XML_CONF_CLASS_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_PINCODE_KEY, FALSE);
    dummy = write(fd, p_xml_config->pin_code, strlen((char *)p_xml_config->pin_code));
    (void)dummy;
    app_xml_close_tag(fd, APP_XML_CONF_PINCODE_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_PINLEN_KEY, FALSE);
    dprintf(fd, "%d", p_xml_config->pin_len);
    app_xml_close_tag(fd, APP_XML_CONF_PINLEN_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_IO_CAP_KEY, FALSE);
    dprintf(fd, "%d", p_xml_config->io_cap);
    app_xml_close_tag(fd, APP_XML_CONF_IO_CAP_KEY, FALSE);

    app_xml_open_tag(fd, APP_XML_CONF_ROOTPATH_KEY, FALSE);
    dummy = write(fd, p_xml_config->root_path, strlen((char *)p_xml_config->root_path));
    (void)dummy;
    app_xml_close_tag(fd, APP_XML_CONF_ROOTPATH_KEY, FALSE);

    app_xml_close_tag(fd, APP_XML_CONF_KEY, TRUE);
    app_xml_close_tag(fd, APP_XML_ROOT_KEY, TRUE);

    fsync(fd);
    close(fd);

    return 0;
}

int app_xml_write_link_status(const char *p_fname, BOOLEAN link)
{
    int fd, dummy;

    if ((fd = open((char *)p_fname, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", p_fname);
        return -1;
    }

    app_xml_open_tag(fd, APP_XML_LINK_ROOT_KEY, TRUE);

            app_xml_open_tag(fd, APP_XML_LINK_KEY, FALSE);
            dprintf(fd, "%d", link);
            app_xml_close_tag(fd, APP_XML_LINK_KEY, FALSE);

    app_xml_close_tag(fd, APP_XML_LINK_ROOT_KEY, TRUE);

    fsync(fd);
    close(fd);

    (void)dummy;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_write_db
 **
 ** Description     Writes the database file in file system or XML, creates one if it doesn't exist
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_write_db(const char *p_fname, const tAPP_XML_REM_DEVICE *p_xml_rem_devices, int devices_max)
{
    int fd, dummy;
    int index;
    int instance;

    if ((fd = open((char *)p_fname, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", p_fname);
        return -1;
    }

    app_xml_open_tag(fd, APP_XML_ROOT_KEY, TRUE);
    app_xml_open_tag(fd, APP_XML_DEVS_KEY, TRUE);

    /* For every element in device database */
    for (index = 0, instance = 0; index < devices_max; index++, p_xml_rem_devices++)
    {
        /* If this element is in use (not empty) */
        if (p_xml_rem_devices->in_use != FALSE)
        {
            app_xml_open_tag_with_value(fd, APP_XML_DEV_KEY, instance);

            app_xml_open_tag(fd, APP_XML_DEV_BDADDR_KEY, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->bd_addr, sizeof(p_xml_rem_devices->bd_addr), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_BDADDR_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_NAME_KEY, FALSE);
            dummy = write(fd, (char *) p_xml_rem_devices->name,
                    strlen((char *) p_xml_rem_devices->name));
            app_xml_close_tag(fd, APP_XML_DEV_NAME_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_CLASS_KEY, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->class_of_device, sizeof(p_xml_rem_devices->class_of_device), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_CLASS_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LINKKEY_KEY, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->link_key, sizeof(p_xml_rem_devices->link_key), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_LINKKEY_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LINKKEYP_KEY, FALSE);
            /* coverity[(PW.IMPLICIT_FUNC_DECL] False-positive: from stdio.h is included */
            dprintf(fd, "%d", p_xml_rem_devices->link_key_present);
            app_xml_close_tag(fd, APP_XML_DEV_LINKKEYP_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_KEYTYPE_KEY, FALSE);
            dprintf(fd, "%d", p_xml_rem_devices->key_type);
            app_xml_close_tag(fd, APP_XML_DEV_KEYTYPE_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_TRUSTED_KEY, FALSE);
            dprintf(fd, "0x%08X", (int) p_xml_rem_devices->trusted_services);
            app_xml_close_tag(fd, APP_XML_DEV_TRUSTED_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_VID_KEY, FALSE);
            dprintf(fd, "0x%04X", (int) p_xml_rem_devices->vid);
            app_xml_close_tag(fd, APP_XML_DEV_VID_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PID_KEY, FALSE);
            dprintf(fd, "0x%04X", (int) p_xml_rem_devices->pid);
            app_xml_close_tag(fd, APP_XML_DEV_PID_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_VERSION_KEY, FALSE);
            dprintf(fd, "0x%04X", (int) p_xml_rem_devices->version);
            app_xml_close_tag(fd, APP_XML_DEV_VERSION_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_FEATURES, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->features, sizeof(p_xml_rem_devices->features), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_FEATURES, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LMP_VERSION, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lmp_version);
            app_xml_close_tag(fd, APP_XML_DEV_LMP_VERSION, FALSE);

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
            app_xml_open_tag(fd, APP_XML_DEV_BLE_ADDR_TYPE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->ble_addr_type);
            app_xml_close_tag(fd, APP_XML_DEV_BLE_ADDR_TYPE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_DEVICE_TYPE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->device_type);
            app_xml_close_tag(fd, APP_XML_DEV_DEVICE_TYPE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_INQ_RESULT_TYPE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->inq_result_type);
            app_xml_close_tag(fd, APP_XML_DEV_INQ_RESULT_TYPE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_BLE_LINKKEYP_KEY, FALSE);
            dprintf(fd, "%d", p_xml_rem_devices->ble_link_key_present);
            app_xml_close_tag(fd, APP_XML_DEV_BLE_LINKKEYP_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PENC_LTK, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->penc_ltk, sizeof(p_xml_rem_devices->penc_ltk), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_PENC_LTK, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PENC_RAND, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->penc_rand, sizeof(p_xml_rem_devices->penc_rand), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_PENC_RAND, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PENC_EDIV, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->penc_ediv);
            app_xml_close_tag(fd, APP_XML_DEV_PENC_EDIV, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PENC_SEC_LEVEL, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->penc_sec_level);
            app_xml_close_tag(fd, APP_XML_DEV_PENC_SEC_LEVEL, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PENC_KEY_SIZE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->penc_key_size);
            app_xml_close_tag(fd, APP_XML_DEV_PENC_KEY_SIZE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PID_IRK, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->pid_irk, sizeof(p_xml_rem_devices->pid_irk), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_PID_IRK, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PID_ADDR_TYPE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->pid_addr_type);
            app_xml_close_tag(fd, APP_XML_DEV_PID_ADDR_TYPE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PID_STATIC_ADDR, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->pid_static_addr, sizeof(p_xml_rem_devices->pid_static_addr), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_PID_STATIC_ADDR, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PCSRK_COUNTER, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->pcsrk_counter);
            app_xml_close_tag(fd, APP_XML_DEV_PCSRK_COUNTER, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PCSRK_CSRK, FALSE);
            app_xml_write_data(fd, p_xml_rem_devices->pcsrk_csrk, sizeof(p_xml_rem_devices->pcsrk_csrk), FALSE);
            app_xml_close_tag(fd, APP_XML_DEV_PCSRK_CSRK, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_PCSRK_SEC_LEVEL, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->pcsrk_sec_level);
            app_xml_close_tag(fd, APP_XML_DEV_PCSRK_SEC_LEVEL, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LCSRK_COUNTER, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lcsrk_counter);
            app_xml_close_tag(fd, APP_XML_DEV_LCSRK_COUNTER, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LCSRK_DIV, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lcsrk_div);
            app_xml_close_tag(fd, APP_XML_DEV_LCSRK_DIV, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LCSRK_SEC_LEVEL, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lcsrk_sec_level);
            app_xml_close_tag(fd, APP_XML_DEV_LCSRK_SEC_LEVEL, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LENC_DIV, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lenc_div);
            app_xml_close_tag(fd, APP_XML_DEV_LENC_DIV, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LENC_KEY_SIZE, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lenc_key_size);
            app_xml_close_tag(fd, APP_XML_DEV_LENC_KEY_SIZE, FALSE);

            app_xml_open_tag(fd, APP_XML_DEV_LENC_SEC_LEVEL, FALSE);
            dprintf(fd, "%d", (int) p_xml_rem_devices->lenc_sec_level);
            app_xml_close_tag(fd, APP_XML_DEV_LENC_SEC_LEVEL, FALSE);

#endif
            app_xml_close_tag(fd, APP_XML_DEV_KEY, TRUE);

            instance++;
        }
    }

    app_xml_close_tag(fd, APP_XML_DEVS_KEY, TRUE);
    app_xml_close_tag(fd, APP_XML_ROOT_KEY, TRUE);

    fsync(fd);
    close(fd);

    (void)dummy;

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_write_si_db
 **
 ** Description     Writes the database file in file system or XML, creates one if it doesn't exist
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_write_si_db(const char *p_fname, const tAPP_XML_SI_DEVICE *p_xml_si_devices,
        int si_dev_max)
{
    int fd;
    int index;
    int instance;

    if ((fd = open((char *)p_fname, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        APP_ERROR1("Error opening/creating %s file", p_fname);
        return -1;
    }

    app_xml_open_tag(fd, APP_XML_ROOT_KEY, TRUE);
    app_xml_open_tag(fd, APP_XML_SI_DEVS_KEY, TRUE);

    /* For every element in device database */
    for (index = 0, instance = 0; index < si_dev_max; index++, p_xml_si_devices++)
    {
        /* If this element is in use (not empty) */
        if (p_xml_si_devices->in_use != FALSE)
        {
            app_xml_open_tag_with_value(fd, APP_XML_SI_DEV_KEY, instance);

            app_xml_open_tag(fd, APP_XML_SI_DEV_BDADDR_KEY, FALSE);
            app_xml_write_data(fd, p_xml_si_devices->bd_addr, sizeof(p_xml_si_devices->bd_addr), FALSE);
            app_xml_close_tag(fd, APP_XML_SI_DEV_BDADDR_KEY, FALSE);

            app_xml_open_tag(fd, APP_XML_SI_DEV_PLATFORM_KEY, FALSE);
            dprintf(fd, "%d", p_xml_si_devices->platform);
            app_xml_close_tag(fd, APP_XML_SI_DEV_PLATFORM_KEY, FALSE);

            app_xml_close_tag(fd, APP_XML_SI_DEV_KEY, TRUE);

            instance++;
        }
    }

    app_xml_close_tag(fd, APP_XML_SI_DEVS_KEY, TRUE);
    app_xml_close_tag(fd, APP_XML_ROOT_KEY, TRUE);

    fsync(fd);
    close(fd);

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_conf_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_conf_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    if (strncmp(tagName, APP_XML_CONF_ENABLE_KEY, strlen(
            APP_XML_CONF_ENABLE_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_ENABLE_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_DISCOVERABLE_KEY, strlen(
            APP_XML_CONF_DISCOVERABLE_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_DISCOVERABLE_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_CONNECTABLE_KEY, strlen(
            APP_XML_CONF_CONNECTABLE_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_CONNECTABLE_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_NAME_KEY, strlen(
            APP_XML_CONF_NAME_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_NAME_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_BDADDR_KEY, strlen(
            APP_XML_CONF_BDADDR_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_BDADDR_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_CLASS_KEY, strlen(
            APP_XML_CONF_CLASS_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_CLASS_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_PINCODE_KEY, strlen(
            APP_XML_CONF_PINCODE_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_PINCODE_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_PINLEN_KEY, strlen(
            APP_XML_CONF_PINLEN_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_PINLEN_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_IO_CAP_KEY, strlen(
            APP_XML_CONF_IO_CAP_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_IOCAP_KEY;
    }
    else if (strncmp(tagName, APP_XML_CONF_ROOTPATH_KEY, strlen(
            APP_XML_CONF_ROOTPATH_KEY)) == 0)
    {
        app_xml_param_cb.cfg_tag = CONF_ROOTPATH_KEY;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_conf_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_conf_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_xml_conf_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_conf_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
}

/*******************************************************************************
 **
 ** Function        app_xml_conf_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_conf_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete data, probably malformed XML file: %s", data);
    }

    switch (app_xml_param_cb.cfg_tag)
    {
    case CONF_ENABLE_KEY:
        app_xml_param_cb.config_ptr->enable = app_xml_read_value(data, len);
        break;

    case CONF_DISCOVERABLE_KEY:
        app_xml_param_cb.config_ptr->discoverable = app_xml_read_value(data, len);
        break;

    case CONF_CONNECTABLE_KEY:
        app_xml_param_cb.config_ptr->connectable = app_xml_read_value(data, len);
        break;

    case CONF_NAME_KEY:
        app_xml_read_string(&app_xml_param_cb.config_ptr->name,
                        sizeof(app_xml_param_cb.config_ptr->name), data, len);
        break;

    case CONF_BDADDR_KEY:
        if (app_xml_read_data_length(&app_xml_param_cb.config_ptr->bd_addr,
                sizeof(app_xml_param_cb.config_ptr->bd_addr), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading BD address in XML");
        }
        break;

    case CONF_CLASS_KEY:
        if (app_xml_read_data_length(&app_xml_param_cb.config_ptr->class_of_device,
                sizeof(app_xml_param_cb.config_ptr->class_of_device), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading BD address in XML");
        }
        break;

    case CONF_PINCODE_KEY:
        app_xml_read_string(&app_xml_param_cb.config_ptr->pin_code,
                        sizeof(app_xml_param_cb.config_ptr->pin_code), data, len);
        break;

    case CONF_PINLEN_KEY:
        app_xml_param_cb.config_ptr->pin_len =  app_xml_read_value(data, len);
        break;

    case CONF_IOCAP_KEY:
        app_xml_param_cb.config_ptr->io_cap =  app_xml_read_value(data, len);
        break;

    case CONF_ROOTPATH_KEY:
        app_xml_read_string(&app_xml_param_cb.config_ptr->root_path,
                        sizeof(app_xml_param_cb.config_ptr->root_path), data, len);
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_conf_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_conf_tagEndCallbackFunc(nxml_t handle, const char *tagName,
        unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_xml_dev_db_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_dev_db_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    if (strncmp(tagName, APP_XML_DEV_BDADDR_KEY,
            strlen(APP_XML_CONF_ENABLE_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_BDADDR_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_NAME_KEY,
            strlen(APP_XML_DEV_NAME_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_NAME_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_CLASS_KEY, strlen(
            APP_XML_DEV_CLASS_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_CLASS_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LINKKEY_KEY, strlen(
            APP_XML_DEV_LINKKEY_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LINKKEY_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LINKKEYP_KEY, strlen(
            APP_XML_DEV_LINKKEYP_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LINKKEYP_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_KEYTYPE_KEY, strlen(
            APP_XML_DEV_KEYTYPE_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_KEYTYPE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_TRUSTED_KEY, strlen(
            APP_XML_DEV_TRUSTED_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_TRUSTED_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PID_KEY, strlen(
            APP_XML_DEV_PID_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PID_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_VID_KEY, strlen(
            APP_XML_DEV_VID_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_VID_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_VERSION_KEY, strlen(
            APP_XML_DEV_VERSION_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_VERSION_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_FEATURES, strlen(
            APP_XML_DEV_FEATURES)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_FEATURES_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LMP_VERSION, strlen(
            APP_XML_DEV_LMP_VERSION)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LMP_VERSION_KEY;
    }
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    else if (strncmp(tagName, APP_XML_DEV_BLE_ADDR_TYPE, strlen(
            APP_XML_DEV_BLE_ADDR_TYPE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_BLE_ADDR_TYPE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_DEVICE_TYPE, strlen(
            APP_XML_DEV_DEVICE_TYPE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_DEVICE_TYPE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_INQ_RESULT_TYPE, strlen(
            APP_XML_DEV_DEVICE_TYPE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_INQ_RESULT_TYPE;
    }
    else if (strncmp(tagName, APP_XML_DEV_BLE_LINKKEYP_KEY, strlen(
            APP_XML_DEV_BLE_LINKKEYP_KEY)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_BLE_LINKKEYP_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PENC_LTK, strlen(
            APP_XML_DEV_PENC_LTK)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PENC_LTK_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PENC_RAND, strlen(
            APP_XML_DEV_PENC_RAND)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PENC_RAND_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PENC_EDIV, strlen(
            APP_XML_DEV_PENC_EDIV)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PENC_EDIV_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PENC_SEC_LEVEL, strlen(
            APP_XML_DEV_PENC_SEC_LEVEL)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PENC_SEC_LEVEL_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PENC_KEY_SIZE, strlen(
            APP_XML_DEV_PENC_KEY_SIZE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PENC_KEY_SIZE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PID_IRK, strlen(
            APP_XML_DEV_PID_IRK)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PID_IRK_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PID_ADDR_TYPE, strlen(
            APP_XML_DEV_PID_ADDR_TYPE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PID_ADDR_TYPE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PID_STATIC_ADDR, strlen(
            APP_XML_DEV_PID_STATIC_ADDR)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PID_STATIC_ADDR_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PCSRK_COUNTER, strlen(
            APP_XML_DEV_PCSRK_COUNTER)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PCSRK_COUNTER_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PCSRK_CSRK, strlen(
            APP_XML_DEV_PCSRK_CSRK)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PCSRK_CSRK_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_PCSRK_SEC_LEVEL, strlen(
            APP_XML_DEV_PCSRK_SEC_LEVEL)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_PCSRK_SEC_LEVEL_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LCSRK_COUNTER, strlen(
            APP_XML_DEV_LCSRK_COUNTER)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LCSRK_COUNTER_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LCSRK_DIV, strlen(
            APP_XML_DEV_LCSRK_DIV)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LCSRK_DIV_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LCSRK_SEC_LEVEL, strlen(
            APP_XML_DEV_LCSRK_SEC_LEVEL)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LCSRK_SEC_LEVEL_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LENC_DIV, strlen(
            APP_XML_DEV_LENC_DIV)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LENC_DIV_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LENC_KEY_SIZE, strlen(
            APP_XML_DEV_LENC_KEY_SIZE)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LENC_KEY_SIZE_KEY;
    }
    else if (strncmp(tagName, APP_XML_DEV_LENC_SEC_LEVEL, strlen(
            APP_XML_DEV_LENC_SEC_LEVEL)) == 0)
    {
        app_xml_param_cb.dev_tag = DEV_LENC_SEC_LEVEL_KEY;
    }
#endif
}

/*******************************************************************************
 **
 ** Function        app_xml_dev_db_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_dev_db_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
    if (strncmp(attrName, APP_XML_DEV_INST_KEY, strlen(APP_XML_DEV_INST_KEY))
            == 0)
    {
        app_xml_param_cb.dev_tag = DEV_INST_KEY;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_dev_db_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_dev_db_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete attribute value, probably malformed XML file: %s", attrValue);
    }

    app_xml_param_cb.dev_instance = app_xml_read_value(attrValue, len);

    if (app_xml_param_cb.dev_instance >= app_xml_param_cb.devices_max)
    {
        APP_ERROR1("bad instance number found:%d", app_xml_param_cb.dev_instance);
        app_xml_param_cb.dev_instance = 0;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_dev_db_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_dev_db_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    tAPP_XML_REM_DEVICE *devices_db_ptr;

    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete data, probably malformed XML file: %s", data);
    }

    devices_db_ptr = app_xml_param_cb.devices_db_ptr + app_xml_param_cb.dev_instance;

    switch (app_xml_param_cb.dev_tag)
    {
    case DEV_BDADDR_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->bd_addr,
                sizeof(devices_db_ptr->bd_addr), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance BD address in XML");
        }
        devices_db_ptr-> in_use = 1;
        break;

    case DEV_NAME_KEY:
        /* Copy the name in the instance */
        app_xml_read_string(devices_db_ptr->name, sizeof(devices_db_ptr->name),
                data, len);
        break;

    case DEV_CLASS_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->class_of_device,
                sizeof(devices_db_ptr->class_of_device), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance class of device in XML");
        }
        break;

    case DEV_LINKKEY_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->link_key,
                sizeof(devices_db_ptr->link_key), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance link key in XML");
        }
        break;

    case DEV_LINKKEYP_KEY:
        devices_db_ptr->link_key_present = app_xml_read_value(data, len);
        break;

    case DEV_KEYTYPE_KEY:
        devices_db_ptr->key_type = app_xml_read_value(data, len);
        break;

    case DEV_TRUSTED_KEY:
        devices_db_ptr->trusted_services = app_xml_read_value(data, len);
        break;

    case DEV_PID_KEY:
        devices_db_ptr->pid = app_xml_read_value(data, len);
        break;

    case DEV_VID_KEY:
        devices_db_ptr->vid = app_xml_read_value(data, len);
        break;

    case DEV_VERSION_KEY:
        devices_db_ptr->version = app_xml_read_value(data, len);
        break;

    case DEV_FEATURES_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->features,
                sizeof(devices_db_ptr->features), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance features in XML");
        }
        break;

    case DEV_LMP_VERSION_KEY:
        devices_db_ptr->lmp_version = app_xml_read_value(data, len);
        break;

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    case DEV_BLE_ADDR_TYPE_KEY:
        devices_db_ptr->ble_addr_type = app_xml_read_value(data, len);
        break;

    case DEV_DEVICE_TYPE_KEY:
        devices_db_ptr->device_type = app_xml_read_value(data, len);
        break;

    case DEV_INQ_RESULT_TYPE:
        devices_db_ptr->inq_result_type= app_xml_read_value(data, len);
        break;

    case DEV_BLE_LINKKEYP_KEY:
        devices_db_ptr->ble_link_key_present = app_xml_read_value(data, len);
        break;

    case DEV_PENC_LTK_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->penc_ltk,
                sizeof(devices_db_ptr->penc_ltk), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance penc ltk in XML");
        }
        break;

    case DEV_PENC_RAND_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->penc_rand,
                sizeof(devices_db_ptr->penc_rand), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance penc rand in XML");
        }
        break;

    case DEV_PENC_EDIV_KEY:
        devices_db_ptr->penc_ediv = app_xml_read_value(data, len);
        break;

    case DEV_PENC_SEC_LEVEL_KEY:
        devices_db_ptr->penc_sec_level = app_xml_read_value(data, len);
        break;

    case DEV_PENC_KEY_SIZE_KEY:
        devices_db_ptr->penc_key_size = app_xml_read_value(data, len);
        break;

    case DEV_PID_IRK_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->pid_irk,
                sizeof(devices_db_ptr->pid_irk), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance pid irk in XML");
        }
        break;

    case DEV_PID_ADDR_TYPE_KEY:
        devices_db_ptr->pid_addr_type = app_xml_read_value(data, len);
        break;

    case DEV_PID_STATIC_ADDR_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->pid_static_addr,
                sizeof(devices_db_ptr->pid_static_addr), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance pid static addr in XML");
        }
        break;

    case DEV_PCSRK_COUNTER_KEY:
        devices_db_ptr->pcsrk_counter = app_xml_read_value(data, len);
        break;

    case DEV_PCSRK_CSRK_KEY:
        if (app_xml_read_data_length(&devices_db_ptr->pcsrk_csrk,
                sizeof(devices_db_ptr->pcsrk_csrk), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance pcsrk csrk in XML");
        }
        break;

    case DEV_PCSRK_SEC_LEVEL_KEY:
        devices_db_ptr->pcsrk_sec_level = app_xml_read_value(data, len);
        break;

    case DEV_LCSRK_COUNTER_KEY:
        devices_db_ptr->lcsrk_counter = app_xml_read_value(data, len);
        break;

    case DEV_LCSRK_DIV_KEY:
        devices_db_ptr->lcsrk_div = app_xml_read_value(data, len);
        break;

    case DEV_LCSRK_SEC_LEVEL_KEY:
        devices_db_ptr->lcsrk_sec_level = app_xml_read_value(data, len);
        break;

    case DEV_LENC_DIV_KEY:
        devices_db_ptr->lenc_div = app_xml_read_value(data, len);
        break;

    case DEV_LENC_KEY_SIZE_KEY:
        devices_db_ptr->lenc_key_size = app_xml_read_value(data, len);
        break;

    case DEV_LENC_SEC_LEVEL_KEY:
        devices_db_ptr->lenc_sec_level = app_xml_read_value(data, len);
        break;

#endif
    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_dev_db_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_dev_db_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_xml_si_dev_db_tagBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_si_dev_db_tagBeginCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
    if (strncmp(tagName, APP_XML_SI_DEV_BDADDR_KEY,
            strlen(APP_XML_SI_DEV_BDADDR_KEY)) == 0)
    {
        app_xml_param_cb.si_dev_tag = SI_DEV_BDADDR_KEY;
    }
    else if (strncmp(tagName, APP_XML_SI_DEV_PLATFORM_KEY,
            strlen(APP_XML_SI_DEV_PLATFORM_KEY)) == 0)
    {
        app_xml_param_cb.si_dev_tag = SI_DEV_PLATFORM_KEY;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_si_dev_db_attrBeginCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_si_dev_db_attrBeginCallbackFunc(nxml_t handle,
        const char *attrName, unsigned len)
{
    if (strncmp(attrName, APP_XML_SI_DEV_INST_KEY, strlen(APP_XML_SI_DEV_INST_KEY))
            == 0)
    {
        app_xml_param_cb.si_dev_tag = SI_DEV_INST_KEY;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_si_dev_db_attrValueCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_si_dev_db_attrValueCallbackFunc(nxml_t handle,
        const char *attrValue, unsigned len, int more)
{
    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete attribute value, probably malformed XML file: %s", attrValue);
    }

    app_xml_param_cb.si_dev_instance = app_xml_read_value(attrValue, len);

    if (app_xml_param_cb.si_dev_instance >= app_xml_param_cb.si_dev_max)
    {
        APP_ERROR1("bad instance number found:%d", app_xml_param_cb.si_dev_instance);
        app_xml_param_cb.si_dev_instance = 0;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_si_dev_db_dataCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_si_dev_db_dataCallbackFunc(nxml_t handle, const char *data,
        unsigned len, int more)
{
    tAPP_XML_SI_DEVICE *si_dev_db_ptr;

    /* Check that the file is not malformed */
    if (more)
    {
        APP_ERROR1("Non complete data, probably malformed XML file: %s", data);
    }

    si_dev_db_ptr = app_xml_param_cb.si_dev_db_ptr + app_xml_param_cb.si_dev_instance;

    switch (app_xml_param_cb.si_dev_tag)
    {
    case SI_DEV_BDADDR_KEY:
        if (app_xml_read_data_length(&si_dev_db_ptr->bd_addr,
                sizeof(si_dev_db_ptr->bd_addr), data, len) == FALSE)
        {
            APP_ERROR0("Failed reading instance BD address in XML");
        }
        si_dev_db_ptr-> in_use = 1;
        break;

    case SI_DEV_PLATFORM_KEY:
        si_dev_db_ptr->platform = app_xml_read_value(data, len);
        break;

    default:
        break;
    }
}

/*******************************************************************************
 **
 ** Function        app_xml_si_dev_db_tagEndCallbackFunc
 **
 ** Description     Nanoxml call back function
 **
 ** Returns         None
 **
 *******************************************************************************/
static void app_xml_si_dev_db_tagEndCallbackFunc(nxml_t handle,
        const char *tagName, unsigned len)
{
}

/*******************************************************************************
 **
 ** Function        app_xml_add_dev_db
 **
 ** Description     Update link key information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_add_dev_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr)
{
    int index;
    tAPP_XML_REM_DEVICE cur;

    /*look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            cur = p_stored_device_db[index];
            break;
        }
    }

    if (index >= nb_device_max) {//no space , remove oldest db
        if (nb_device_max > 1)
            memmove(&p_stored_device_db[1], p_stored_device_db,
                            sizeof(tAPP_XML_REM_DEVICE) * (nb_device_max -1));
        memset(&p_stored_device_db[0], 0, sizeof(tAPP_XML_REM_DEVICE));
        p_stored_device_db[0].in_use = TRUE;
        bdcpy(p_stored_device_db[0].bd_addr, bd_addr);
    } else if (index > 0) {//update cur to lasted db
        memmove(&p_stored_device_db[1], p_stored_device_db,
                            sizeof(tAPP_XML_REM_DEVICE) * index);
        p_stored_device_db[0] = cur;
    }

    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_find_dev_db
 **
 ** Description     Find the pointer to the device info in the list
 **
 ** Returns         Pointer to the device in the D, NULL if not found
 **
 *******************************************************************************/
tAPP_XML_REM_DEVICE *app_xml_find_dev_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr)
{
    int index;

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use) &&
            (bdcmp(p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            return &p_stored_device_db[index];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_key_db
 **
 ** Description     Update link key information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_key_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, LINK_KEY link_key,
        unsigned char key_type)
{
    int index;
    int ret_code = -1;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            ret_code = 0;
            printf("Update link key\n");
            memcpy(p_stored_device_db[index].link_key, link_key,
                    sizeof(LINK_KEY));
            p_stored_device_db[index].link_key_present = TRUE;
            p_stored_device_db[index].key_type = key_type;
            break;
        }
    }
    return ret_code;
}

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function        app_xml_update_ble_key_db
 **
 ** Description     Update BLE keys information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_ble_key_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBSA_LE_KEY_VALUE ble_key,
        tBSA_LE_KEY_TYPE ble_key_type)
{
    int index;
    int ret_code = -1;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            ret_code = 0;
            printf("Update BLE link key\n");
            p_stored_device_db[index].ble_link_key_present = TRUE;
            switch (ble_key_type)
            {
            case BSA_LE_KEY_PENC:
                printf("Update BSA_LE_KEY_PENC\n");
                memcpy(p_stored_device_db[index].penc_ltk, ble_key.penc_key.ltk,
                        sizeof(p_stored_device_db[index].penc_ltk));
                memcpy(p_stored_device_db[index].penc_rand, ble_key.penc_key.rand,
                        sizeof(p_stored_device_db[index].penc_rand));
                p_stored_device_db[index].penc_ediv = ble_key.penc_key.ediv;
                p_stored_device_db[index].penc_sec_level = ble_key.penc_key.sec_level;
                p_stored_device_db[index].penc_key_size = ble_key.penc_key.key_size;
                break;
            case BSA_LE_KEY_PID:
                printf("Update BSA_LE_KEY_PID\n");
                memcpy(p_stored_device_db[index].pid_irk, ble_key.pid_key.irk,
                        sizeof(p_stored_device_db[index].pid_irk));
                p_stored_device_db[index].pid_addr_type = ble_key.pid_key.addr_type;
                bdcpy(p_stored_device_db[index].pid_static_addr, ble_key.pid_key.static_addr);
                break;
            case BSA_LE_KEY_PCSRK:
                printf("Update BSA_LE_KEY_PCSRK\n");
                p_stored_device_db[index].pcsrk_counter = ble_key.pcsrk_key.counter;
                memcpy(p_stored_device_db[index].pcsrk_csrk, ble_key.pcsrk_key.csrk,
                        sizeof(p_stored_device_db[index].pcsrk_csrk));
                p_stored_device_db[index].pcsrk_sec_level = ble_key.pcsrk_key.sec_level;
                break;
            case BSA_LE_KEY_LCSRK:
                printf("Update BSA_LE_KEY_LCSRK\n");
                p_stored_device_db[index].lcsrk_counter = ble_key.lcsrk_key.counter;
                p_stored_device_db[index].lcsrk_div = ble_key.lcsrk_key.div;
                p_stored_device_db[index].lcsrk_sec_level = ble_key.lcsrk_key.sec_level;
                break;
            case BSA_LE_KEY_LENC:
                printf("Update BSA_LE_KEY_LENC\n");
                p_stored_device_db[index].lenc_div = ble_key.lenc_key.div;
                p_stored_device_db[index].lenc_key_size = ble_key.lenc_key.key_size;
                p_stored_device_db[index].lenc_sec_level = ble_key.lenc_key.sec_level;
                break;
            case BSA_LE_KEY_LID:
                printf("Update BSA_LE_KEY_LID\n");
                break;
            default:
                printf("Unknown key(%d)", ble_key_type);
                break;
            }
            break;
        }
    }
    return ret_code;
}
#endif

/*******************************************************************************
 **
 ** Function        app_xml_add_trusted_services_db
 **
 ** Description     Add a trusted service for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_add_trusted_services_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBSA_SERVICE_MASK trusted_services)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Added trusted services\n");
            p_stored_device_db[index].trusted_services |= trusted_services;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_name_db
 **
 ** Description     Update link key information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_name_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, BD_NAME name)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update name with %s\n", name);
            strncpy((char *) p_stored_device_db[index].name, (char *) name,
                    sizeof(p_stored_device_db[index].name) - 1);
            p_stored_device_db[index].name[sizeof(p_stored_device_db[index].name) - 1] = '\0';
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_cod_db
 **
 ** Description     Update Class of device information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_cod_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, DEV_CLASS class_of_device)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update class-of-device [0x%02X-0x%02X-0x%02X]\n",
                    class_of_device[0], class_of_device[1], class_of_device[2]);
            p_stored_device_db[index].class_of_device[0] = class_of_device[0];
            p_stored_device_db[index].class_of_device[1] = class_of_device[1];
            p_stored_device_db[index].class_of_device[2] = class_of_device[2];
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_display_devices
 **
 ** Description     Display a list of device stored in database
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_display_devices(const tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max)
{
    int index;

    /* First look in database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if (p_stored_device_db[index].in_use != FALSE)
        {
            printf("Dev:%d\n", index);
            printf("\tBdaddr:%02x:%02x:%02x:%02x:%02x:%02x\n",
                    p_stored_device_db[index].bd_addr[0],
                    p_stored_device_db[index].bd_addr[1],
                    p_stored_device_db[index].bd_addr[2],
                    p_stored_device_db[index].bd_addr[3],
                    p_stored_device_db[index].bd_addr[4],
                    p_stored_device_db[index].bd_addr[5]);
            printf("\tName:%s\n", p_stored_device_db[index].name);
            printf("\tClassOfDevice:%02x:%02x:%02x => %s\n",
                    p_stored_device_db[index].class_of_device[0],
                    p_stored_device_db[index].class_of_device[1],
                    p_stored_device_db[index].class_of_device[2],
                    app_get_cod_string(
                            p_stored_device_db[index].class_of_device));
            printf("\tTrusted Services:%x\n",
                    (int) p_stored_device_db[index].trusted_services);
            if (p_stored_device_db[index].link_key_present != FALSE)
                printf("\tLink Key present:TRUE\n");
            else
                printf("\tLink Key present:FALSE\n");

            printf("\tPid:%x Vid:%d\n", p_stored_device_db[index].pid,
                    p_stored_device_db[index].vid);
        }
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_pidvid_db
 **
 ** Description     Update the PID/VID information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_pidvid_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT16 pid, UINT16 vid)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update Pid:0x%x Vid:0x%x\n", pid, vid);
            p_stored_device_db[index].pid = pid;
            p_stored_device_db[index].vid = vid;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_version_db
 **
 ** Description     Update the version information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_version_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT16 version)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update Version:0x%04X\n", version);
            p_stored_device_db[index].version = version;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_features_db
 **
 ** Description     Update Features information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_features_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, BD_FEATURES features)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update features:%02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
                    features[0], features[1], features[2], features[3],
                    features[3], features[4], features[5], features[6]);
            memcpy(p_stored_device_db[index].features, features, sizeof(BD_FEATURES));
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_lmp_version_db
 **
 ** Description     Update LMP Version information for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_lmp_version_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 lmp_version)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update lmp_version:%d\n", lmp_version);
            p_stored_device_db[index].lmp_version = lmp_version;
            return 0;
        }
    }
    return -1;
}

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function        app_xml_update_ble_addr_type_db
 **
 ** Description     Update ble_addr_type for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_ble_addr_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 ble_addr_type)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update ble_addr_type:%d\n", ble_addr_type);
            p_stored_device_db[index].ble_addr_type = ble_addr_type;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_device_type_db
 **
 ** Description     Update device_type for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_device_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBT_DEVICE_TYPE device_type)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update device_type:%d\n", device_type);
            p_stored_device_db[index].device_type = device_type;
            return 0;
        }
    }
    return -1;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_inq_result_type_db
 **
 ** Description     Update inq_result_type for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_inq_result_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 inq_res_type)
{
    int index;

    app_xml_add_dev_db(p_stored_device_db, nb_device_max, bd_addr);

    /* First look in Database if this device already exist */
    for (index = 0; index < nb_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use != FALSE) && (bdcmp(
                p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            printf("Update inquiry result type :%d\n", inq_res_type);
            p_stored_device_db[index].inq_result_type= inq_res_type;
            return 0;
        }
    }
    return -1;
}


#endif

/*******************************************************************************
 **
 ** Function        app_xml_add_si_dev_db
 **
 ** Description     Add a special interest device to database
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
tAPP_XML_SI_DEVICE *app_xml_add_si_dev_db(tAPP_XML_SI_DEVICE *p_stored_device_db,
        int si_device_max, BD_ADDR bd_addr)
{
    int index;

    /* First look in Database if this device already exist */
    for (index = 0; index < si_device_max; index++)
    {
        if ((p_stored_device_db[index].in_use) &&
            (bdcmp(p_stored_device_db[index].bd_addr, bd_addr) == 0))
        {
            return &p_stored_device_db[index];
        }
    }

    /* Look for a free location in database */
    for (index = 0; index < si_device_max; index++)
    {
        /* If this one is free */
        if (p_stored_device_db[index].in_use == FALSE)
        {
            memset(&p_stored_device_db[index], 0, sizeof(tAPP_XML_SI_DEVICE));
            /* Let's use it */
            p_stored_device_db[index].in_use = TRUE;
            bdcpy(p_stored_device_db[index].bd_addr, bd_addr);
            return  &p_stored_device_db[index];
        }
    }
    return NULL;
}

/*******************************************************************************
 **
 ** Function        app_xml_update_si_dev_platform_db
 **
 ** Description     Update platform for a device
 **
 ** Returns         0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_xml_update_si_dev_platform_db(tAPP_XML_SI_DEVICE *p_stored_device_db,
        int si_device_max, BD_ADDR bd_addr, UINT8 platform)
{
   tAPP_XML_SI_DEVICE *p_si_device =  app_xml_add_si_dev_db(p_stored_device_db,
            si_device_max, bd_addr);

    if (p_si_device != NULL)
    {
        printf("Update platform:%d\n", platform);
        p_si_device->platform = platform;
        return 0;
    }
    return -1;
}

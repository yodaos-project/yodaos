/*****************************************************************************
 **
 **  Name:           app_xml_param.h
 **
 **  Description:    This module contains utility functions to access bluetooth
 **                  parameters (configuration and remote devices).
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_XML_PARAM_H_
#define APP_XML_PARAM_H_

#include <bsa_rokid/bt_target.h>

#include <bsa_rokid/bsa_api.h>

#ifndef APP_ROOT_LEN
#define APP_ROOT_LEN 500
#endif

/*****************************************************************************
 **  New types
 *****************************************************************************/
typedef struct
{
    BOOLEAN enable; /* Is Bluetooth enabled/disabled */
    BOOLEAN discoverable; /* Is bluetooth device discoverable (InquiryScan) */
    BOOLEAN connectable; /* Is bluetooth device Connectable (PageScan) */
    BD_ADDR bd_addr; /* Local BdAddr */
    BD_NAME name; /* Local Name */
    DEV_CLASS class_of_device;/* Class Of Device */
    char root_path[APP_ROOT_LEN]; /* Root path to configure data exchange profiles */
    PIN_CODE pin_code;
    char pin_len;
    tBSA_SEC_IO_CAP io_cap;
} tAPP_XML_CONFIG;

/* remote device */
typedef struct
{
    BOOLEAN in_use;
    BD_ADDR bd_addr;
    BD_NAME name;
    DEV_CLASS class_of_device;
    tBSA_SERVICE_MASK available_services;
    tBSA_SERVICE_MASK trusted_services;
    BOOLEAN is_default_hs;
    BOOLEAN stored;
    BOOLEAN link_key_present;
    LINK_KEY link_key;
    unsigned char key_type;
    tBSA_SEC_IO_CAP io_cap;
    UINT16 pid;
    UINT16 vid;
    UINT16 version;
    BD_FEATURES features;
    UINT8 lmp_version;
#if 1
    UINT8 ble_addr_type;
    tBT_DEVICE_TYPE device_type;
    UINT8 inq_result_type;
    BOOLEAN ble_link_key_present;
    /* KEY_PENC */
    BT_OCTET16 penc_ltk;
    BT_OCTET8 penc_rand;
    UINT16 penc_ediv;
    UINT8 penc_sec_level;
    UINT8 penc_key_size;
    /* KEY_PID */
    BT_OCTET16 pid_irk;
    tBLE_ADDR_TYPE pid_addr_type;
    BD_ADDR pid_static_addr;
    /* KEY_PCSRK */
    UINT32 pcsrk_counter;
    BT_OCTET16 pcsrk_csrk;
    UINT8 pcsrk_sec_level;
    /* KEY_LCSRK */
    UINT32 lcsrk_counter;
    UINT16 lcsrk_div;
    UINT8 lcsrk_sec_level;
    /* KEY_LENC */
    UINT16 lenc_div;
    UINT8 lenc_key_size;
    UINT8 lenc_sec_level;
#endif
} tAPP_XML_REM_DEVICE;

/* special interest device */
typedef struct
{
    BOOLEAN in_use;
    BD_ADDR bd_addr;
    UINT8 platform;
} tAPP_XML_SI_DEVICE;

int app_xml_write_link_status(const char *p_fname, BOOLEAN link);//rokid define

int app_xml_init(void);
int app_xml_read_cfg(const char *p_fname, tAPP_XML_CONFIG *p_xml_config);
int app_xml_read_db(const char *xml_devices_file, tAPP_XML_REM_DEVICE *p_xml_rem_devices, int devices_max);
int app_xml_read_si_db(const char *xml_si_dev_file, tAPP_XML_SI_DEVICE *p_xml_si_devices, int si_dev_max);
int app_xml_write_cfg(const char *p_fname, const tAPP_XML_CONFIG *p_xml_config);
int app_xml_write_db(const char *p_fname, const tAPP_XML_REM_DEVICE *p_xml_rem_devices, int devices_max);
int app_xml_write_si_db(const char *p_fname, const tAPP_XML_SI_DEVICE *p_xml_si_devices, int si_dev_max);

int app_xml_display_devices(const tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max);
int app_xml_add_dev_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr);
tAPP_XML_REM_DEVICE *app_xml_find_dev_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr);

int app_xml_update_name_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, BD_NAME name);

int app_xml_update_key_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, LINK_KEY link_key,
        unsigned char key_type);

int app_xml_update_cod_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, DEV_CLASS class_of_device);

int app_xml_add_trusted_services_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBSA_SERVICE_MASK trusted_services);

int app_xml_update_pidvid_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT16 pid, UINT16 vid);

int app_xml_update_version_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT16 version);

int app_xml_update_features_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, BD_FEATURES features);

int app_xml_update_lmp_version_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 lmp_version);

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
int app_xml_update_ble_addr_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 ble_addr_type);

int app_xml_update_device_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBT_DEVICE_TYPE device_type);

int app_xml_update_inq_result_type_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, UINT8 inq_res_type);

int app_xml_update_ble_key_db(tAPP_XML_REM_DEVICE *p_stored_device_db,
        int nb_device_max, BD_ADDR bd_addr, tBSA_LE_KEY_VALUE ble_key,
        tBSA_LE_KEY_TYPE ble_key_type);
#endif

int app_xml_update_si_dev_platform_db(tAPP_XML_SI_DEVICE *p_stored_device_db,
        int si_device_max, BD_ADDR bd_addr, UINT8 platform);

#endif /* BTAPP_PARAM_H_ */

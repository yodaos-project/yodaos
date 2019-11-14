/*****************************************************************************
 **
 **  Name:               app_dm.c
 **
 **  Description:        Device Management utility functions for applications
 **
 **  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <bsa_rokid/bsa_api.h>
#include "app_dm.h"
#include "app_utils.h"
#include "app_xml_param.h"
#include "app_xml_utils.h"

/*******************************************************************************
 **
 ** Function        app_dm_get_local_bt_config
 **
 ** Description     Get the Bluetooth configuration
 **
 ** Parameters      None
 **
 ** Returns         1 if BT enabled/0 if BT disabled/-1 in case of error
 **
 *******************************************************************************/
int app_dm_get_local_bt_config(void)
{
    int status;
    tBSA_DM_GET_CONFIG bt_config;

    /*
     * Get Bluetooth configuration
     */
    status = BSA_DmGetConfigInit(&bt_config);
    status = BSA_DmGetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to get BT config from server status:%d", status);
        return(-1);
    }
    APP_INFO0("Get Local Bluetooth Config:");
    APP_INFO1("\tEnable:%d", bt_config.enable);
    APP_INFO1("\tDiscoverable:%d", bt_config.discoverable);
    APP_INFO1("\tConnectable:%d", bt_config.connectable);
    APP_INFO1("\tName:%s", bt_config.name);
    APP_INFO1 ("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
             bt_config.bd_addr[0], bt_config.bd_addr[1],
             bt_config.bd_addr[2], bt_config.bd_addr[3],
             bt_config.bd_addr[4], bt_config.bd_addr[5]);
    APP_INFO1("\tClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
            bt_config.class_of_device[1], bt_config.class_of_device[2]);

    if(bt_config.enable)
        return 1;
    else
        return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_local_bt_config
 **
 ** Description     Set the Bluetooth basic configuration
 **
 ** Parameters      enable: 0 to disable Bluetooth; 1 to enable it
 **
 ** Returns         0 if success /-1 in case of error
 **
 *******************************************************************************/
int app_dm_set_local_bt_config(BOOLEAN enable)
{
    int status;
    tBSA_DM_SET_CONFIG bt_config;
    tAPP_XML_CONFIG xml_local_config;

    status = app_read_xml_config(&xml_local_config);
    if (status < 0)
    {
        APP_ERROR0("Unable to Read XML config file");
        return status;
    }

    /* Init config parameter */
    status = BSA_DmSetConfigInit(&bt_config);

    /* Set Bluetooth configuration (from XML contents) */
    bt_config.enable = enable;
    bt_config.discoverable = xml_local_config.discoverable;
    bt_config.connectable = xml_local_config.connectable;
    bdcpy(bt_config.bd_addr, xml_local_config.bd_addr);
    strncpy((char *)bt_config.name, (char *)xml_local_config.name, sizeof(bt_config.name));
    bt_config.name[sizeof(bt_config.name)-1] = '\0';
    memcpy(bt_config.class_of_device, xml_local_config.class_of_device, sizeof(DEV_CLASS));

    APP_INFO0("Set Local Bluetooth Config:");
    APP_INFO1("\tEnable:%d", bt_config.enable);
    APP_INFO1("\tDiscoverable:%d", bt_config.discoverable);
    APP_INFO1("\tConnectable:%d", bt_config.connectable);
    APP_INFO1("\tName:%s", bt_config.name);
    APP_INFO1("\tBdaddr %02x:%02x:%02x:%02x:%02x:%02x",
            bt_config.bd_addr[0], bt_config.bd_addr[1],
            bt_config.bd_addr[2], bt_config.bd_addr[3],
            bt_config.bd_addr[4], bt_config.bd_addr[5]);
    APP_INFO1("\tClassOfDevice:%02x:%02x:%02x", bt_config.class_of_device[0],
            bt_config.class_of_device[1], bt_config.class_of_device[2]);

    /* Apply BT config */
    status = BSA_DmSetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to set BT config to server status:%d", status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dm_set_channel_map
 **
 ** Description      This function is used to avoid an AFH channel range
 **
 ** Parameters
 **             first_afh_ch: first afh channel to avoid
 **             last_afh_ch: last afh channel to avoid
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_channel_map(int first_afh_ch, int last_afh_ch)
{
    tBSA_DM_SET_CONFIG new_bt_config;
    tBSA_STATUS status;

    BSA_DmSetConfigInit(&new_bt_config);
    new_bt_config.first_disabled_channel = first_afh_ch;
    new_bt_config.last_disabled_channel = last_afh_ch;

    new_bt_config.config_mask = BSA_DM_CONFIG_CHANNEL_MASK;

    status = BSA_DmSetConfig(&new_bt_config);

    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed:%d", status);
        return -1;
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function         app_dm_set_tx_class
 **
 ** Description      This function sets the Tx Power Class
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_dm_set_tx_class(UINT8 tx_power)
{
    int status;
    tBSA_DM_SET_CONFIG bt_config;

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* BT must be enabled */
    bt_config.enable = TRUE;

    /* Indicate the specific entries to configure */
    bt_config.config_mask = BSA_DM_CONFIG_TX_POWER_MASK;
    bt_config.tx_power = tx_power;

    status = BSA_DmSetConfig(&bt_config);
    if (status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to change Tx power status:%d", status);
    }
    return status;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_page_scan_param
 **
 ** Description     Configure the Page Scan parameters (interval & window)
 **
 ** Parameters      Interval: PageScan Interval (in slot)
 **                 windows: PageScan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_page_scan_param(UINT16 interval, UINT16 window)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Interval:%d (%dms) window:%d (%dms)", interval, (int)((float)interval*0.625),
            window, (int)((float)window*0.625));

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Page Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_PAGE_SCAN_PARAM_MASK;
    bt_config.page_scan_interval = interval;
    bt_config.page_scan_window = window;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_inquiry_scan_param
 **
 ** Description     Configure the Inquiry Scan parameters (interval & window)
 **
 ** Parameters      Interval: InquiryScan Interval (in slot)
 **                 windows: InquiryScan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_inquiry_scan_param(UINT16 interval, UINT16 window)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Interval:%d (%dms) window:%d (%dms)", interval, (int)((float)interval*0.625),
            window, (int)((float)window*0.625));

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_INQ_SCAN_PARAM_MASK;
    bt_config.inquiry_scan_interval = interval;
    bt_config.inquiry_scan_window = window;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_scan_param
 **
 ** Description     Configure the BLE Scan parameters (interval & window)
 **
 ** Parameters      Interval: BLE Scan Interval (in slot)
 **                 windows: BLE Scan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_scan_param(UINT16 interval, UINT16 window)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Interval:%d (%dms) window:%d (%dms)", interval, (int)((float)interval*0.625),
            window, (int)((float)window*0.625));

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_SCAN_PARAM_MASK;
    bt_config.ble_scan_interval = interval;
    bt_config.ble_scan_window = window;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_conn_param
 **
 ** Description     Configure the BLE Prefer Conn parameters
 **
 ** Parameters      pointer to tBSA_DM_BLE_CONN_PARAM
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_conn_param(tBSA_DM_BLE_CONN_PARAM *p_req)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
            p_req->bd_addr[0], p_req->bd_addr[1],
            p_req->bd_addr[2], p_req->bd_addr[3],
            p_req->bd_addr[4], p_req->bd_addr[5]);

    APP_DEBUG1("min_conn_int:%d max_conn_int:%d slave_latency:%d supervision_tout:%d",
            p_req->max_conn_int, p_req->min_conn_int,
            p_req->slave_latency, p_req->supervision_tout);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_CONN_PARAM_MASK;
    bdcpy(bt_config.ble_conn_param.bd_addr, p_req->bd_addr);
    bt_config.ble_conn_param.max_conn_int = p_req->max_conn_int;
    bt_config.ble_conn_param.min_conn_int = p_req->min_conn_int;
    bt_config.ble_conn_param.slave_latency = p_req->slave_latency;
    bt_config.ble_conn_param.supervision_tout = p_req->supervision_tout;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_conn_scan_param
 **
 ** Description     Configure the BLE Connecton Scan parameters (interval & window)
 **
 ** Parameters      Interval: BLE Scan Interval (in slot)
 **                 windows: BLE Scan Windows (in slot)
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_conn_scan_param(UINT16 interval, UINT16 window)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("BLE Conn Scan Interval:%d (%dms) window:%d (%dms)",
            interval, (int)((float)interval*0.625),
            window, (int)((float)window*0.625));

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_CONN_SCAN_PARAM_MASK;
    bt_config.ble_conn_scan_param.interval = interval;
    bt_config.ble_conn_scan_param.window = window;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_adv_data
 **
 ** Description     Configure the BLE advertisng data
 **
 ** Parameters      pointer to tBSA_DM_BLE_ADV_CONFIG
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_adv_data(tBSA_DM_BLE_ADV_CONFIG *p_data)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_ADV_CONFIG_MASK;

    bt_config.adv_config.len = p_data->len;
    bt_config.adv_config.flag = p_data->flag;
    bt_config.adv_config.adv_data_mask = p_data->adv_data_mask;
    bt_config.adv_config.appearance_data = p_data->appearance_data;
    bt_config.adv_config.num_service = p_data->num_service;
    bt_config.adv_config.is_scan_rsp = p_data->is_scan_rsp;

    memcpy(bt_config.adv_config.uuid_val, p_data->uuid_val,p_data->num_service * sizeof(UINT16));
    memcpy(bt_config.adv_config.p_val,p_data->p_val,p_data->len);

    /*for DataType Service Data - 0x16*/
    bt_config.adv_config.service_data_len = p_data->service_data_len;
    bt_config.adv_config.service_data_uuid.len = p_data->service_data_uuid.len;
    bt_config.adv_config.service_data_uuid.uu.uuid16 = p_data->service_data_uuid.uu.uuid16;
    memcpy(bt_config.adv_config.service_data_val,p_data->service_data_val,p_data->service_data_len);

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_adv_param
 **
 ** Description     Configure the BLE Prefer Advertisement parameters
 **
 ** Parameters      pointer to tBSA_DM_BLE_ADV_PARAM
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_adv_param(tBSA_DM_BLE_ADV_PARAM *p_req)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("BDA:%02X:%02X:%02X:%02X:%02X:%02X  ",
            p_req->dir_bda.bd_addr[0], p_req->dir_bda.bd_addr[1],
            p_req->dir_bda.bd_addr[2], p_req->dir_bda.bd_addr[3],
            p_req->dir_bda.bd_addr[4], p_req->dir_bda.bd_addr[5]);

    APP_DEBUG1("adv_int_min:%d adv_int_max:%d", p_req->adv_int_min, p_req->adv_int_max);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Configure the Inquiry Scan parameters */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_ADV_PARAM_MASK;
    bdcpy(bt_config.ble_adv_param.dir_bda.bd_addr, p_req->dir_bda.bd_addr);
    bt_config.ble_adv_param.adv_int_min = p_req->adv_int_min;
    bt_config.ble_adv_param.adv_int_max = p_req->adv_int_max;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_device_class
 **
 ** Description     Set the Device Class
 **
 ** Parameters      services: Service Class
 **                 major: Major Class
 **                 minor: Minor Class
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_device_class(UINT16 services, UINT8 major, UINT8 minor)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set COD Services:%x Major:%x Minor:%x", services, major, minor);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set COD (to allow 3DG to perform Proximity Pairing */
    bt_config.config_mask = BSA_DM_CONFIG_DEV_CLASS_MASK;
    FIELDS_TO_COD(bt_config.class_of_device, minor, major, services);

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_visibility
 **
 ** Description     Set the Device Visibility parameters (InquiryScan & PageScan)
 **
 ** Parameters      discoverable: FALSE if not discoverable
 **                 connectable: FALSE if not connectable
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_visibility(BOOLEAN discoverable, BOOLEAN connectable)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set Visibility Discoverable:%d Connectable:%d", discoverable, connectable);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set visibility configuration */
    bt_config.config_mask = BSA_DM_CONFIG_VISIBILITY_MASK;
    bt_config.connectable = connectable;
    bt_config.discoverable = discoverable;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
/*******************************************************************************
 **
 ** Function        app_dm_set_ble_visibility
 **
 ** Description     Set the Device BLE Visibility parameters
 **
 ** Parameters      discoverable: FALSE if not discoverable
 **                 connectable: FALSE if not connectable
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_visibility(BOOLEAN discoverable, BOOLEAN connectable)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set BLE Visibility Discoverable:%d Connectable:%d", discoverable, connectable);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set visibility configuration */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_VISIBILITY_MASK;
    bt_config.ble_connectable = connectable;
    bt_config.ble_discoverable = discoverable;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_set_ble_local_privacy
 **
 ** Description     Set the Device BLE privacy on the local device
 **
 ** Parameters      privacy_enable: enable/disable BLE local privacy
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_local_privacy(BOOLEAN privacy_enable)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set BLE Local Privacy : %d",privacy_enable);

    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set BLE local privacy*/
    bt_config.config_mask = BSA_DM_CONFIG_BLE_PRIVACY_MASK;
    bt_config.privacy_enable = privacy_enable;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}
#endif

/*******************************************************************************
 **
 ** Function        app_dm_set_dual_stack_mode
 **
 ** Description     Switch Stack
 **
 ** Parameters      Switch Stack mode
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_dual_stack_mode(tBSA_DM_DUAL_STACK_MODE dual_stack_mode)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set DualStackMode:%d ", dual_stack_mode);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set visibility configuration */
    bt_config.config_mask = BSA_DM_CONFIG_DUAL_STACK_MODE_MASK;
    bt_config.dual_stack_mode = dual_stack_mode;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_get_dual_stack_mode
 **
 ** Description     Get current Switch Stack mode
 **
 ** Parameters      none
 **
 ** Returns         Switch Stack mode if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_get_dual_stack_mode(void)
{
    tBSA_DM_GET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG0("Get DualStackMode");

    /* Set Bluetooth configuration */
    BSA_DmGetConfigInit(&bt_config);

    bsa_status = BSA_DmGetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmGetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return (int)bt_config.dual_stack_mode;
}


/*******************************************************************************
 **
 ** Function        app_dm_get_chip_id
 **
 ** Description     Get BT Chip ID
 **
 ** Parameters      none
 **
 ** Returns         Chip ID if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_get_chip_id(void)
{
    tBSA_DM_GET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG0("Get BT Chip ID");

    /* Set Bluetooth configuration */
    BSA_DmGetConfigInit(&bt_config);

    bsa_status = BSA_DmGetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmGetConfig failed status:%d ", bsa_status);
        return(-1);
    }

    switch (bt_config.chip_id)
    {
    case BSA_DM_CHIP_ID_2046B1:
        APP_INFO1("ChipId:BCM2046B1:%d",bt_config.chip_id);
        break;

    case BSA_DM_CHIP_ID_20702A1:
        APP_INFO1("ChipId:BCM20702A1:%d",bt_config.chip_id);
        break;

    case BSA_DM_CHIP_ID_20702B0:
        APP_INFO1("ChipId:BCM20702B0:%d",bt_config.chip_id);
        break;

    case BSA_DM_CHIP_ID_43242A1:
        APP_INFO1("ChipId:BCM43242A1:%d",bt_config.chip_id);
        break;

    case BSA_DM_CHIP_ID_43569A2:
        APP_INFO1("ChipId:BCM43569A2:%d",bt_config.chip_id);
        break;

    default:
        APP_INFO1("Unsupported ChipId:%d", bt_config.chip_id);
        break;
    }
    return (int)bt_config.chip_id;
}


/*******************************************************************************
 **
 ** Function        app_dm_set_ble_bg_conn_type
 **
 ** Description     Configure BLE Background Connection type
 **                 to make automatic connection to BLE device
 **
 ** Parameters      type : 0 - None, 1 - Auto
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_set_ble_bg_conn_type(int type)
{
    tBSA_DM_GET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("Set BLE BG conn type (%d)", type);

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set BLE background connection type */
    bt_config.config_mask = BSA_DM_CONFIG_BLE_BGCONN_TYPE_MASK;
    bt_config.ble_bg_conn_type = type;

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}

/*******************************************************************************
 **
 ** Function        app_dm_monitor_rssi
 **
 ** Description     Enables/Disables periodic monitoring of the RSSI
 **                 for all connected devices
 **
 ** Parameters      enable : 1 - Enable, 0 - Disable
 **                 period : Measurement period in seconds
 **
 ** Returns         status: 0 if success / -1 otherwise
 **
 *******************************************************************************/
int app_dm_monitor_rssi(BOOLEAN enable, UINT16 period)
{
    tBSA_DM_SET_CONFIG bt_config;
    tBSA_STATUS bsa_status;

    APP_DEBUG1("%s RSSI Monitoring\n", (enable)?"Enabling":"Disabling" );

    /* Set Bluetooth configuration */
    BSA_DmSetConfigInit(&bt_config);

    /* Obviously */
    bt_config.enable = TRUE;

    /* Set visibility configuration */
    bt_config.config_mask = BSA_DM_CONFIG_MONITOR_RSSI;

    /* Turn On or Off RSSI Monitoring */
    bt_config.monitor_rssi_param.enable = enable;

    if ( bt_config.monitor_rssi_param.enable )
    {
        bt_config.monitor_rssi_param.period = period;
    }

    bsa_status = BSA_DmSetConfig(&bt_config);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("BSA_DmSetConfig failed status:%d ", bsa_status);
        return(-1);
    }
    return 0;
}


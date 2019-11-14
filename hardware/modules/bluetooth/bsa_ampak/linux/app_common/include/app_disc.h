/*****************************************************************************
**
**  Name:           app_disc.h
**
**  Description:    Bluetooth Application Discovery functions
**
**  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_DISC_H
#define APP_DISC_H

#include <bsa_rokid/bsa_disc_api.h>

/* Vid/Pid Discovery callback */
typedef void (tAPP_DISC_VIP_PID_CBACK)(BD_ADDR bd_addr, UINT16 vid, UINT16 pid);

#ifndef APP_DISC_NB_DEVICES
#define APP_DISC_NB_DEVICES 30
#endif

typedef struct
{
    tBSA_DISC_DEV devs[APP_DISC_NB_DEVICES];
} tAPP_DISCOVERY_CB;

/*
 * Globals
 */

#if 0
extern tAPP_DISCOVERY_CB app_discovery_cb;

/*******************************************************************************
 **
 ** Function         app_disc_display_devices
 **
 ** Description      This function is used to list discovered devices
 **
 ** Returns          int
 **
 *******************************************************************************/
void app_disc_display_devices(void);

/*******************************************************************************
 **
 ** Function         app_disc_cback
 **
 ** Description      Discovery callback
 **
 ** Returns          int
 **
 *******************************************************************************/
void app_disc_cback(tBSA_DISC_EVT event, tBSA_DISC_MSG *p_data);

/*******************************************************************************
 **
 ** Function         app_disc_start_regular
 **
 ** Description      Start Device discovery
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_regular(tBSA_DISC_CBACK *p_custom_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_start_ble_regular
 **
 ** Description      Start BLE Device discovery
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_ble_regular(tBSA_DISC_CBACK *p_custom_disc_cback);


/*******************************************************************************
 **
 ** Function         app_disc_start_services
 **
 ** Description      Start Device service discovery
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_services(tBSA_SERVICE_MASK services);

/*******************************************************************************
 **
 ** Function         app_disc_start_cod
 **
 ** Returns          int
 **
 ** Returns          void
 **
 *******************************************************************************/
int app_disc_start_cod(unsigned short services, unsigned char major, unsigned char minor,
        tBSA_DISC_CBACK *p_custom_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_abort
 **
 ** Description      Abort Device discovery
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_abort(void);

/*******************************************************************************
 **
 ** Function         app_disc_start_dev_info
 **
 ** Description      Start the discovery of the device info of a device
 **
 ** Parameters       bd_addr BD address of the device to get the ID from
 **                  p_callback invoked upon discovery completion
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_dev_info(BD_ADDR bd_addr, tBSA_DISC_CBACK *p_custom_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_start_brcm_filter_cod
 **
 ** Description      Start Device discovery based on Brcm and COD filters
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_brcm_filter_cod(tBSA_DISC_BRCM_FILTER brcm_filter,
        unsigned short services, unsigned char major, unsigned char minor,
        tBSA_DISC_CBACK *p_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_start_bdaddr
 **
 ** Description      Start Device discovery based on BdAddr
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_bdaddr(BD_ADDR bd_addr, BOOLEAN name_req,
        tBSA_DISC_CBACK *p_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_start_bdaddr_services
 **
 ** Description      Start Service discovery based on BdAddr
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_bdaddr_services(BD_ADDR bd_addr,
        tBSA_SERVICE_MASK services, tBSA_DISC_CBACK *p_disc_cback);

/*******************************************************************************
 **
 ** Function         app_disc_start_update
 **
 ** Description      Start Device discovery with update parameter
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_update(tBSA_DISC_UPDATE update);

/*******************************************************************************
 **
 ** Function         app_disc_start_limited
 **
 ** Description      Start Device Limited discovery
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_start_limited(void);

/*******************************************************************************
 **
 ** Function         app_disc_start_power
 **
 ** Description      Start discovery with specific inquiry TX power
 **
 ** Returns          0 if successful, error code otherwise
 **
 *******************************************************************************/
int app_disc_start_power(INT8 inq_tx_power);

/*******************************************************************************
 **
 ** Function         app_disc_find_device
 **
 ** Description      Find a device in the list of discovered devices
 **
 ** Parameters       bda: BD address of the searched device
 **
 ** Returns          Pointer to the discovered device, NULL if not found
 **
 *******************************************************************************/
tBSA_DISC_DEV *app_disc_find_device(BD_ADDR bda);

/*******************************************************************************
 **
 ** Function         app_disc_set_nb_devices
 **
 ** Description      Set the maximum number of returned discovered devices
 **
 ** Parameters       nb_devices : maximum number of returned devices
 **
 ** Returns          0 if successful, -1 in case of error
 **
 *******************************************************************************/
int app_disc_set_nb_devices(int nb_devices);

#endif
/*******************************************************************************
 **
 ** Function         app_disc_parse_eir
 **
 ** Description      This function is used to parse EIR data
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_disc_parse_eir(UINT8 *p_eir);

/*******************************************************************************
 **
 ** Function         app_disc_read_remote_device_name
 **
 ** Description      Start Read Remote Device Name
 **
 ** Returns          int
 **
 *******************************************************************************/
int app_disc_read_remote_device_name(BD_ADDR bd_addr,tBSA_TRANSPORT transport);

char *app_disc_address_type_desc(UINT8 device_type);
char *app_disc_inquiry_type_desc(UINT8 device_type);
char *app_disc_device_type_desc(UINT8 device_type);
#endif

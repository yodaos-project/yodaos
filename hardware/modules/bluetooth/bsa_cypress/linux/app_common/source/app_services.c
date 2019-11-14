/*****************************************************************************
 **
 **  Name:           app_services.c
 **
 **  Description:    Bluetooth Services functions
 **
 **  Copyright (c) 2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bsa_rokid/bsa_api.h>

#include "app_services.h"

/*
 * Global variables
 */
static char services_string[BSA_MAX_SERVICE_ID * 7];

/*******************************************************************************
 **
 ** Function         app_service_id_to_string
 **
 ** Description      This function is used to convert a service ID to a string
 **
 ** Parameters
 **
 ** Returns          Pointer to string containing the service in human format
 **
 *******************************************************************************/
const char *app_service_id_to_string(tBSA_SERVICE_ID serviceId)
{
    switch(serviceId)
    {
    case BSA_RES_SERVICE_ID:
        return "Reserved !!!";
    case BSA_SPP_SERVICE_ID:
        return "SPP";
    case BSA_DUN_SERVICE_ID:
        return "DUN";
    case BSA_A2DP_SRC_SERVICE_ID:
        return "A2DP_SRC";
    case BSA_AVRCP_TG_SERVICE_ID:
        return "AVRCP_TG";
    case BSA_HSP_SERVICE_ID:
        return "HSP";
    case BSA_HFP_SERVICE_ID:
        return "HFP";
    case BSA_OPP_SERVICE_ID:
        return "OPP";
    case BSA_FTP_SERVICE_ID:
        return "FTP";
    case BSA_SYNC_SERVICE_ID:
        return "SYNC";
    case BSA_BPP_SERVICE_ID:
        return "BPP";
    case BSA_BIP_SERVICE_ID:
        return "BIP";
    case BSA_PANU_SERVICE_ID:
        return "PANU";
    case BSA_NAP_SERVICE_ID:
        return "NAP";
    case BSA_GN_SERVICE_ID:
        return "GN";
    case BSA_SAP_SERVICE_ID:
        return "SAP";
    case BSA_A2DP_SERVICE_ID:
        return "A2DP";
    case BSA_AVRCP_SERVICE_ID:
        return "AVRCP";
    case BSA_HID_SERVICE_ID:
        return "HID_HOST";
    case BSA_HID_DEVICE_SERVICE_ID:
        return "HID_DEVICE";
    case BSA_VDP_SERVICE_ID:
        return "VDP";
    case BSA_PBAP_SERVICE_ID:
        return "PBAP";
    case BSA_HFP_HS_SERVICE_ID:
        return "HFP_HS";
    case BSA_HSP_HS_SERVICE_ID:
        return "HSP_HS";
    case BSA_MAP_SERVICE_ID:
        return "MAP";
    case BSA_HL_SERVICE_ID:
        return "HL";
#if BLE_INCLUDED == TRUE 
    case BSA_BLE_SERVICE_ID:
        return "BLE";
#endif
    default:
        break;
    }
    return "BAD Service !!!";
}

/*******************************************************************************
 **
 ** Function         app_service_mask_to_string
 **
 ** Description      This function is used to convert a service ID Mask to a string
 **
 ** Parameters
 **
 ** Returns          Pointer to string containing the services in human format
 **
 *******************************************************************************/
char *app_service_mask_to_string(tBSA_SERVICE_MASK serviceMask)
{
    /* Clear the entire string */
    memset(services_string, 0, sizeof(services_string));

    if (serviceMask & BSA_RES_SERVICE_MASK)
        strncat(services_string, "RESERVED!!! ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_SPP_SERVICE_MASK)
        strncat(services_string, "SPP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_DUN_SERVICE_MASK)
        strncat(services_string, "DUN ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_A2DP_SRC_SERVICE_MASK)
        strncat(services_string, "A2DP_SRC ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_AVRCP_TG_SERVICE_MASK)
        strncat(services_string, "AVRCP_TG ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HSP_SERVICE_MASK)
        strncat(services_string, "HSP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HFP_SERVICE_MASK)
        strncat(services_string, "HFP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_OPP_SERVICE_MASK)
        strncat(services_string, "OPP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_FTP_SERVICE_MASK)
        strncat(services_string, "FTP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_SYNC_SERVICE_MASK)
        strncat(services_string, "SYNC ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_BPP_SERVICE_MASK)
        strncat(services_string, "BPP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_BIP_SERVICE_MASK)
        strncat(services_string, "BIP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_PANU_SERVICE_MASK)
        strncat(services_string, "PANU ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_NAP_SERVICE_MASK)
        strncat(services_string, "NAP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_GN_SERVICE_MASK)
        strncat(services_string, "GN ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_SAP_SERVICE_MASK)
        strncat(services_string, "SAP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_A2DP_SERVICE_MASK)
        strncat(services_string, "A2DP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_AVRCP_SERVICE_MASK)
        strncat(services_string, "AVRCP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HID_SERVICE_MASK)
        strncat(services_string, "HID ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HID_DEVICE_SERVICE_MASK)
        strncat(services_string, "HID_DEVICE ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_VDP_SERVICE_MASK)
        strncat(services_string, "VDP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_PBAP_SERVICE_MASK)
        strncat(services_string, "PBAP ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HSP_HS_SERVICE_MASK)
        strncat(services_string, "HSP_HS ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HFP_HS_SERVICE_MASK)
        strncat(services_string, "HFP_HS ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_MAS_SERVICE_MASK)
        strncat(services_string, "MAS ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_MN_SERVICE_MASK)
        strncat(services_string, "NN ", sizeof(services_string) - strlen(services_string) - 1);
    if (serviceMask & BSA_HL_SERVICE_MASK)
        strncat(services_string, "HL ", sizeof(services_string) - strlen(services_string) - 1);
#if (defined(BLE_INCLUDED) && BLE_INCLUDED == TRUE)
    if (serviceMask & BSA_BLE_SERVICE_MASK)
        strncat(services_string, "BLE ", sizeof(services_string) - strlen(services_string) - 1);
#endif
    return services_string;
}


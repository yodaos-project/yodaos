/*****************************************************************************
 **
 **  Name:           app_services.h
 **
 **  Description:    Bluetooth Services functions
 **
 **  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/


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
const char *app_service_id_to_string(tBSA_SERVICE_ID serviceId);

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
char *app_service_mask_to_string(tBSA_SERVICE_MASK serviceMask);


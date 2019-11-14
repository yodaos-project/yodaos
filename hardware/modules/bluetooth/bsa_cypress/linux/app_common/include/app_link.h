/*****************************************************************************
 **
 **  Name:           app_link.h
 **
 **  Description:    Bluetooth Link activity debug functions
 **
 **  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_LINK_H
#define APP_LINK_H

/* for types */
#include <bsa_rokid/bt_target.h>

/*******************************************************************************
 **
 ** Function         app_link_find_by_cod
 **
 ** Description      This function searches for a specific class of device
 **
 ** Parameters       p_cod: class of device to look for
 **                  p_cod_mask: mask of the class of device to apply for check
 **
 ** Returns          -1 if error, 1 if matching device found, 0 if none found
 **
 *******************************************************************************/
int app_link_find_by_cod(const UINT8 *p_cod, const UINT8 *p_cod_mask);

/*******************************************************************************
 **
 ** Function         app_link_add
 **
 ** Description      This function is used to add an ACL link in the database
 **
 ** Parameters       bd_addr: BD address of the device to add in the database
 **                  p_class_of_device: pointer to its class of device
 **
 ** Returns          status (0 if successful, negative error code otherwise)
 **
 *******************************************************************************/
int app_link_add(BD_ADDR bd_addr, UINT8 *p_class_of_device);

/*******************************************************************************
 **
 ** Function         app_link_remove
 **
 ** Description      This function is used to remove an ACL link in the database
 **
 ** Parameters       bd_addr: BD address of the device to remove from database
 **
 ** Returns          status (0 if successful, negative error code otherwise)
 **
 *******************************************************************************/
int app_link_remove(BD_ADDR bd_addr);

/*******************************************************************************
 **
 ** Function         app_link_display
 **
 ** Description      This function is used to display ACL link(s) in the database
 **
 ** Parameters
 **
 ** Returns          void
 **
 *******************************************************************************/
void app_link_display(void);

/*******************************************************************************
 **
 ** Function         app_link_get_cod
 **
 ** Description      This function is used to retrieve the COD of an ACL link
 **                  from the database
 **
 ** Parameters       bd_addr: BD address of the device to retrieve the COD of
 **                  p_class_of_device: returned COD if successful
 **
 ** Returns          status (0 if successful, negative error code otherwise)
 **
 *******************************************************************************/
int app_link_get_cod(BD_ADDR bd_addr, UINT8 *class_of_device);

/*******************************************************************************
 **
 ** Function         app_link_set_suspended
 **
 ** Description      Indicate if ACL link is suspended
 **
 ** Parameters       bd_addr: BD address of the device
 **                  suspended: whether link i9s suspended or not
 **
 ** Returns          status (0 if successful, negative error code otherwise)
 **
 *******************************************************************************/
int app_link_set_suspended(BD_ADDR bd_addr, BOOLEAN suspended);

/*******************************************************************************
 **
 ** Function         app_link_get_suspended
 **
 ** Description      Check if ACL link is suspended
 **
 ** Parameters       bd_addr: BD address of the device
 **
 ** Returns          status (0 if not suspended, 1 if suspended, negative error code otherwise)
 **
 *******************************************************************************/
int app_link_get_suspended(BD_ADDR bd_addr);

/*******************************************************************************
 **
 ** Function        app_link_get_bd_addr_from_index
 **
 ** Description     Get BdAddr from Device Index
 **
 ** Parameters      index: Device Index
 **                 bd_addr: BD address of the device
 **
 ** Returns         status (0 if device found, -1 if device not connected)
 **
 *******************************************************************************/
int app_link_get_bd_addr_from_index(int index, BD_ADDR bd_addr);

/*******************************************************************************
 **
 ** Function        app_link_get_num_active
 **
 ** Description     Get the number of active links
 **
 ** Parameters      
 **
 ** Returns         status (-1 if error, number of active links otherwise)
 **
 *******************************************************************************/
int app_link_get_num_active(void);

#endif


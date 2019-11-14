/*****************************************************************************
 **
 **  Name:           app_link.c
 **
 **  Description:    Bluetooth Link activity debug functions
 **
 **  Copyright (c) 2010-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include "app_link.h"
#include "app_utils.h"
#include <bsa_rokid/bd.h>
/*
 * Maximum number of ACL links currently established.
 * Bluetooth allow up to 7 slaves connected, but the Broadcom's Baseband
 * may support three more slaves connected
 */
#define APP_LINK_NB_MAX 10

typedef struct
{
    BOOLEAN in_use;
    DEV_CLASS class_of_device;
    BD_ADDR bd_addr;
    BOOLEAN suspended;
} tAPP_ACL_LINK;

/*
 * Globals
 */
tAPP_ACL_LINK app_acl_link[APP_LINK_NB_MAX];


/* Local functions */
/*******************************************************************************
 **
 ** Function         app_link_find_by_addr
 **
 ** Description      This function searches for an existing link in database
 **
 ** Parameters       bd_addr: BD address of the device to look for in database
 **
 ** Returns          NULL if not found, otherwise pointer to device structure
 **
 *******************************************************************************/
static tAPP_ACL_LINK *app_link_find_by_addr(const BD_ADDR bd_addr)
{
    int index;

    /* Check if this link is already established */
    for (index = 0; index < APP_LINK_NB_MAX; index++)
    {
        if ((app_acl_link[index].in_use) &&
            (bdcmp(app_acl_link[index].bd_addr, bd_addr) == 0))
        {
            return &app_acl_link[index];
        }
    }
    return NULL;
}

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
int app_link_find_by_cod(const UINT8 *p_cod, const UINT8 *p_cod_mask)
{
    int index;

    /* Check if this link is already established */
    for (index = 0; index < APP_LINK_NB_MAX; index++)
    {
        if ((app_acl_link[index].in_use) &&
            ((app_acl_link[index].class_of_device[0] & p_cod_mask[0]) == p_cod[0]) &&
            ((app_acl_link[index].class_of_device[1] & p_cod_mask[1]) == p_cod[1]) &&
            ((app_acl_link[index].class_of_device[2] & p_cod_mask[2]) == p_cod[2]))
        {
            return 1;
        }
    }
    return 0;
}

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
int app_link_add(BD_ADDR bd_addr, UINT8 *p_class_of_device)
{
    int index;

    APP_DEBUG0("Adding device in ACL database");

    /* First check if this link is established */
    if (app_link_find_by_addr(bd_addr) != NULL)
    {
        APP_ERROR0("This device is already in the database");
        return -1;
    }

    /* Look for a free entry */
    for (index = 0; index < APP_LINK_NB_MAX; index++)
    {
        if (app_acl_link[index].in_use == FALSE)
        {
            break;
        }
    }
    if (index >= APP_LINK_NB_MAX)
    {
        APP_ERROR0("Database full");
        return -1;
    }

    /* This device was not in the database and we found a free entry */
    /* Let's update it */
    app_acl_link[index].in_use = TRUE;
    app_acl_link[index].suspended = FALSE;
    bdcpy(app_acl_link[index].bd_addr, bd_addr);
    app_acl_link[index].class_of_device[0] = p_class_of_device[0];
    app_acl_link[index].class_of_device[1] = p_class_of_device[1];
    app_acl_link[index].class_of_device[2] = p_class_of_device[2];

    app_link_display();

    return 0;
}

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
int app_link_remove(BD_ADDR bd_addr)
{
    tAPP_ACL_LINK *p_link;

    APP_DEBUG0("Removing device from ACL database");

    p_link = app_link_find_by_addr(bd_addr);

    /* First check if this link is established */
    if (p_link == NULL)
    {
        APP_ERROR0("This device is not in the database");
        return -1;
    }

    APP_DEBUG1("Suppressing device index: %d", (int)(p_link - app_acl_link));
    p_link->in_use = FALSE;
    app_link_display();

    return 0;
}

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
void app_link_display(void)
{
    int index;
    BOOLEAN connected = FALSE;

    APP_DEBUG0("Currently connected devices");

    /* Check if this link is already established */
    for (index = 0 ; index < APP_LINK_NB_MAX ; index++)
    {
        if (app_acl_link[index].in_use)
        {
            APP_DEBUG1("Device index:[%d] BdAddr: %02X:%02X:%02X:%02X:%02X:%02X COD: %02X:%02X:%02X => %s",
                    index,
                    app_acl_link[index].bd_addr[0],app_acl_link[index].bd_addr[1],
                    app_acl_link[index].bd_addr[2],app_acl_link[index].bd_addr[3],
                    app_acl_link[index].bd_addr[4],app_acl_link[index].bd_addr[5],
                    app_acl_link[index].class_of_device[0],
                    app_acl_link[index].class_of_device[1],
                    app_acl_link[index].class_of_device[2],
                    app_get_cod_string(app_acl_link[index].class_of_device) );

            connected = TRUE;
        }
    }

    if (connected == FALSE)
    {
        APP_DEBUG0("No device connected");
    }
}

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
int app_link_get_cod(BD_ADDR bd_addr, UINT8 *p_class_of_device)
{
    tAPP_ACL_LINK *p_link;

    APP_DEBUG0("Get COD from ACL database");
    p_link = app_link_find_by_addr(bd_addr);

    /* First check if this link is established */
    if (p_link == NULL)
    {
        APP_DEBUG0("This device is not in the database");
        p_class_of_device[0] = 0;
        p_class_of_device[1] = 0;
        p_class_of_device[2] = 0;
        app_link_display();
        return -1;
    }

    APP_DEBUG1("Device found index:[%d] COD:%02X:%02X:%02X => %s",
            (int)(p_link - app_acl_link),
            p_link->class_of_device[0],
            p_link->class_of_device[1],
            p_link->class_of_device[2],
            app_get_cod_string(p_link->class_of_device));

    p_class_of_device[0] = p_link->class_of_device[0];
    p_class_of_device[1] = p_link->class_of_device[1];
    p_class_of_device[2] = p_link->class_of_device[2];

    return 0;
}

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
int app_link_set_suspended(BD_ADDR bd_addr, BOOLEAN suspended)
{
    tAPP_ACL_LINK *p_link;

    APP_DEBUG1("Set suspended: %d", (int)suspended);

    p_link = app_link_find_by_addr(bd_addr);

    /* First check if this link is established */
    if (p_link == NULL)
    {
        APP_DEBUG0("This device is not in the database");
        return -1;
    }

    p_link->suspended = suspended;
    return 0;
}

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
int app_link_get_suspended(BD_ADDR bd_addr)
{
    tAPP_ACL_LINK *p_link;

    APP_DEBUG0("Get suspended");

    p_link = app_link_find_by_addr(bd_addr);

    /* First check if this link is established */
    if (p_link == NULL)
    {
        APP_DEBUG0("This device is not in the database");
        return -1;
    }

    if (p_link->suspended)
    {
        return 1;
    }
    return 0;
}

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
int app_link_get_bd_addr_from_index(int index, BD_ADDR bd_addr)
{
    tAPP_ACL_LINK *p_link;

    APP_DEBUG0("Get BdAddr");

    if (bd_addr == NULL)
    {
        APP_ERROR0("Null BdAddr pointer");
        return -1;
    }

    if ((index < 0) || (index >= APP_LINK_NB_MAX))
    {
        APP_ERROR1("Bad Index:%d", index);
        return -1;
    }

    p_link = &app_acl_link[index];

    if (p_link->in_use == FALSE)
    {
        APP_ERROR1("Device Index:%d is not connected", index);
        return -1;
    }

    bdcpy(bd_addr, p_link->bd_addr);
    APP_DEBUG1("BdAddr %02X:%02X:%02X:%02X:%02X:%02X",
            bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

    return 0;
}

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
int app_link_get_num_active(void)
{
    int index, count;

    count = 0;
    /* Count the number of active links */
    for (index = 0; index < APP_LINK_NB_MAX; index++)
    {
        if (app_acl_link[index].in_use)
        {
            count++;
        }
    }
    return count;
}



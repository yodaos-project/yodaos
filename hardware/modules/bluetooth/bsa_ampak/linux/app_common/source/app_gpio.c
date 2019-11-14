/*****************************************************************************
 **
 **  Name:           app_gpio.c
 **
 **  Description:    Bluetooth GPIO functions
 **
 **  Copyright (c) 2010, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bsa_rokid/bsa_api.h>
#include "bsa_trace.h"

#include "app_utils.h"


/*******************************************************************************
 **
 ** Function         app_gpio_read
 **
 ** Description      This function reads a GPIO
 **
 ** Parameters
 **
 ** Returns          GPIO value (0 or 1) or -1 if error
 **
 *******************************************************************************/
int app_gpio_read(UINT8 gpio_number)
{
    tBSA_TM_VSC bsa_vsc;
    tBSA_STATUS bsa_status;

    if (gpio_number == 0xff)
    {
        gpio_number = app_get_choice("Enter GPIO Number");
    }

    /* Prepare VSC Parameters */
    bsa_status = BSA_TmVscInit(&bsa_vsc);
    bsa_vsc.opcode = 0x001A;             /* GPIO_Read */
    bsa_vsc.length = 1; /* 1 byte parameter */
    bsa_vsc.data[0] = gpio_number;
    bsa_status = BSA_TmVsc(&bsa_vsc);
    if (bsa_status != BSA_SUCCESS)
    {
        APP_ERROR1("Unable to Send VSC status:%d", bsa_status);
        return(-1);
    }

    APP_DEBUG1("VSC Server status:%d", bsa_vsc.status);
    APP_DEBUG1("VSC opcode:0x%03X", bsa_vsc.opcode);
    APP_DEBUG1("VSC length:%d", bsa_vsc.length);
    APP_DEBUG1("VSC Chip/HCI status:0x%02X", bsa_vsc.data[0]);
#ifndef APP_TRACE_NODEBUG
    /* Dump the Received buffer */
    if ((bsa_vsc.length - 1) > 0)
    {
        scru_dump_hex(&bsa_vsc.data[1], "VSC Data", bsa_vsc.length - 1, TRACE_LAYER_NONE,
                TRACE_TYPE_DEBUG);
    }
#endif
    if (bsa_vsc.status != 0)
    {
        APP_ERROR0("Bad HCI Return code");
        return (-1);
    }
    if (bsa_vsc.data[1] != gpio_number)
    {
        APP_ERROR1("Bad GPIO number returned:%d", bsa_vsc.data[1]);
        return (-1);
    }
    APP_INFO1("GPIO Value:%d", bsa_vsc.data[2]);

    return (bsa_vsc.data[2]);
}


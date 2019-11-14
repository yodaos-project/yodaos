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

#ifndef APP_GPIO_H
#define APP_GPIO_H

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
int app_gpio_read(UINT8 gpio_number);

#endif


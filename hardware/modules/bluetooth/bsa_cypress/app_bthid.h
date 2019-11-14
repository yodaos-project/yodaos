/*****************************************************************************
**
**  Name:           app_bthid.h
**
**  Description:    This file contains the functions implementation
**                  to pass HID reports/descriptor received from peer HID
**                  devices to the Linux kernel (via bthid module)
**
**  Copyright (c) 2010-2013, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef _APP_BTHID_H_
#define _APP_BTHID_H_

/*******************************************************************************
 **
 ** Function         app_bthid_open
 **
 ** Description      Open BTHID driver
 **
 ** Returns          BTHID File descriptor
 **
 *******************************************************************************/
int app_bthid_open(void);

/*******************************************************************************
 **
 ** Function         app_bthid_close
 **
 ** Description      This function is executed by app HH when connection is closed
 **
 ** Parameters       BTHID file descriptor
 **
 ** Returns          status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_bthid_close(int bthid_fd);

/******************************************************************************
 **
 ** Function:    app_bthid_report_write
 **
 ** Description: Send Bluetooth HID report to bthid driver for processing
 **              bthid expects that application has sent HID DSCP to driver
 **              this tells Linux system how to parse the reports
 **              The bthid.ko driver will send the report to the Linux kernel
 **
 ** Returns:     Number of bytes of the report written (negative if error)
 **
 *******************************************************************************/
int app_bthid_report_write(int fd, unsigned char *p_rpt, int size);

/******************************************************************************
 **
 ** Function:    app_bthid_config
 **
 ** Description: Configure the bthid driver instance
 **
 ** Returns:     status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_bthid_config(int fd, const char *name, unsigned short vid, unsigned short pid,
        unsigned short version, unsigned short country);

/******************************************************************************
 **
 ** Function:    app_bthid_desc_write
 **
 ** Description: Send Bluetooth HID Descriptor to bthid driver for processing
 **
 ** Returns:     status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_bthid_desc_write(int fd, unsigned char *p_dscp, unsigned int size);

#endif


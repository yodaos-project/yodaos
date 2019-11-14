/*****************************************************************************
**
**  Name:           app_bthid.c
**
**  Description:    This file contains the functions implementation
**                  to pass HID reports/descriptor received from peer HID
**                  devices to the Linux kernel (via bthid module)
**
**  Copyright (c) 2010-2013, Broadcom, All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#include <stdio.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/uinput.h>

#include <bsa_rokid/bt_types.h>
#include "app_utils.h"
#include "app_bthid.h"

#include "bthid.h"

#define APP_HH_BTHID_PATH "/dev/bthid"


/*******************************************************************************
 **
 ** Function         app_bthid_open
 **
 ** Description      Open BTHID driver
 **
 ** Returns          BTHID File descriptor
 **
 *******************************************************************************/
int app_bthid_open(void)
{
    int bthid_fd;

    APP_DEBUG0("");

    /****************************************************
     * Before opening bthid device you need to make
     * sure the device entry is created under /dev/input
     * and that the bthid device driver is loaded.
     * To do so, you can run an external script to do this for you.
     * The contents of this script could be:
     * >mknod /dev/input/bthid c 10 224
     * >chmod 666 /dev/input/bthid
     * >insmod bthid.ko
     ****************************************************/

    /* Open bthid device.  Return if failed */
    bthid_fd = open(APP_HH_BTHID_PATH, O_RDWR);
    if (bthid_fd < 0)
    {
        APP_ERROR1("Failed to open %s : %d", APP_HH_BTHID_PATH, errno);
        return -1;
    }
    APP_DEBUG1("fd=%d", bthid_fd);

    return bthid_fd;
}

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
int app_bthid_close(int bthid_fd)
{
    int status;

    if (bthid_fd > 0)
    {
        APP_DEBUG1("fd: %d", bthid_fd);
        status = close(bthid_fd);
    }
    else
    {
        APP_ERROR0("NULL bthid file descriptor");
        status = -1;
    }

    /*******************************************
     * After closing bthid, normally it would
     * get unregistered from system.
     * To do so, you can run an external script to do this.
     * The contents of this script could be:
     * rmmod bthid
     * rm -Rf /dev/input/bthid
     *********************************************/

    return status;
}

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
int app_bthid_report_write(int fd, unsigned char *p_rpt, int size)
{
    ssize_t size_wrote;

    size_wrote = write(fd, p_rpt, (size_t)size);

    if (size_wrote < 0)
    {
        APP_ERROR1("unable to write HID report:%d", errno);
    }
    return size_wrote;
}

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
        unsigned short version, unsigned short country)
{
     BTHID_CONFIG config;
     unsigned long arg;
     int status;

     APP_DEBUG1("fd=%d name=%s vid=x%02x pid=x%02x version=%d", fd, name, vid, pid,
             version);

     arg = (unsigned long)&config;
     status = ioctl(fd, BTHID_GET_CONFIG, arg);
     APP_DEBUG1("BTHID_GET_CONFIG -> status=%d", status);
     if (status < 0)
     {
         return status;
     }

     APP_DEBUG1("BTHID_GET_CONFIG name=%s vid=x%02x pid=x%02x version=%d",
             config.name, config.vid, config.pid, config.version);

     /* Prepare ioctl structure */
     if (name)
     {
         strncpy(config.name, name, sizeof(config.name));
         config.name[sizeof(config.name) - 1] = 0;
     }
     else
     {
         config.name[0] = 0;
     }
     config.vid = vid;
     config.pid = pid;
     config.version = version;
     config.country = country;

     status = ioctl(fd, BTHID_SET_CONFIG, arg);
     APP_DEBUG1("BTHID_GET_CONFIG -> status=%d", status);
     if (status < 0)
     {
         return status;
     }

     status = ioctl(fd, BTHID_GET_CONFIG, arg);
     APP_DEBUG1("BTHID_GET_CONFIG -> status=%d", status);
     if (status < 0)
     {
         return status;
     }

     APP_DEBUG1("BTHID_GET_CONFIG name=%s vid=x%02x pid=x%02x version=%d",
             config.name, config.vid, config.pid, config.version);

     return status;
}

/******************************************************************************
 **
 ** Function:    app_bthid_desc_write
 **
 ** Description: Send Bluetooth HID Descriptor to bthid driver for processing
 **
 ** Returns:     status (negative code if error, positive if successful)
 **
 *******************************************************************************/
int app_bthid_desc_write(int fd, unsigned char *p_dscp, unsigned int size)
{
     BTHID_CONTROL ctrl;
     unsigned long arg;
     int status;

     APP_DEBUG1("fd=%d", fd);

     if ((p_dscp == NULL) || (size > sizeof(ctrl.data)))
     {
         APP_ERROR0("bad parameter");
     }
     /* Prepare ioctl structure */
     ctrl.size =  size;
     memcpy(ctrl.data, p_dscp, ctrl.size);

     arg = (unsigned long)&ctrl;
     status = ioctl(fd, BTHID_PARSE_HID_DESC, arg);
     if (status < 0)
     {
         return status;
     }
     return status;
}


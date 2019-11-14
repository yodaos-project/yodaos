/*
*
* bthid.h
*
*
*
* Copyright (C) 2011 Broadcom Corporation.
*
*
*
* This software is licensed under the terms of the GNU General Public License,
* version 2, as published by the Free Software Foundation (the "GPL"), and may
* be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GPL for more details.
*
*
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php
* or by writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA
*
*
*/

#ifndef BTHID_H
#define BTHID_H

// debug information flags. one-hot encoded:
// bit0 : map bthid_dbg to printk(KERN_DEBUG) otherwise compiled out
// bit1 : map bthid_info to printk(KERN_INFO) otherwise compiled out
#define BTHID_DBGFLAGS 1

#if defined(BTHID_DBGFLAGS) && (BTHID_DBGFLAGS & 1)
#define BTHID_DBG(fmt, ...) \
    printk(KERN_DEBUG "BTHID %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define BTHID_DBG(fmt, ...)
#endif

#if defined(BTHID_DBGFLAGS) && (BTHID_DBGFLAGS & 2)
#define BTHID_INFO(fmt, ...) \
    printk(KERN_INFO "BTHID %s: " fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define BTHID_INFO(fmt, ...)
#endif

#define BTHID_ERR(fmt, ...) \
    printk(KERN_ERR "BTHID %s: " fmt, __FUNCTION__, ##__VA_ARGS__)

#define BTHID_MINOR             224
#define BTHID_NAME              "bthid"

typedef struct
{
    int size;
    unsigned char data[800];
} BTHID_CONTROL;

typedef struct
{
    char name[128];
    unsigned short vid;
    unsigned short pid;
    unsigned short version;
    unsigned short country;
} BTHID_CONFIG;

/* ioctl */
#define BTHID_PARSE_HID_DESC    _IOW('u',  1, BTHID_CONTROL)
#define BTHID_SET_CONFIG        _IOW('u',  2, BTHID_CONFIG)
#define BTHID_GET_CONFIG        _IOR('u',  3, BTHID_CONFIG)


#endif /* BTHID_H */

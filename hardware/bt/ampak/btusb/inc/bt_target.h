/****************************************************************************
**
**  Name:       bt_target.h
**
**  Function    this file contains definitions that will probably
**              change for each Bluetooth target system. This includes
**              such things as buffer pool sizes, number of tasks,
**              little endian/big endian conversions, etc...
**
**  NOTE        This file should always be included first.
**
**
**  Copyright (c) 1999-2014, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef BT_TARGET_H
#define BT_TARGET_H

/* Maximum number of simultaneous local AMP controllers in system (BR/EDR excluded) */
#ifndef AMP_MAX_LOCAL_CTRLS
#define AMP_MAX_LOCAL_CTRLS     1
#endif

#ifndef LOCAL_BLE_CONTROLLER_ID
#define LOCAL_BLE_CONTROLLER_ID         (AMP_MAX_LOCAL_CTRLS + 1)
#endif

#endif /* BT_TARGET_H */




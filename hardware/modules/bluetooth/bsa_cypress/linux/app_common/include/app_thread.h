/*****************************************************************************
**
 **  Name:           app_thread.h
 **
 **  Description:    Thread utility functions for applications
 **
 **  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/

#ifndef APP_THREAD_H
#define APP_THREAD_H

#include <pthread.h>

typedef void (APP_THREAD_ENTRY)(void);
typedef pthread_t tAPP_THREAD;


/*******************************************************************************
 **
 ** Function         app_create_thread
 **
 ** Description      Create a thread.
 **
 ** Parameters       task_entry: Thread function
 **                  stack: Pointer to the stack
 **                  stacksize: Size of the stack
 **                  p_thread: Returned tAPP_THREAD
 **
 ** Returns          -1 in case of error, 0 otherwise
 **
 *******************************************************************************/
int app_create_thread(APP_THREAD_ENTRY task_entry, UINT16 *stack, UINT16 stacksize, tAPP_THREAD *p_thread);

/*******************************************************************************
 **
 ** Function         app_stop_thread
 **
 ** Description      Stop a thread.
 **
 ** Parameters       thread: tAPP_THREAD returned by app_create_thread call
 **
 ** Returns          -1 in case of error, 0 otherwise
 **
 *******************************************************************************/
int app_stop_thread(tAPP_THREAD thread);

#endif


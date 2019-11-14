/*****************************************************************************
 **
 **  Name:           app_thread.c
 **
 **  Description:    Thread utility functions for applications
 **
 **  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include <bsa_rokid/bsa_api.h>

#include "app_thread.h"
#include "app_utils.h"

typedef void *(*PTHREAD_ENTRY)(void*);


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
int app_create_thread(APP_THREAD_ENTRY task_entry, UINT16 *stack, UINT16 stacksize, tAPP_THREAD *p_thread)
{
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);

    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(p_thread, &thread_attr, (PTHREAD_ENTRY)task_entry, NULL) < 0 )
    {
        APP_ERROR1("pthread_create failed:%d", errno);
        return -1;
    }
    return 0;
}

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
int app_stop_thread(tAPP_THREAD thread)
{
#if !defined(BSA_ANDROID) || (BSA_ANDROID == FALSE)
    if ((thread != 0) && (pthread_cancel(thread) < 0))
    {
        APP_ERROR1("pthread_cancel failed:%d", errno);
        return -1;
    }
#endif
    return 0;
}


/*****************************************************************************
 **
 **  Name:           app_mutex.h
 **
 **  Description:    Mutual exclusion semaphores utility functions for 
 **                  applications
 **
 **  Copyright (c) 2009-2011, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

#ifndef APP_MUTEX_H
#define APP_MUTEX_H

#if defined(__FreeBSD__)
typedef struct
{
    pthread_mutex_t pthread_mutex;
    pthread_cond_t  pthread_cond;
    int             count;
} tAPP_MUTEX;
#else
typedef pthread_mutex_t tAPP_MUTEX;
#endif

/*******************************************************************************
 **
 ** Function         app_init_mutex
 **
 ** Description      Initialize the mutex.
 **
 ** Parameters       p_mutex: pointer to the application instance
 **
 ** Returns          0 if success, error code otherwise
 **
 *******************************************************************************/
int app_init_mutex(tAPP_MUTEX *p_mutex);

/*******************************************************************************
 **
 ** Function         app_delete_mutex
 **
 ** Description      Delete the application mutex.
 **
 ** Parameters       p_mutex: pointer to the application instance
 **
 ** Returns          0 if success, error code otherwise
 **
 *******************************************************************************/
int app_delete_mutex(tAPP_MUTEX *p_mutex);

/*******************************************************************************
 **
 ** Function         app_lock_mutex
 **
 ** Description      Lock the application mutex.
 **
 ** Parameters       p_mutex: pointer to the application instance
 **
 ** Returns          0 if success, error code otherwise
 **
 *******************************************************************************/
int app_lock_mutex(tAPP_MUTEX *p_mutex);

/*******************************************************************************
 **
 ** Function         app_unlock_mutex
 **
 ** Description      Unlock the application mutex.
 **
 ** Parameters       p_mutex: pointer to the application instance
 **
 ** Returns          0 if success, error code otherwise
 **
 *******************************************************************************/
int app_unlock_mutex(tAPP_MUTEX *p_mutex);

#endif


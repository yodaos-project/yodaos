/*****************************************************************************
 **
 **  Name:           app_mutex.c
 **
 **  Description:    Mutual exclusion semaphores utility functions for
 **                  applications
 **
 **  Copyright (c) 2009-2012, Broadcom Corp., All Rights Reserved.
 **  Broadcom Bluetooth Core. Proprietary and confidential.
 **
 *****************************************************************************/

/* For perror */
#include <stdio.h>
/* For pthread_* functions */
#include <pthread.h>

#include "app_mutex.h"

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
int app_init_mutex(tAPP_MUTEX *p_mutex)
{
    int status;
#ifdef __CYGWIN__
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    status = pthread_mutex_init(p_mutex, &attr);
#elif defined(__FreeBSD__)
    memset(p_mutex, 0, sizeof(tAPP_MUTEX));
    status = pthread_mutex_init(&(p_mutex->pthread_mutex), NULL);
#else
    status = pthread_mutex_init(p_mutex, NULL);
#endif
    if (status)
    {
        perror("app_init_mutex pthread_mutex_init failed Reason:");
        return status;
    }

#if defined(__FreeBSD__)
    status = pthread_cond_init(&(p_mutex->pthread_cond), NULL);
    if (status)
    {
        perror("app_init_mutex pthread_cond_init failed Reason:");
        pthread_mutex_destroy(&(p_mutex->pthread_mutex));
        return status;
    }
    p_mutex->count = 1;
#endif

    return status;
}

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
int app_delete_mutex(tAPP_MUTEX *p_mutex)
{
    int status;
#if defined(__FreeBSD__)
    int ret = 0;

    status = pthread_mutex_destroy(&(p_mutex->pthread_mutex));
    if (status)
    {
        perror("app_delete_mutex pthread_mutex_destroy failure while destroying Reason:");
        ret = -1;
    }
    status = pthread_cond_destroy(&(p_mutex->pthread_cond));
    if (status)
    {
        perror("app_delete_mutex pthread_cond_destroy failure while destroying Reason:");
        ret = -1;
    }
    p_mutex->count = 0;

    return(ret);
#else
    status = pthread_mutex_destroy(p_mutex);
    if (status)
    {
        perror("app_delete_mutex pthread_mutex_destroy failure while destroying Reason:");
        return (-1);
    }
    return 0;
#endif
}

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
int app_lock_mutex(tAPP_MUTEX *p_mutex)
{
    int status;

    /* locking the mutex */
#if defined(__FreeBSD__)
    status = pthread_mutex_lock(&(p_mutex->pthread_mutex));
#else
    status = pthread_mutex_lock(p_mutex);
#endif
    if (status)
    {
        perror("app_lock_mutex pthread_mutex_lock failure Reason:");
    }

#if defined(__FreeBSD__)
    p_mutex->count--;
    if (p_mutex->count < 0)
    {
        status = pthread_cond_wait(&(p_mutex->pthread_cond), &(p_mutex->pthread_mutex));
        if (status)
        {
            perror("app_lock_mutex pthread_cond_wait failure Reason:");
            pthread_mutex_unlock(&(p_mutex->pthread_mutex));
            return -1;
        }
    }
    status = pthread_mutex_unlock(&(p_mutex->pthread_mutex));
    if (status)
    {
        perror("app_lock_mutex pthread_unlock_mutex failure Reason:");
    }
#endif
    return status;
}

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
int app_unlock_mutex(tAPP_MUTEX *p_mutex)
{
    int status;

    /* unlocking the mutex */
#if defined(__FreeBSD__)
    status = pthread_mutex_lock(&(p_mutex->pthread_mutex));
    if (status)
    {
        perror("app_unlock_mutex pthread_mutex_lock failure Reason:");
        return -1;
    }
    p_mutex->count++;
    if (p_mutex->count < 1) {
        status = pthread_cond_signal(&(p_mutex->pthread_cond));
        if (status)
        {
            perror("app_unlock_mutex pthread_cond_signal failure Reason:");
            pthread_mutex_unlock(&(p_mutex->pthread_mutex));
            return -1;
        }
    }
    status = pthread_mutex_unlock(&(p_mutex->pthread_mutex));
#else
    status = pthread_mutex_unlock(p_mutex);
#endif
    if (status)
    {
        perror("app_unlock_mutex pthread_mutex_unlock failure Reason:");
    }
    return status;
}


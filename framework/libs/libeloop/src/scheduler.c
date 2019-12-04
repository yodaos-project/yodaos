#include "scheduler.h"
#include "syncer.h"
#include "list.h"

#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>

struct scheduler;

struct schd_timer {
    struct list_head list;

    uint32_t timeout_ms;
    uint32_t period_ms;
    task_cb_t cb;
    void *ctx;

    timer_t system_timer;
    pthread_attr_t system_timer_attr;

    struct syncer *syncer;
};

struct scheduler {
    struct list_head timers_list;
    struct syncer *syncer;
};

int scheduler_timer_start(eloop_id_t timer_id)
{
    struct schd_timer *schd_timer = (struct schd_timer *)timer_id;
    struct itimerspec its = {
        .it_value = {
            .tv_sec = schd_timer->timeout_ms / 1e3,
            .tv_nsec = (schd_timer->timeout_ms % 1000) * 1e6,
        },
        .it_interval = {
            .tv_sec = schd_timer->period_ms / 1e3,
            .tv_nsec = (schd_timer->period_ms % 1000) * 1e6,
        },
    };

    return timer_settime(schd_timer->system_timer, 0, &its, NULL);
}

int scheduler_timer_stop(eloop_id_t timer_id)
{
    struct schd_timer *schd_timer = (struct schd_timer *)timer_id;
    struct itimerspec its = {0};

    return timer_settime(schd_timer->system_timer, 0, &its, NULL);
}

static void schd_timer_sync_handler(void *ctx)
{
    struct schd_timer *schd_timer = ctx;

    schd_timer->cb(schd_timer->ctx);
}

static void schd_timer_handle_timeout(union sigval sv)
{
    struct schd_timer *schd_timer = sv.sival_ptr;

    syncer_task_add(schd_timer->syncer,
                    schd_timer_sync_handler,
                    schd_timer);
}

static int schd_timer_init_system_timer(struct schd_timer *schd_timer)
{
    struct sigevent se = {
        .sigev_notify = SIGEV_THREAD,
        .sigev_notify_function = schd_timer_handle_timeout,
        .sigev_notify_attributes = &schd_timer->system_timer_attr,
        .sigev_value = {
            .sival_ptr = schd_timer,
        },
    };

    return timer_create(CLOCK_MONOTONIC, &se, &schd_timer->system_timer);
}

static int schd_timer_init_system_timer_attr(struct schd_timer *schd_timer)
{
    if (pthread_attr_init(&schd_timer->system_timer_attr))
        return -1;

    return pthread_attr_setdetachstate(&schd_timer->system_timer_attr,
                                       PTHREAD_CREATE_DETACHED);
}

static int schd_timer_destroy_system_timer_attr(struct schd_timer *schd_timer)
{
    return pthread_attr_destroy(&schd_timer->system_timer_attr);
}

eloop_id_t scheduler_timer_new(struct scheduler *schd,
                               task_cb_t cb,
                               void *ctx,
                               uint32_t expires_ms,
                               uint32_t period_ms)
{
    struct schd_timer *schd_timer = malloc(sizeof(*schd_timer));
    if (!schd_timer)
        return INVALID_ID;

    schd_timer->timeout_ms = expires_ms;
    schd_timer->period_ms = period_ms;
    schd_timer->cb = cb;
    schd_timer->ctx = ctx;
    schd_timer->syncer = schd->syncer;

    if (schd_timer_init_system_timer_attr(schd_timer)) {
        free(schd_timer);
        return INVALID_ID;
    }

    if (schd_timer_init_system_timer(schd_timer)) {
        schd_timer_destroy_system_timer_attr(schd_timer);
        free(schd_timer);
        return INVALID_ID;
    }

    list_add(&schd->timers_list, &schd_timer->list);

    return (eloop_id_t)schd_timer;
}

void scheduler_timer_delete(eloop_id_t timer_id)
{
    struct schd_timer *schd_timer = (struct schd_timer *)timer_id;

    list_del(&schd_timer->list);
    timer_delete(schd_timer->system_timer);
    schd_timer_destroy_system_timer_attr(schd_timer);
    free(schd_timer);
}

struct scheduler *scheduler_new(struct syncer *syncer)
{
    struct scheduler *schd = malloc(sizeof(*schd));
    if (!schd)
        return NULL;


    list_init(&schd->timers_list);
    schd->syncer = syncer;

    return schd;
}

void scheduler_delete(struct scheduler *schd)
{
    struct list_head *itr, *item;

    list_for_each_safe(&schd->timers_list, itr, item)
        scheduler_timer_delete((eloop_id_t)item);

    free(schd);
}

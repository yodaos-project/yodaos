#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <eloop.h>

struct syncer;
struct scheduler;

struct scheduler *scheduler_new(struct syncer *syncer);

void scheduler_delete(struct scheduler *schd);

eloop_id_t scheduler_timer_new(struct scheduler *schd,
                               task_cb_t cb,
                               void *ctx,
                               unsigned int expires_ms,
                               unsigned int period_ms);

void scheduler_timer_delete(eloop_id_t timer_id);

int scheduler_timer_start(eloop_id_t timer_id);

int scheduler_timer_stop(eloop_id_t timer_id);

#endif /* __SCHEDULER_H__ */

#ifndef __SYNCER_H__
#define __SYNCER_H__

#include "eloop.h"

struct syncer;

struct syncer *syncer_new(void);

void syncer_delete(struct syncer *syncer);

void syncer_process_queue(struct syncer *syncer);

void syncer_stop(struct syncer *syncer);

eloop_id_t syncer_task_add(struct syncer *syncer,
                           task_cb_t cb,
                           void *ctx);

int syncer_task_cancel(struct syncer *syncer,
                       eloop_id_t task_id);

#endif /* __SYNCER_H__ */

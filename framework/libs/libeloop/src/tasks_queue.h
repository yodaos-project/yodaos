#ifndef __TASKS_QUEUE_H__
#define __TASKS_QUEUE_H__

#include "eloop.h"

#include "list.h"

struct tasks_queue;

struct tasks_queue *tasks_queue_new(void);

void tasks_queue_delete(struct tasks_queue *queue,
                        bool execute_all);

int tasks_queue_add(struct tasks_queue *queue,
                    task_cb_t cb,
                    void *ctx,
                    eloop_id_t task_id);

void tasks_queue_remove(struct tasks_queue *queue,
                        eloop_id_t task_id);

eloop_id_t tasks_queue_execute_next(struct tasks_queue *queue);

unsigned int tasks_queue_pending_tasks(struct tasks_queue *queue);

#endif /* __TASKS_QUEUE_H__ */

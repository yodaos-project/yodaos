#include "tasks_queue.h"

#include <stdlib.h>
#include <string.h>

#define TASK_ID_MIN 1
#define TASK_ID_MAX ((1 << 16) -1)

struct task {
    struct list_head list;
    task_cb_t cb;
    void *ctx;
    eloop_id_t id;
};

#define _execute_task(t) \
    (t)->cb(t->ctx);

struct tasks_queue {
    struct list_head head;
    unsigned int len;
};

static struct task *task_new(task_cb_t cb,
                             void *ctx,
                             eloop_id_t task_id)
{
    struct task *new_task = malloc(sizeof(*new_task));
    if (!new_task)
        return NULL;

    list_init(&new_task->list);
    new_task->cb = cb;
    new_task->ctx = ctx;
    new_task->id = task_id;

    return new_task;
}

static void task_delete(struct task *rem_task)
{
    list_del(&rem_task->list);
    free(rem_task);
}

struct tasks_queue *tasks_queue_new()
{
    struct tasks_queue *queue = malloc(sizeof(*queue));
    if (!queue)
        return NULL;

    list_init(&queue->head);
    queue->len = 0;

    return queue;
}

void tasks_queue_delete(struct tasks_queue *queue,
                        bool execute_all)
{
    struct list_head *itr, *item;

    list_for_each_safe(&queue->head, itr, item) {
        struct task *current_task = (struct task *)item;
        if (execute_all)
            _execute_task(current_task);

        task_delete(current_task);
        queue->len--;
    }

    free(queue);
}

int tasks_queue_add(struct tasks_queue *queue,
                    task_cb_t cb,
                    void *ctx,
                    eloop_id_t task_id)
{
    struct task *new_task = task_new(cb, ctx, task_id);
    if (!new_task)
        return -1;

    list_add_last(&queue->head, &new_task->list);

    queue->len++;

    return 0;
}

static struct task *task_find(struct tasks_queue *queue,
                              eloop_id_t id)
{
    struct list_head *item = NULL, *itr;

    list_for_each(&queue->head, itr) {
        if (((struct task *)itr)->id != id)
            continue;

        item = itr;
        break;
    }

    return (struct task *)item;
}

void tasks_queue_remove(struct tasks_queue *queue, eloop_id_t id)
{
    struct task *item = task_find(queue, id);
    if (!item)
        return;

    task_delete(item);
    queue->len--;
}

eloop_id_t tasks_queue_execute_next(struct tasks_queue *queue)
{
    struct task *current_task;
    eloop_id_t id;

    if (!queue->len)
        return 0;

    current_task = (struct task *)queue->head.next;
    _execute_task(current_task);

    id = current_task->id;
    task_delete(current_task);
    queue->len--;

    return id;
}

unsigned int tasks_queue_pending_tasks(struct tasks_queue *queue)
{
    return queue->len;
}

#include "syncer.h"
#include "olist.h"

#include <stdio.h>
#include <stdlib.h>

struct task_ctx {
    struct olist_head head;
    struct syncer *syncer;
    eloop_id_t inner_id;
};

static int task_ctx_cmp(void *key1, void *key2)
{
    struct task_ctx *ctx1 = key1,
                    *ctx2 = key2;
                    
    if (ctx1->inner_id > ctx2->inner_id)
        return 1; 

    if (ctx1->inner_id < ctx2->inner_id)
        return -1; 

    return 0;
}

static olist_new(contexts, task_ctx_cmp);

static eloop_id_t get_next_inner_id()
{
    static eloop_id_t next_id = 1;
    eloop_id_t ret;

    ret = next_id;
    next_id++;

    return ret;
}

static void task(void *ctx);

static struct task_ctx *create_new_task_ctx(struct syncer *syncer,
                                            eloop_id_t inner_id)
{
    struct task_ctx *data = malloc(sizeof(*data));
    if (!data)
        return NULL;

    data->syncer = syncer;
    data->inner_id = inner_id;

    olist_add(&contexts, &data->head, data);

    return data;
}

static void delete_task(struct task_ctx *task)
{
    olist_del(&task->head);
    free(task);
}

static void add_n_tasks(struct syncer *syncer, unsigned int n)
{
    struct task_ctx *data;
    eloop_id_t id;
    unsigned int i;

    for (i = 0; i < n; i++) {
        data = create_new_task_ctx(syncer, get_next_inner_id());
        if (!data)
            break;

        id = syncer_task_add(data->syncer, task, data);
        if (id == INVALID_ID) {
            delete_task(data);
            break;
        }
        printf("Added task #%"PRIid" (id %"PRIid")\n",
               data->inner_id, id);
    }
}

static void task(void *ctx)
{
    struct task_ctx *data = ctx;

    printf("Executing task #%"PRIid"\n", data->inner_id);

    switch (data->inner_id) {
        case 1:
            add_n_tasks(data->syncer, 10);
            break;
        case 2:
            syncer_task_cancel(data->syncer, 3);
            break;
        case 5:
            syncer_stop(data->syncer);
            break;
        default:
            break;
    }

    olist_del_by_key(&contexts, ctx);
    free(ctx);
}

static void delete_unused_contexts()
{
    struct olist_head *itr, *item;
    struct task_ctx *data;

    olist_for_each_safe(&contexts, itr, item) {
        data = (struct task_ctx *)item;
        printf("Task #%"PRIid" has not been executed\n", data->inner_id);
        delete_task(data);
    }
}

int main()
{
    struct syncer *syncer = syncer_new();
    if (!syncer) {
        printf("Error creating a new syncer\n");
        return 1;
    }

    add_n_tasks(syncer, 1);
    syncer_process_queue(syncer);

    syncer_delete(syncer);

    delete_unused_contexts();

    return 0;
}

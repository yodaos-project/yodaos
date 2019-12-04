#include <tasks_queue.h>
#include <stdio.h>

#define arr_len(arr) \
    (sizeof(arr)/sizeof((arr)[0]))

static void work_func_1(void *ctx)
{
    printf("Function #1 with value %d\n", *(int *)ctx);
}

static void work_func_2(void *ctx)
{
    printf("Function #2 with value %d\n", *(int *)ctx);
}

struct {
    task_cb_t cb;
    int ctx;
} tasks[] = {
    { work_func_1, 0 },
    { work_func_1, 0 },
    { work_func_2, 0 },
    { work_func_2, 0 },
    { work_func_1, 0 },
    { work_func_1, 0 },
    { work_func_2, 0 },
    { work_func_2, 0 },
    { work_func_1, 0 },
    { work_func_2, 0 },
};

static void fill_queue(struct tasks_queue *queue)
{
    int i;

    for (i = 0; i < arr_len(tasks); i++) {
        tasks[i].ctx = i + 1;
        tasks_queue_add(queue, tasks[i].cb, &tasks[i].ctx, i);
    }
}

static void execute_queue(struct tasks_queue *queue, int n)
{
    while (tasks_queue_execute_next(queue) > 0);
}

int main()
{
    struct tasks_queue *queue = tasks_queue_new();
    if (!queue)
        return 1;

    tasks_queue_remove(queue, 110);
    fill_queue(queue);
    tasks_queue_remove(queue, 3);
    tasks_queue_remove(queue, 6);
    execute_queue(queue, 4);
    tasks_queue_remove(queue, 5);
    tasks_queue_remove(queue, 11);
    tasks_queue_delete(queue, true);

    return 0;
}

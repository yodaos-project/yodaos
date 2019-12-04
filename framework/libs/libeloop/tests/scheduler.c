#include <scheduler.h>
#include <syncer.h>

#include <stdio.h>
#include <stdlib.h>

struct test_data {
    struct scheduler *scheduler;
    struct syncer *syncer;
    bool stop;

    eloop_id_t timer_1s_id;
    eloop_id_t timer_5s_id;
    eloop_id_t timer_stop_id;
};

void timer_1s_task(void *ctx)
{
    printf("-");
}

void timer_5s_task(void *ctx)
{
    struct test_data *test_data = ctx;
    static uint32_t call_count = 0;

    if (call_count % 2)
        scheduler_timer_stop(test_data->timer_1s_id);
    else
        scheduler_timer_start(test_data->timer_1s_id);

    printf("|");

    call_count++;
}

void timer_stop_task(void *ctx)
{
    struct test_data *test_data = ctx;

    printf("\n");

    scheduler_timer_stop(test_data->timer_1s_id);
    scheduler_timer_stop(test_data->timer_5s_id);

    test_data->stop = true;
}

static struct test_data *test_new()
{
    struct test_data *test_data = malloc(sizeof(*test_data));
    if (!test_data)
        return NULL;

    test_data->syncer = syncer_new();
    if (!test_data->syncer) {
        free(test_data);
        return NULL;
    }

    test_data->scheduler = scheduler_new(test_data->syncer);
    if (!test_data->scheduler) {
        syncer_delete(test_data->syncer);
        free(test_data);
        return NULL;
    }

    test_data->stop = false;

    return test_data;
}

static int test_prepare_timers(struct test_data *test_data)
{
    test_data->timer_1s_id = scheduler_timer_new(test_data->scheduler,
                                                 timer_1s_task,
                                                 test_data,
                                                 1000,
                                                 1000); 
    if (test_data->timer_1s_id == INVALID_ID) {
        printf("Error setting 1s timer\n");
        return -1;
    }

    test_data->timer_5s_id = scheduler_timer_new(test_data->scheduler,
                                                 timer_5s_task,
                                                 test_data,
                                                 10,
                                                 5000); 
    if (test_data->timer_5s_id == INVALID_ID) {
        scheduler_timer_delete(test_data->timer_1s_id);
        printf("Error setting 5s timer\n");
        return -1;
    }

    test_data->timer_stop_id = scheduler_timer_new(test_data->scheduler,
                                                   timer_stop_task,
                                                   test_data,
                                                   20100,
                                                   0); 
    if (test_data->timer_stop_id == INVALID_ID) {
        scheduler_timer_delete(test_data->timer_1s_id);
        scheduler_timer_delete(test_data->timer_5s_id);
        printf("Error setting stop timer\n");
        return -1;
    }

    return 0;
}

static int test_start_timers(struct test_data *test_data)
{
    if (scheduler_timer_start(test_data->timer_5s_id))
        return -1;

    if (scheduler_timer_start(test_data->timer_stop_id))
        return -1;

    return 0;
}

static void test_delete(struct test_data *test_data)
{
    scheduler_delete(test_data->scheduler); 
    syncer_delete(test_data->syncer); 
    free(test_data);
}

static void stdout_no_buffering()
{
    setvbuf(stdout, NULL, _IONBF, 0);
}

int main()
{
    stdout_no_buffering();

    struct test_data *test_data = test_new();
    if (!test_data) {
        printf("Error initializing test\n");
        return 1;
    }

    if (test_prepare_timers(test_data)) {
        test_delete(test_data);
        return 1;
    }

    printf("1sec timer ID is %"PRIid"\n", test_data->timer_1s_id);
    printf("5sec timer ID is %"PRIid"\n", test_data->timer_5s_id);
    printf("stop timer ID is %"PRIid"\n", test_data->timer_stop_id);

    if (test_start_timers(test_data)) {
        test_delete(test_data);
        return 1;
    }

    while (!test_data->stop)
        syncer_process_queue(test_data->syncer);

    test_delete(test_data);

    return 0;
}

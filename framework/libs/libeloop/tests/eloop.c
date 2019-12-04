#include <eloop.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

struct test_data {
    int comm[2];
    eloop_id_t comm_handler_id;
    eloop_id_t send_timer_id;
    eloop_id_t verify_timer_id;
    eloop_id_t terminate_timer_id;
    uint32_t counter;
    uint32_t received;
};

#define COMM_READ   0
#define COMM_WRITE  1

#define NOW 1
#define SEND_PERIOD_MS 50
#define VERIFY_LATENCY_MS (SEND_PERIOD_MS / 2)
#define TEST_DURATION_MS 5000

static int comm_read(struct test_data *test_data,
                     char *buff, size_t sz);

void comm_handle_read_cb(void *ctx, uint32_t event_mask)
{
    struct test_data *test_data = ctx;

    if (event_mask & ELOOP_FD_EVENT_READ)
        comm_read(test_data, (char *)&test_data->received,
                  sizeof(test_data->received));
}

static int comm_open(struct test_data *test_data)
{
    int res = pipe2(test_data->comm, O_NONBLOCK);
    if (res)
        return res;

    test_data->comm_handler_id =
        eloop_handler_add_fd(test_data->comm[COMM_READ],
                             comm_handle_read_cb, test_data);
    if (test_data->comm_handler_id == INVALID_ID)
        return -1;

    return 0;
}

static void comm_close(struct test_data *test_data)
{
    eloop_handler_remove_fd(test_data->comm_handler_id);

    if (test_data->comm[COMM_READ])
        close(test_data->comm[COMM_READ]);
    if (test_data->comm[COMM_WRITE])
        close(test_data->comm[COMM_WRITE]);
}

static int comm_write(struct test_data *test_data,
                      char *buff, size_t sz)
{
    return write(test_data->comm[COMM_WRITE], buff, sz);
}

static int comm_read(struct test_data *test_data,
                     char *buff, size_t sz)
{
    return read(test_data->comm[COMM_READ], buff, sz);
}

void send_cb(void *ctx)
{
    struct test_data *test_data = ctx;

    test_data->counter++;

    comm_write(test_data, (char *)&test_data->counter,
               sizeof(test_data->counter));
}

void verify_cb(void *ctx)
{
    struct test_data *test_data = ctx;

    printf("Expected: %-4"PRIu32" ;  Received: %-4"PRIu32"\n",
           test_data->counter, test_data->received);
}

void terminate_cb(void *ctx)
{
    eloop_stop();
}

static int init_timers(struct test_data *test_data)
{
    test_data->terminate_timer_id =
        eloop_timer_add(terminate_cb, test_data, TEST_DURATION_MS, 0);
    if (test_data->terminate_timer_id == INVALID_ID)
        return -1;

    test_data->send_timer_id =
        eloop_timer_add(send_cb, test_data, NOW, SEND_PERIOD_MS);
    if (test_data->send_timer_id == INVALID_ID)
        return -1;

    test_data->verify_timer_id = eloop_timer_add(verify_cb,
                                                 test_data,
                                                 VERIFY_LATENCY_MS,
                                                 SEND_PERIOD_MS);
    if (test_data->verify_timer_id == INVALID_ID)
        return -1;

    return 0;
}

static int test_init(struct test_data *test_data)
{
    int res;
   
    res = eloop_init();
    if (res)
        return res;

    res = comm_open(test_data);
    if (res)
        return res;

    test_data->counter = 0;
    test_data->received = 0;

    if (init_timers(test_data))
        return -1;

    return 0;
}

static void test_run(struct test_data *test_data)
{
    eloop_timer_start(test_data->send_timer_id);
    eloop_timer_start(test_data->verify_timer_id);
    eloop_timer_start(test_data->terminate_timer_id);
    eloop_run();
}

static int test_close(struct test_data *test_data)
{
    eloop_timer_stop(test_data->send_timer_id);
    eloop_timer_stop(test_data->verify_timer_id);
    comm_close(test_data);
    eloop_exit();
}

int main()
{
    struct test_data test_data;

    if (test_init(&test_data))
        return 1;

    test_run(&test_data);

    test_close(&test_data);

    return 0;
}

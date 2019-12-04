#include <eloop.h>

#include "syncer.h"
#include "scheduler.h"
#include "fd.h"

#define MAX_FD_EVENTS 512

struct {
    struct syncer *syncer;
    struct scheduler *scheduler;
    struct fd_handler *fd_handler;
    bool running;
} eloop_ctx;

eloop_id_t eloop_timer_add(task_cb_t cb,
                           void *ctx,
                           unsigned int expires_ms,
                           unsigned int period_ms)
{
    return scheduler_timer_new(eloop_ctx.scheduler,
                               cb, ctx, expires_ms, period_ms);
}

void eloop_timer_delete(eloop_id_t timer_id)
{
    scheduler_timer_delete(timer_id);
}

int eloop_timer_start(eloop_id_t timer_id)
{
    scheduler_timer_start(timer_id);
}

int eloop_timer_stop(eloop_id_t timer_id)
{
    scheduler_timer_stop(timer_id);
}

eloop_id_t eloop_handler_add_fd(int fd, fd_cb_t cb, void *ctx)
{
    return fd_handler_add_fd(eloop_ctx.fd_handler, fd, cb, ctx);
}

void eloop_handler_remove_fd(eloop_id_t fd_id)
{
    fd_handler_remove_fd(eloop_ctx.fd_handler, fd_id);
}

int eloop_init()
{
    eloop_ctx.running = false;

    eloop_ctx.syncer = syncer_new();
    if (!eloop_ctx.syncer)
        return -1; 

    eloop_ctx.scheduler = scheduler_new(eloop_ctx.syncer);
    if (!eloop_ctx.scheduler) {
        syncer_delete(eloop_ctx.syncer);
        return -1; 
    }

    eloop_ctx.fd_handler =
        fd_handler_new(eloop_ctx.syncer, MAX_FD_EVENTS);
    if (!eloop_ctx.fd_handler) {
        scheduler_delete(eloop_ctx.scheduler);
        syncer_delete(eloop_ctx.syncer);
        return -1;
    }

    eloop_ctx.running = true;

    return 0;
}

void eloop_exit()
{
    fd_handler_delete(eloop_ctx.fd_handler);
    scheduler_delete(eloop_ctx.scheduler);
    syncer_delete(eloop_ctx.syncer);
}

void eloop_run()
{
    while (eloop_ctx.running) {
        fd_handler_handle_events(eloop_ctx.fd_handler);
        syncer_process_queue(eloop_ctx.syncer);
    }
}

void eloop_stop()
{
    eloop_ctx.running = false;
}

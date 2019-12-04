#include "fd.h"
#include "list.h"

#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>

struct fd_handler {
    struct syncer *syncer;
    struct list_head fd_list;
    int poll_fd;
    uint32_t max_events;
    struct epoll_event* events_buffer;
};

#define _event_ctx(handler, idx) \
    (struct fd_data *)(handler)->events_buffer[(idx)].data.ptr
#define _event_mask(handler, idx) \
    (handler)->events_buffer[(idx)].events;

struct fd_data {
    struct list_head list;
    int fd;
    fd_cb_t cb;
    void *ctx;
};

static struct fd_data *fd_data_new(struct fd_handler *handler,
                                   int fd,
                                   fd_cb_t cb,
                                   void *ctx)
{
    struct fd_data *fd_data = malloc(sizeof(*fd_data));
    if (!fd_data)
        return NULL;

    list_add(&handler->fd_list, &fd_data->list);
    fd_data->fd = fd;
    fd_data->cb = cb;
    fd_data->ctx = ctx;

    return fd_data;
}

static void fd_data_delete(struct fd_data *fd_data)
{
    list_del(&fd_data->list);
    free(fd_data);
}

struct fd_handler *fd_handler_new(struct syncer *syncer,
                                  uint32_t max_events)
{
    struct fd_handler *handler =
        (struct fd_handler *)malloc(sizeof(*handler));
    if (!handler)
        return NULL;

    handler->syncer = syncer;
    handler->max_events = max_events;
    handler->events_buffer = calloc(handler->max_events,
                                    sizeof(struct epoll_event));
    if (!handler->events_buffer)
    {
        free(handler);
        return NULL;
    }

    list_init(&handler->fd_list);

    handler->poll_fd = epoll_create1(0);
    if (handler->poll_fd < 0)
    {
        free(handler->events_buffer);
        free(handler);
        return NULL;
    }

    return handler;
}

void fd_handler_delete(struct fd_handler *handler)
{
    struct list_head *itr, *item;

    list_for_each_safe(&handler->fd_list, itr, item)
        fd_handler_remove_fd(handler, (eloop_id_t)item);

    close(handler->poll_fd);
    free(handler->events_buffer);
    free(handler);
}

eloop_id_t fd_handler_add_fd(struct fd_handler *handler,
                             int fd,
                             fd_cb_t cb,
                             void *ctx)
{
    struct epoll_event event_config;
    struct fd_data *fd_data = fd_data_new(handler, fd, cb, ctx);
    if (!fd_data)
        return INVALID_ID;

    event_config.data.ptr = fd_data;
    event_config.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;

    if (epoll_ctl(handler->poll_fd,
                  EPOLL_CTL_ADD,
                  fd_data->fd,
                  &event_config) < 0) {
        fd_data_delete(fd_data);
        return INVALID_ID;
    }

    return (eloop_id_t)fd_data;
}

void fd_handler_remove_fd(struct fd_handler *handler,
                          eloop_id_t fd_id)
{
    struct fd_data *fd_data = (struct fd_data *)fd_id;

    epoll_ctl(handler->poll_fd, EPOLL_CTL_DEL, fd_data->fd, NULL);
    fd_data_delete(fd_data);
}

struct event_sync_ctx {
    struct fd_data *fd_data;
    uint32_t event_mask;
};

static struct event_sync_ctx *event_sync_ctx_new(struct fd_data *fd_data,
                                                 uint32_t event_mask)
{
    struct event_sync_ctx *event_sync_ctx = malloc(sizeof(*event_sync_ctx));
    if (!event_sync_ctx)
        return NULL;

    event_sync_ctx->fd_data = fd_data;
    event_sync_ctx->event_mask = event_mask;

    return event_sync_ctx;
}

static void event_sync_ctx_delete(struct event_sync_ctx *event_sync_ctx)
{
    free(event_sync_ctx); 
}

static void fd_handler_sync_event(void *ctx)
{
    struct event_sync_ctx *event_sync_ctx = ctx;
    struct fd_data *fd_data = event_sync_ctx->fd_data;

    fd_data->cb(fd_data->ctx, event_sync_ctx->event_mask);
    event_sync_ctx_delete(event_sync_ctx);
}

static uint32_t epoll_event_to_fd_event(uint32_t epoll_event)
{
    uint32_t event_mask = 0;

    if (epoll_event & EPOLLERR) {
        event_mask = ELOOP_FD_EVENT_ERROR;
    } else if ((epoll_event & EPOLLHUP) ||
            (epoll_event & EPOLLRDHUP)) {
        event_mask = ELOOP_FD_EVENT_HUP;
    } else {
        if (epoll_event & EPOLLIN)
            event_mask |= ELOOP_FD_EVENT_READ;
        if (epoll_event & EPOLLOUT)
            event_mask |= ELOOP_FD_EVENT_WRITE;
    }

    return event_mask;
}

void fd_handler_handle_events(struct fd_handler *handler)
{
    uint32_t i;
    int num_events = epoll_wait(handler->poll_fd,
                                handler->events_buffer,
                                handler->max_events,
                                5);
    if (num_events <= 0)
        return;

    for (i = 0; i < num_events; i++) {
        struct fd_data *fd_data = _event_ctx(handler, i);
        uint32_t current_event = _event_mask(handler, i);
        uint32_t event = epoll_event_to_fd_event(current_event);

        struct event_sync_ctx *event_sync_ctx =
            event_sync_ctx_new(fd_data, event);
        if (!event_sync_ctx)
            continue;
             
        syncer_task_add(handler->syncer, fd_handler_sync_event, event_sync_ctx);
    }
}

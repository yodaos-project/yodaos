#ifndef __ELOOP_H__
#define __ELOOP_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <inttypes.h>
#include <stdbool.h>

#define INVALID_ID 0
#define PRIid PRIuPTR
typedef uintptr_t eloop_id_t;

typedef void (*task_cb_t)(void *ctx);
typedef void (*fd_cb_t)(void *ctx, uint32_t event_mask);

int eloop_init(void);
void eloop_exit(void);
void eloop_run(void);
void eloop_stop(void);

eloop_id_t eloop_timer_add(task_cb_t cb,
                           void *ctx,
                           unsigned int expires_ms,
                           unsigned int period_ms);
void eloop_timer_delete(eloop_id_t timer_id);
int eloop_timer_start(eloop_id_t timer_id);
int eloop_timer_stop(eloop_id_t timer_id);

#define ELOOP_FD_EVENT_ERROR 0x01
#define ELOOP_FD_EVENT_HUP   0x02
#define ELOOP_FD_EVENT_READ  0x04
#define ELOOP_FD_EVENT_WRITE 0x08

eloop_id_t eloop_handler_add_fd(int fd, fd_cb_t cb, void *ctx);
void eloop_handler_remove_fd(eloop_id_t fd_id);

#endif /* __ELOOP_H__ */

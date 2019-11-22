/* Copyright 2015-2016 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "internal.h"

#include <stddef.h> /* NULL */
#include <stdio.h> /* printf */
#include <stdlib.h>
#include <string.h> /* strerror */
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h> /* INT_MAX, PATH_MAX, IOV_MAX */
#if !defined(__TIZENRT__)
# include <sys/uio.h> /* writev */
# include <pwd.h>
#endif

#ifdef __sun
# include <sys/filio.h>
# include <sys/types.h>
# include <sys/wait.h>
#endif

#ifdef __APPLE__
# include <mach-o/dyld.h> /* _NSGetExecutablePath */
# include <sys/filio.h>
# if defined(O_CLOEXEC)
#  define UV__O_CLOEXEC O_CLOEXEC
# endif
#endif

#if defined(__DragonFly__)      || \
    defined(__FreeBSD__)        || \
    defined(__FreeBSD_kernel__)
# include <sys/sysctl.h>
# include <sys/filio.h>
# include <sys/wait.h>
# define UV__O_CLOEXEC O_CLOEXEC
# if defined(__FreeBSD__) && __FreeBSD__ >= 10
#  define uv__accept4 accept4
#  define UV__SOCK_NONBLOCK SOCK_NONBLOCK
#  define UV__SOCK_CLOEXEC  SOCK_CLOEXEC
# endif
# if !defined(F_DUP2FD_CLOEXEC) && defined(_F_DUP2FD_CLOEXEC)
#  define F_DUP2FD_CLOEXEC  _F_DUP2FD_CLOEXEC
# endif
#endif

#if defined(__ANDROID_API__) && __ANDROID_API__ < 21
# include <dlfcn.h>  /* for dlsym */
#endif

#if defined(__MVS__)
#include <sys/ioctl.h>
#endif

static int uv__run_pending(uv_loop_t* loop);

/* Verify that uv_buf_t is ABI-compatible with struct iovec. */
STATIC_ASSERT(sizeof(uv_buf_t) == sizeof(struct iovec));
STATIC_ASSERT(sizeof(&((uv_buf_t*) 0)->base) ==
              sizeof(((struct iovec*) 0)->iov_base));
STATIC_ASSERT(sizeof(&((uv_buf_t*) 0)->len) ==
              sizeof(((struct iovec*) 0)->iov_len));
STATIC_ASSERT(offsetof(uv_buf_t, base) == offsetof(struct iovec, iov_base));
STATIC_ASSERT(offsetof(uv_buf_t, len) == offsetof(struct iovec, iov_len));

uint64_t uv_hrtime(void) {
  return uv__hrtime(UV_CLOCK_PRECISE);
}

void uv_close(uv_handle_t* handle, uv_close_cb close_cb) {
  assert(!uv__is_closing(handle));

  handle->flags |= UV_CLOSING;
  handle->close_cb = close_cb;

  switch (handle->type) {
  case UV_NAMED_PIPE:
    uv__pipe_close((uv_pipe_t*)handle);
    break;

  case UV_TTY:
    uv__stream_close((uv_stream_t*)handle);
    break;

  case UV_TCP:
    uv__tcp_close((uv_tcp_t*)handle);
    break;

  case UV_UDP:
    uv__udp_close((uv_udp_t*)handle);
    break;

  case UV_IDLE:
    uv__idle_close((uv_idle_t*)handle);
    break;

  case UV_PREPARE:
    uv__prepare_close((uv_idle_t*)handle);
    break;

  case UV_CHECK:
    uv__check_close((uv_idle_t*)handle);
    break;

  case UV_ASYNC:
    uv__async_close((uv_async_t*)handle);
    break;

  case UV_TIMER:
    uv__timer_close((uv_timer_t*)handle);
    break;

  case UV_PROCESS:
    uv__process_close((uv_process_t*)handle);
    break;

  case UV_POLL:
    uv__poll_close((uv_poll_t*)handle);
    break;

  case UV_SIGNAL:
    uv__signal_close((uv_signal_t*)handle);
    /* Signal handles may not be closed immediately. The signal code will
     * itself close uv__make_close_pending whenever appropriate. */
    return;

  default:
    assert(0);
  }

  uv__make_close_pending(handle);
}


void uv__make_close_pending(uv_handle_t* handle) {
  assert(handle->flags & UV_CLOSING);
  assert(!(handle->flags & UV_CLOSED));
  handle->next_closing = handle->loop->closing_handles;
  handle->loop->closing_handles = handle;
}

int uv__getiovmax(void) {
#if defined(IOV_MAX)
  return IOV_MAX;
#elif defined(_SC_IOV_MAX)
  static int iovmax = -1;
  if (iovmax == -1) {
    iovmax = sysconf(_SC_IOV_MAX);
    /* On some embedded devices (arm-linux-uclibc based ip camera),
     * sysconf(_SC_IOV_MAX) can not get the correct value. The return
     * value is -1 and the errno is EINPROGRESS. Degrade the value to 1.
     */
    if (iovmax == -1) iovmax = 1;
  }
  return iovmax;
#else
  return 1024;
#endif
}


static void uv__finish_close(uv_handle_t* handle) {
  /* Note: while the handle is in the UV_CLOSING state now, it's still possible
   * for it to be active in the sense that uv__is_active() returns true.
   * A good example is when the user calls uv_shutdown(), immediately followed
   * by uv_close(). The handle is considered active at this point because the
   * completion of the shutdown req is still pending.
   */
  assert(handle->flags & UV_CLOSING);
  assert(!(handle->flags & UV_CLOSED));
  handle->flags |= UV_CLOSED;

  switch (handle->type) {
    case UV_PREPARE:
    case UV_CHECK:
    case UV_IDLE:
    case UV_ASYNC:
    case UV_TIMER:
    case UV_PROCESS:
    // case UV_FS_EVENT:
    // case UV_FS_POLL:
    case UV_POLL:
    case UV_SIGNAL:
      break;

    case UV_NAMED_PIPE:
    case UV_TCP:
    case UV_TTY:
      uv__stream_destroy((uv_stream_t*)handle);
      break;

    case UV_UDP:
      uv__udp_finish_close((uv_udp_t*)handle);
      break;

    default:
      assert(0);
      break;
  }

  uv__handle_unref(handle);
  QUEUE_REMOVE(&handle->handle_queue);

  if (handle->close_cb) {
    handle->close_cb(handle);
  }
}


static void uv__run_closing_handles(uv_loop_t* loop) {
  uv_handle_t* p;
  uv_handle_t* q;

  p = loop->closing_handles;
  loop->closing_handles = NULL;

  while (p) {
    q = p->next_closing;
    uv__finish_close(p);
    p = q;
  }
}


int uv_is_closing(const uv_handle_t* handle) {
  return uv__is_closing(handle);
}


int uv_backend_timeout(const uv_loop_t* loop) {
  if (loop->stop_flag != 0)
    return 0;

  if (!uv__has_active_handles(loop) && !uv__has_active_reqs(loop))
    return 0;

  if (!QUEUE_EMPTY(&loop->idle_handles))
    return 0;

  if (!QUEUE_EMPTY(&loop->pending_queue))
    return 0;

  if (loop->closing_handles)
    return 0;

  return uv__next_timeout(loop);
}


static int uv__loop_alive(const uv_loop_t* loop) {
  return uv__has_active_handles(loop) ||
         uv__has_active_reqs(loop) ||
         loop->closing_handles != NULL;
}


int uv_loop_alive(const uv_loop_t* loop) {
    return uv__loop_alive(loop);
}

/* test driver */
int uv_loop_has_active_reqs(const uv_loop_t* loop) {
    return uv__has_active_reqs(loop);
}


int uv_run(uv_loop_t* loop, uv_run_mode mode) {
  int timeout;
  int r;
  int ran_pending;

  r = uv__loop_alive(loop);
  if (!r)
    uv__update_time(loop);

  while (r != 0 && loop->stop_flag == 0) {
    uv__update_time(loop);
    uv__run_timers(loop);
    ran_pending = uv__run_pending(loop);
    uv__run_idle(loop);
    uv__run_prepare(loop);

    timeout = 0;
    if ((mode == UV_RUN_ONCE && !ran_pending) || mode == UV_RUN_DEFAULT)
      timeout = uv_backend_timeout(loop);

    uv__io_poll(loop, timeout);
    uv__run_check(loop);
    uv__run_closing_handles(loop);

    if (mode == UV_RUN_ONCE) {
      /* UV_RUN_ONCE implies forward progress: at least one callback must have
       * been invoked when it returns. uv__io_poll() can return without doing
       * I/O (meaning: no callbacks) when its timeout expires - which means we
       * have pending timers that satisfy the forward progress constraint.
       *
       * UV_RUN_NOWAIT makes no guarantees about progress so it's omitted from
       * the check.
       */
      uv__update_time(loop);
      uv__run_timers(loop);
    }

    r = uv__loop_alive(loop);
    if (mode == UV_RUN_ONCE || mode == UV_RUN_NOWAIT)
      break;
  }

  /* The if statement lets gcc compile it to a conditional store. Avoids
   * dirtying a cache line.
   */
  if (loop->stop_flag != 0)
    loop->stop_flag = 0;

  return r;
}


void uv_update_time(uv_loop_t* loop) {
  uv__update_time(loop);
}


int uv_is_active(const uv_handle_t* handle) {
  return uv__is_active(handle);
}


/* Open a socket in non-blocking close-on-exec mode, atomically if possible. */
int uv__socket(int domain, int type, int protocol) {
  int sockfd;
  int err;

#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
  sockfd = socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
  if (sockfd != -1)
    return sockfd;

  if (errno != EINVAL)
    return -errno;
#endif

  sockfd = socket(domain, type, protocol);
  if (sockfd == -1)
    return -errno;

  err = uv__nonblock(sockfd, 1);
  if (err == 0)
    err = uv__cloexec(sockfd, 1);

  if (err) {
    uv__close(sockfd);
    return err;
  }

#if defined(SO_NOSIGPIPE)
  {
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
  }
#endif

  return sockfd;
}

/* get a file pointer to a file in read-only and close-on-exec mode */
FILE* uv__open_file(const char* path) {
  int fd;
  FILE* fp;

  fd = uv__open_cloexec(path, O_RDONLY);
  if (fd < 0)
    return NULL;

   fp = fdopen(fd, "r");
   if (fp == NULL)
     uv__close(fd);

  return fp;
}

int uv__accept(int sockfd) {
  int peerfd;
  int err;

  assert(sockfd >= 0);

  while (1) {
#if defined(__linux__) || (defined(__FreeBSD__) && __FreeBSD__ >= 10)
    static int no_accept4;

    if (no_accept4)
      goto skip;

    peerfd = uv__accept4(sockfd,
                         NULL,
                         NULL,
                         UV__SOCK_NONBLOCK|UV__SOCK_CLOEXEC);
    if (peerfd != -1)
      return peerfd;

    if (errno == EINTR)
      continue;

    if (errno != ENOSYS)
      return -errno;

    no_accept4 = 1;
skip:
#endif

    peerfd = accept(sockfd, NULL, NULL);
    if (peerfd == -1) {
      if (errno == EINTR)
        continue;
      return -errno;
    }

    err = uv__cloexec(peerfd, 1);
    if (err == 0)
      err = uv__nonblock(peerfd, 1);

    if (err) {
      uv__close(peerfd);
      return err;
    }

    return peerfd;
  }
}


int uv__close_nocheckstdio(int fd) {
  int saved_errno;
  int rc;

  assert(fd > -1);  /* Catch uninitialized io_watcher.fd bugs. */

  saved_errno = errno;
  rc = close(fd);
  if (rc == -1) {
    rc = -errno;
    if (rc == -EINTR || rc == -EINPROGRESS)
      rc = 0;    /* The close is in progress, not an error. */
    errno = saved_errno;
  }

  return rc;
}


int uv__close(int fd) {
#if !defined(__NUTTX__) && !defined(__TIZENRT__) /* No STDERR_FILENO in nuttx */
  assert(fd > STDERR_FILENO);  /* Catch stdio close bugs. */
#endif
  return uv__close_nocheckstdio(fd);
}


#if defined(_AIX) || \
    defined(__APPLE__) || \
    defined(__DragonFly__) || \
    defined(__FreeBSD__) || \
    defined(__FreeBSD_kernel__) || \
    defined(__linux__)
int uv__nonblock_ioctl(int fd, int set) {
  int r;

  do
    r = ioctl(fd, FIONBIO, &set);
  while (r == -1 && errno == EINTR);

  if (r)
    return -errno;

  return 0;
}


int uv__cloexec_ioctl(int fd, int set) {
  int r;

  do
    r = ioctl(fd, set ? FIOCLEX : FIONCLEX);
  while (r == -1 && errno == EINTR);

  if (r)
    return -errno;

  return 0;
}
#endif

int uv__nonblock_fcntl(int fd, int set) {
  int flags;
  int r;

  do
    r = fcntl(fd, F_GETFL);
  while (r == -1 && errno == EINTR);

  if (r == -1)
    return -errno;

  /* Bail out now if already set/clear. */
  if (!!(r & O_NONBLOCK) == !!set)
    return 0;

  if (set)
    flags = r | O_NONBLOCK;
  else
    flags = r & ~O_NONBLOCK;

  do
    r = fcntl(fd, F_SETFL, flags);
  while (r == -1 && errno == EINTR);

  if (r)
    return -errno;

  return 0;
}


int uv__cloexec_fcntl(int fd, int set) {
#if defined(__NUTTX__) || defined(__TIZENRT__)
  return 0;
#endif
  int flags;
  int r;

  do
    r = fcntl(fd, F_GETFD);
  while (r == -1 && errno == EINTR);

  if (r == -1)
    return -errno;

  /* Bail out now if already set/clear. */
  if (!!(r & FD_CLOEXEC) == !!set)
    return 0;

  if (set)
    flags = r | FD_CLOEXEC;
  else
    flags = r & ~FD_CLOEXEC;

  do
    r = fcntl(fd, F_SETFD, flags);
  while (r == -1 && errno == EINTR);

  if (r)
    return -errno;

  return 0;
}


#if defined(__NUTTX__) || defined(__TIZENRT__)
ssize_t uv__recvmsg(int fd, struct msghdr* msg, int flags) {
  return -1;
}
#else
ssize_t uv__recvmsg(int fd, struct msghdr* msg, int flags) {
  struct cmsghdr* cmsg;
  ssize_t rc;
  int* pfd;
  int* end;
#if defined(__linux__)
  static int no_msg_cmsg_cloexec;
  if (no_msg_cmsg_cloexec == 0) {
    rc = recvmsg(fd, msg, flags | 0x40000000);  /* MSG_CMSG_CLOEXEC */
    if (rc != -1)
      return rc;
    if (errno != EINVAL)
      return -errno;
    rc = recvmsg(fd, msg, flags);
    if (rc == -1)
      return -errno;
    no_msg_cmsg_cloexec = 1;
  } else {
    rc = recvmsg(fd, msg, flags);
  }
#else
  rc = recvmsg(fd, msg, flags);
#endif
  if (rc == -1)
    return -errno;
  if (msg->msg_controllen == 0)
    return rc;
  for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg))
    if (cmsg->cmsg_type == SCM_RIGHTS)
      for (pfd = (int*) CMSG_DATA(cmsg),
           end = (int*) ((char*) cmsg + cmsg->cmsg_len);
           pfd < end;
           pfd += 1)
        uv__cloexec(*pfd, 1);
  return rc;
}
#endif /* End of !NUTTX */


int uv_cwd(char* buffer, size_t* size) {
  if (buffer == NULL || size == NULL)
    return -EINVAL;

  if (getcwd(buffer, *size) == NULL)
    return -errno;

  *size = strlen(buffer);
  if (*size > 1 && buffer[*size - 1] == '/') {
    buffer[*size-1] = '\0';
    (*size)--;
  }

  return 0;
}


int uv_chdir(const char* dir) {
  if (chdir(dir))
    return -errno;

  return 0;
}


static int uv__run_pending(uv_loop_t* loop) {
  QUEUE* q;
  QUEUE pq;
  uv__io_t* w;

  if (QUEUE_EMPTY(&loop->pending_queue))
    return 0;

  QUEUE_MOVE(&loop->pending_queue, &pq);

  while (!QUEUE_EMPTY(&pq)) {
    q = QUEUE_HEAD(&pq);
    QUEUE_REMOVE(q);
    QUEUE_INIT(q);
    w = QUEUE_DATA(q, uv__io_t, pending_queue);
    w->cb(loop, w, POLLOUT);
  }

  return 1;
}


static unsigned int next_power_of_two(unsigned int val) {
  val -= 1;
  val |= val >> 1;
  val |= val >> 2;
  val |= val >> 4;
  val |= val >> 8;
  val |= val >> 16;
  val += 1;
  return val;
}

static void maybe_resize(uv_loop_t* loop, unsigned int len) {
  uv__io_t** watchers;
  void* fake_watcher_list;
  void* fake_watcher_count;
  unsigned int nwatchers;
  unsigned int i;

  if (len <= loop->nwatchers)
    return;

  /* Preserve fake watcher list and count at the end of the watchers */
  if (loop->watchers != NULL) {
    fake_watcher_list = loop->watchers[loop->nwatchers];
    fake_watcher_count = loop->watchers[loop->nwatchers + 1];
  } else {
    fake_watcher_list = NULL;
    fake_watcher_count = NULL;
  }

  nwatchers = next_power_of_two(len + 2) - 2;
  watchers = uv__realloc(loop->watchers,
                         (nwatchers + 2) * sizeof(loop->watchers[0]));

  if (watchers == NULL)
    abort();
  for (i = loop->nwatchers; i < nwatchers; i++)
    watchers[i] = NULL;
  watchers[nwatchers] = fake_watcher_list;
  watchers[nwatchers + 1] = fake_watcher_count;

  loop->watchers = watchers;
  loop->nwatchers = nwatchers;
}


void uv__io_init(uv__io_t* w, uv__io_cb cb, int fd) {
  assert(cb != NULL);
  assert(fd >= -1);
  QUEUE_INIT(&w->pending_queue);
  QUEUE_INIT(&w->watcher_queue);
  w->cb = cb;
  w->fd = fd;
  w->events = 0;
  w->pevents = 0;

#if defined(UV_HAVE_KQUEUE)
  w->rcount = 0;
  w->wcount = 0;
#endif /* defined(UV_HAVE_KQUEUE) */
}


void uv__io_start(uv_loop_t* loop, uv__io_t* w, unsigned int events) {
  assert(0 == (events & ~(POLLIN | POLLOUT | UV__POLLRDHUP)));
  assert(0 != events);
  assert(w->fd >= 0);
  assert(w->fd < INT_MAX);

  w->pevents |= events;
  maybe_resize(loop, w->fd + 1);

#if !defined(__sun)
  /* The event ports backend needs to rearm all file descriptors on each and
   * every tick of the event loop but the other backends allow us to
   * short-circuit here if the event mask is unchanged.
   */
  if (w->events == w->pevents) {
    if (w->events == 0 && !QUEUE_EMPTY(&w->watcher_queue)) {
      QUEUE_REMOVE(&w->watcher_queue);
      QUEUE_INIT(&w->watcher_queue);
    }
    return;
  }
#endif

  if (QUEUE_EMPTY(&w->watcher_queue))
    QUEUE_INSERT_TAIL(&loop->watcher_queue, &w->watcher_queue);

  if (loop->watchers[w->fd] == NULL) {
    loop->watchers[w->fd] = w;
    loop->nfds++;
  }
}


void uv__io_stop(uv_loop_t* loop, uv__io_t* w, unsigned int events) {
  assert(0 == (events & ~(POLLIN | POLLOUT | UV__POLLRDHUP)));
  assert(0 != events);

  if (w->fd == -1)
    return;

  assert(w->fd >= 0);

  /* Happens when uv__io_stop() is called on a handle that was never started. */
  if ((unsigned) w->fd >= loop->nwatchers)
    return;

  w->pevents &= ~events;

  if (w->pevents == 0) {
    QUEUE_REMOVE(&w->watcher_queue);
    QUEUE_INIT(&w->watcher_queue);

    if (loop->watchers[w->fd] != NULL) {
      assert(loop->watchers[w->fd] == w);
      assert(loop->nfds > 0);
      loop->watchers[w->fd] = NULL;
      loop->nfds--;
      w->events = 0;
    }
  }
  else if (QUEUE_EMPTY(&w->watcher_queue))
    QUEUE_INSERT_TAIL(&loop->watcher_queue, &w->watcher_queue);
}


void uv__io_close(uv_loop_t* loop, uv__io_t* w) {
  uv__io_stop(loop, w, POLLIN | POLLOUT | UV__POLLRDHUP);
  QUEUE_REMOVE(&w->pending_queue);

  /* Remove stale events for this file descriptor */
  uv__platform_invalidate_fd(loop, w->fd);
}


void uv__io_feed(uv_loop_t* loop, uv__io_t* w) {
  if (QUEUE_EMPTY(&w->pending_queue))
    QUEUE_INSERT_TAIL(&loop->pending_queue, &w->pending_queue);
}


int uv__io_active(const uv__io_t* w, unsigned int events) {
  assert(0 == (events & ~(POLLIN | POLLOUT | UV__POLLRDHUP)));
  assert(0 != events);
  return 0 != (w->pevents & events);
}


int uv__open_cloexec(const char* path, int flags) {
  int err;
  int fd;

#if defined(UV__O_CLOEXEC)
  static int no_cloexec;

  if (!no_cloexec) {
    fd = open(path, flags | UV__O_CLOEXEC);
    if (fd != -1)
      return fd;

    if (errno != EINVAL)
      return -errno;

    /* O_CLOEXEC not supported. */
    no_cloexec = 1;
  }
#endif

  fd = open(path, flags);
  if (fd == -1)
    return -errno;

  err = uv__cloexec(fd, 1);
  if (err) {
    uv__close(fd);
    return err;
  }

  return fd;
}


int uv_os_getpriority(uv_pid_t pid, int* priority) {
  int r;
  if (priority == NULL)
    return UV_EINVAL;
  errno = 0;
  r = getpriority(PRIO_PROCESS, (int) pid);
  if (r == -1 && errno != 0)
    return UV__ERR(errno);
  *priority = r;
  return 0;
}


int uv_os_setpriority(uv_pid_t pid, int priority) {
  if (priority < UV_PRIORITY_HIGHEST || priority > UV_PRIORITY_LOW)
    return UV_EINVAL;
  if (setpriority(PRIO_PROCESS, (int) pid, priority) != 0)
    return UV__ERR(errno);
  return 0;
}

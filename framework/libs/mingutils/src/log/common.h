#pragma once

#include <stdint.h>
#include <pthread.h>
#include <stdarg.h>
#include "rlog.h"

typedef struct {
  char* buffer;
  uint32_t size;
} BuiltinLogBuffer;

#define MAX_TCP_SOCKET_CONN 8
typedef struct {
  BuiltinLogBuffer* log_buffer;
  pthread_mutex_t write_mutex;
  pthread_mutex_t init_mutex;
  int cli_sockets[MAX_TCP_SOCKET_CONN];
  int listen_fd;
} BuiltinTCPSocketInst;

int32_t std_log_writer(RokidLogLevel lv, const char* tag,
    const char* fmt, va_list ap, void* arg1, void* arg2);
int32_t tcp_socket_log_writer(RokidLogLevel lv, const char* tag,
    const char* fmt, va_list ap, void* arg1, void* arg2);
int32_t start_tcp_listen(void* arg1, void* arg2);

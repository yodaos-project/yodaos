/* Copyright 2015-present Samsung Electronics Co., Ltd. and other contributors
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

#include "iotjs_def.h"
#include "iotjs_module.h"
#include "mbedtls/ssl.h"

#define NODE_DEFINE_CONSTANT(object, constant)                   \
  do {                                                           \
    iotjs_jval_set_property_number(object, #constant, constant); \
  } while (0)

#ifdef MBEDTLS_SSL_MAX_CONTENT_LEN
#define TLS_CHUNK_MAX_SIZE MBEDTLS_SSL_MAX_CONTENT_LEN
#else
#define TLS_CHUNK_MAX_SIZE INT_MAX
#endif

#define INT32_MAX_VALUE UINT32_MAX

void DefineSignalConstants(jerry_value_t target) {
#ifdef SIGHUP
  NODE_DEFINE_CONSTANT(target, SIGHUP);
#endif

#ifdef SIGINT
  NODE_DEFINE_CONSTANT(target, SIGINT);
#endif

#ifdef SIGQUIT
  NODE_DEFINE_CONSTANT(target, SIGQUIT);
#endif

#ifdef SIGILL
  NODE_DEFINE_CONSTANT(target, SIGILL);
#endif

#ifdef SIGTRAP
  NODE_DEFINE_CONSTANT(target, SIGTRAP);
#endif

#ifdef SIGABRT
  NODE_DEFINE_CONSTANT(target, SIGABRT);
#endif

#ifdef SIGIOT
  NODE_DEFINE_CONSTANT(target, SIGIOT);
#endif

#ifdef SIGBUS
  NODE_DEFINE_CONSTANT(target, SIGBUS);
#endif

#ifdef SIGFPE
  NODE_DEFINE_CONSTANT(target, SIGFPE);
#endif

#ifdef SIGKILL
  NODE_DEFINE_CONSTANT(target, SIGKILL);
#endif

#ifdef SIGUSR1
  NODE_DEFINE_CONSTANT(target, SIGUSR1);
#endif

#ifdef SIGSEGV
  NODE_DEFINE_CONSTANT(target, SIGSEGV);
#endif

#ifdef SIGUSR2
  NODE_DEFINE_CONSTANT(target, SIGUSR2);
#endif

#ifdef SIGPIPE
  NODE_DEFINE_CONSTANT(target, SIGPIPE);
#endif

#ifdef SIGALRM
  NODE_DEFINE_CONSTANT(target, SIGALRM);
#endif

  NODE_DEFINE_CONSTANT(target, SIGTERM);

#ifdef SIGCHLD
  NODE_DEFINE_CONSTANT(target, SIGCHLD);
#endif

#ifdef SIGSTKFLT
  NODE_DEFINE_CONSTANT(target, SIGSTKFLT);
#endif

#ifdef SIGCONT
  NODE_DEFINE_CONSTANT(target, SIGCONT);
#endif

#ifdef SIGSTOP
  NODE_DEFINE_CONSTANT(target, SIGSTOP);
#endif

#ifdef SIGTSTP
  NODE_DEFINE_CONSTANT(target, SIGTSTP);
#endif

#ifdef SIGBREAK
  NODE_DEFINE_CONSTANT(target, SIGBREAK);
#endif

#ifdef SIGTTIN
  NODE_DEFINE_CONSTANT(target, SIGTTIN);
#endif

#ifdef SIGTTOU
  NODE_DEFINE_CONSTANT(target, SIGTTOU);
#endif

#ifdef SIGURG
  NODE_DEFINE_CONSTANT(target, SIGURG);
#endif

#ifdef SIGXCPU
  NODE_DEFINE_CONSTANT(target, SIGXCPU);
#endif

#ifdef SIGXFSZ
  NODE_DEFINE_CONSTANT(target, SIGXFSZ);
#endif

#ifdef SIGVTALRM
  NODE_DEFINE_CONSTANT(target, SIGVTALRM);
#endif

#ifdef SIGPROF
  NODE_DEFINE_CONSTANT(target, SIGPROF);
#endif

#ifdef SIGWINCH
  NODE_DEFINE_CONSTANT(target, SIGWINCH);
#endif

#ifdef SIGIO
  NODE_DEFINE_CONSTANT(target, SIGIO);
#endif

#ifdef SIGPOLL
  NODE_DEFINE_CONSTANT(target, SIGPOLL);
#endif

#ifdef SIGLOST
  NODE_DEFINE_CONSTANT(target, SIGLOST);
#endif

#ifdef SIGPWR
  NODE_DEFINE_CONSTANT(target, SIGPWR);
#endif

#ifdef SIGINFO
  NODE_DEFINE_CONSTANT(target, SIGINFO);
#endif

#ifdef SIGSYS
  NODE_DEFINE_CONSTANT(target, SIGSYS);
#endif

#ifdef SIGUNUSED
  NODE_DEFINE_CONSTANT(target, SIGUNUSED);
#endif
}

void DefinePriorityConstants(jerry_value_t target) {
  NODE_DEFINE_CONSTANT(target, UV_PRIORITY_LOW);
  NODE_DEFINE_CONSTANT(target, UV_PRIORITY_HIGHEST);
}

jerry_value_t InitConstants() {
  jerry_value_t exports = jerry_create_object();
  jerry_value_t os = jerry_create_object();
  iotjs_jval_set_property_jval(exports, "os", os);

  NODE_DEFINE_CONSTANT(exports, O_APPEND);
  NODE_DEFINE_CONSTANT(exports, O_CREAT);
  NODE_DEFINE_CONSTANT(exports, O_EXCL);
  NODE_DEFINE_CONSTANT(exports, O_RDONLY);
  NODE_DEFINE_CONSTANT(exports, O_RDWR);
  NODE_DEFINE_CONSTANT(exports, O_SYNC);
  NODE_DEFINE_CONSTANT(exports, O_TRUNC);
  NODE_DEFINE_CONSTANT(exports, O_WRONLY);
  NODE_DEFINE_CONSTANT(exports, S_IFMT);
  NODE_DEFINE_CONSTANT(exports, S_IFDIR);
  NODE_DEFINE_CONSTANT(exports, S_IFIFO);
  NODE_DEFINE_CONSTANT(exports, S_IFREG);
  NODE_DEFINE_CONSTANT(exports, S_IFLNK);
  NODE_DEFINE_CONSTANT(exports, S_IFSOCK);
  NODE_DEFINE_CONSTANT(exports, TLS_CHUNK_MAX_SIZE);
  NODE_DEFINE_CONSTANT(exports, INT32_MAX_VALUE);
  // define uv errnos
#define V(name, _) NODE_DEFINE_CONSTANT(exports, UV_##name);
  UV_ERRNO_MAP(V)
#undef V

  // define uv priority constants
  DefinePriorityConstants(exports);
  jerry_value_t signals = jerry_create_object();
  DefineSignalConstants(signals);

  iotjs_jval_set_property_jval(os, "signals", signals);
  jerry_release_value(signals);
  jerry_release_value(os);
  return exports;
}

/* Copyright 2015 Samsung Electronics Co., Ltd.
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
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>
#include "runner.h"

static uv_loop_t loop;

static int server_closed;
static stream_type serverType;
static uv_tcp_t tcpServer;
static uv_handle_t* serverHandle;


//-----------------------------------------------------------------------------

static void on_close(uv_handle_t* peer) {
  free(peer);
}

static void on_server_close(uv_handle_t* handle) {
  TUV_ASSERT(handle == serverHandle);
}


//-----------------------------------------------------------------------------

static void after_write(uv_write_t* req, int status) {
  write_req_t* wr;

  /* Free the read/write buffer and the request */
  wr = (write_req_t*) req;
  free(wr->buf.base);
  free(wr);

  if (status == 0)
    return;

  TDLOG("uv_write error: %s - %s", uv_err_name(status), uv_strerror(status));
}


static void after_shutdown(uv_shutdown_t* req, int status) {
  uv_close((uv_handle_t*) req->handle, on_close);
  free(req);
}


static void after_read(uv_stream_t* handle,
                       ssize_t nread,
                       const uv_buf_t* buf) {
  int i;
  write_req_t *wr;
  uv_shutdown_t* sreq;

  if (nread < 0) {
    /* Error or EOF */
    TUV_ASSERT(nread == UV_EOF);

    free(buf->base);
    sreq = (uv_shutdown_t*)malloc(sizeof(uv_shutdown_t));
    memset(sreq, 0, sizeof(uv_shutdown_t));
    TUV_ASSERT(0 == uv_shutdown(sreq, handle, after_shutdown));
    return;
  }

  if (nread == 0) {
    /* Everything OK, but nothing read. */
    free(buf->base);
    return;
  }

  /*
   * Scan for the letter Q which signals that we should quit the server.
   * If we get QS it means close the stream.
   */
  if (!server_closed) {
    for (i = 0; i < nread; i++) {
      if (buf->base[i] == 'Q') {
        if (i + 1 < nread && buf->base[i + 1] == 'S') {
          free(buf->base);
          uv_close((uv_handle_t*)handle, on_close);
          return;
        } else {
          free(buf->base);
          uv_close((uv_handle_t*)handle, on_close);
          uv_close(serverHandle, on_server_close);
          server_closed = 1;
          //break;
          return;
        }
      }
    }
  }

  wr = (write_req_t*)malloc(sizeof(write_req_t));
  TUV_ASSERT(wr != NULL);
  wr->buf = uv_buf_init(buf->base, nread);

  if (uv_write(&wr->req, handle, &wr->buf, 1, after_write)) {
    TUV_FATAL("uv_write failed");
  }
}


static void echo_alloc(uv_handle_t* handle,
                       size_t suggested_size,
                       uv_buf_t* buf) {
  buf->base = (char*)malloc(suggested_size);
  buf->len = suggested_size;
}


static void on_connection(uv_stream_t* server, int status) {
  uv_stream_t* stream;
  int r;

  if (status != 0) {
    TDLOG("echo server on_connection, Connect error %d, %s",
          status, uv_err_name(status));
    return;
  }
  TUV_ASSERT(status == 0);

  switch (serverType) {
  case TEST_TCP:
    stream = (uv_stream_t*)malloc(sizeof(uv_tcp_t));
    TUV_ASSERT(stream != NULL);
    r = uv_tcp_init(&loop, (uv_tcp_t*)stream);
    TUV_ASSERT(r == 0);
    break;

/*
  case PIPE:
    stream = (uv_stream_t*)malloc(sizeof(uv_pipe_t));
    TUV_ASSERT(stream != NULL);
    r = uv_pipe_init(loop, (uv_pipe_t*)stream, 0);
    TUV_ASSERT(r == 0);
    break;
*/

  default:
    TUV_ASSERT(0 && "Bad serverType");
    ABORT();
  }

  /* associate server with stream */
  stream->data = server;

  r = uv_accept(server, stream);
  TUV_ASSERT(r == 0);

  r = uv_read_start(stream, echo_alloc, after_read);
  TUV_ASSERT(r == 0);

#if defined(__HOST_HELPER__)
  struct sockaddr_in tcpname;
  int namelen;
  namelen = sizeof(struct sockaddr_in);
  uv_tcp_getpeername(stream, &tcpname, &namelen);
  union {
    uint8_t addr8[4];
    uint32_t addr32;
  } c4;
  c4.addr32 = tcpname.sin_addr.s_addr;
  TDDDLOG("Client has connected: %d.%d.%d.%d",
          c4.addr8[0], c4.addr8[1], c4.addr8[2], c4.addr8[3]);
#endif
}


//-----------------------------------------------------------------------------

static int tcp4_echo_start(int port) {
  struct sockaddr_in addr;
  int r;

  TUV_ASSERT(0 == uv_ip4_addr("0.0.0.0", port, &addr));

  serverHandle = (uv_handle_t*)&tcpServer;
  serverType = TEST_TCP;

  r = uv_tcp_init(&loop, &tcpServer);
  if (r) {
    /* TODO: Error codes */
    TDLOG("svr Socket creation error");
    return 1;
  }

  r = uv_tcp_bind(&tcpServer, (const struct sockaddr*) &addr, 0);
  if (r) {
    /* TODO: Error codes */
    TDLOG("svr Bind error");
    return 1;
  }

  r = uv_listen((uv_stream_t*)&tcpServer, SOMAXCONN, on_connection);
  if (r) {
    /* TODO: Error codes */
    TDLOG("svr Listen error %s", uv_err_name(r));
    return 1;
  }

  return 0;
}


//-----------------------------------------------------------------------------

HELPER_IMPL(tcp4_echo_server) {
  int r;

  server_closed = 0;
  serverHandle = NULL;

  r = uv_loop_init(&loop);
  TUV_ASSERT(r == 0);

  if (tcp4_echo_start(TEST_PORT))
    return 1;

  r = uv_run(&loop, UV_RUN_DEFAULT);
  TUV_ASSERT(r == 0);

  assert(server_closed != 0);

  TUV_ASSERT(uv_loop_close(&loop) == 0);

  return 0;
}


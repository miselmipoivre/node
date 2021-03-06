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

#include <assert.h>

#include "uv.h"
#include "../uv-common.h"
#include "internal.h"


void uv_stream_init(uv_stream_t* handle) {
  handle->write_queue_size = 0;
  handle->flags = 0;
  handle->error = uv_ok_;

  uv_counters()->handle_init++;
  uv_counters()->stream_init++;

  uv_ref();
}


void uv_connection_init(uv_stream_t* handle) {
  handle->flags |= UV_HANDLE_CONNECTION;
  handle->write_reqs_pending = 0;

  uv_req_init((uv_req_t*) &(handle->read_req));
  handle->read_req.type = UV_READ;
  handle->read_req.data = handle;
}


int uv_accept(uv_handle_t* server, uv_stream_t* client) {
  assert(client->type == server->type);

  if (server->type == UV_TCP) {
    return uv_tcp_accept((uv_tcp_t*)server, (uv_tcp_t*)client);
  } else if (server->type == UV_NAMED_PIPE) {
    return uv_pipe_accept((uv_pipe_t*)server, (uv_pipe_t*)client);
  }

  return -1;
}


int uv_read_start(uv_stream_t* handle, uv_alloc_cb alloc_cb, uv_read_cb read_cb) {
  if (handle->type == UV_TCP) {
    return uv_tcp_read_start((uv_tcp_t*)handle, alloc_cb, read_cb);
  } else if (handle->type == UV_NAMED_PIPE) {
    return uv_pipe_read_start((uv_pipe_t*)handle, alloc_cb, read_cb);
  }

  return -1;
}


int uv_read_stop(uv_stream_t* handle) {
  handle->flags &= ~UV_HANDLE_READING;

  return 0;
}


int uv_write(uv_write_t* req, uv_stream_t* handle, uv_buf_t bufs[], int bufcnt,
    uv_write_cb cb) {
  if (handle->type == UV_TCP) {
    return uv_tcp_write(req, (uv_tcp_t*) handle, bufs, bufcnt, cb);
  } else if (handle->type == UV_NAMED_PIPE) {
    return uv_pipe_write(req, (uv_pipe_t*) handle, bufs, bufcnt, cb);
  }

  uv_set_sys_error(WSAEINVAL);
  return -1;
}


int uv_shutdown(uv_shutdown_t* req, uv_stream_t* handle, uv_shutdown_cb cb) {
  if (!(handle->flags & UV_HANDLE_CONNECTION)) {
    uv_set_sys_error(WSAEINVAL);
    return -1;
  }

  if (handle->flags & UV_HANDLE_SHUTTING) {
    uv_set_sys_error(WSAESHUTDOWN);
    return -1;
  }

  uv_req_init((uv_req_t*) req);
  req->type = UV_SHUTDOWN;
  req->handle = handle;
  req->cb = cb;

  handle->flags |= UV_HANDLE_SHUTTING;
  handle->shutdown_req = req;
  handle->reqs_pending++;

  uv_want_endgame((uv_handle_t*)handle);

  return 0;
}


size_t uv_count_bufs(uv_buf_t bufs[], int count) {
  size_t bytes = 0;
  int i;

  for (i = 0; i < count; i++) {
    bytes += (size_t)bufs[i].len;
  }

  return bytes;
}

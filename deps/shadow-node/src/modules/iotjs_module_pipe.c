
#include "iotjs_module_pipe.h"
#include "iotjs_module_buffer.h"
#include <stdlib.h>

IOTJS_DEFINE_NATIVE_HANDLE_INFO_THIS_MODULE(pipewrap);

typedef struct iotjs_write_wrap {
  char* data;
  uv_write_t req;
  jerry_value_t callback;
} iotjs_write_wrap;

static void iotjs_pipe_on_connection(uv_stream_t* handle, int status) {
  printf("connected\n");
}

static iotjs_pipewrap_t* iotjs_pipewrap_create(const jerry_value_t value) {
  iotjs_pipewrap_t* pipewrap = IOTJS_ALLOC(iotjs_pipewrap_t);
  IOTJS_VALIDATED_STRUCT_CONSTRUCTOR(iotjs_pipewrap_t, pipewrap);
  iotjs_handlewrap_initialize(&_this->handlewrap, value,
                              (uv_handle_t*)(&_this->handle),
                              &this_module_native_info);
  return pipewrap;
}

static void iotjs_pipewrap_destroy(iotjs_pipewrap_t* pipewrap) {
  IOTJS_VALIDATED_STRUCT_DESTRUCTOR(iotjs_pipewrap_t, pipewrap);
  iotjs_handlewrap_destroy(&_this->handlewrap);
  IOTJS_RELEASE(pipewrap);
}

iotjs_pipewrap_t* iotjs_pipewrap_from_handle(uv_pipe_t* pipe) {
  uv_handle_t* handle = (uv_handle_t*)(pipe);
  iotjs_handlewrap_t* handlewrap = iotjs_handlewrap_from_handle(handle);
  iotjs_pipewrap_t* wrap = (iotjs_pipewrap_t*)handlewrap;
  IOTJS_ASSERT(iotjs_pipewrap_get_handle(wrap) == pipe);
  return wrap;
}

iotjs_pipewrap_t* iotjs_pipewrap_from_jobject(jerry_value_t jval) {
  iotjs_handlewrap_t* handlewrap = iotjs_handlewrap_from_jobject(jval);
  return (iotjs_pipewrap_t*)handlewrap;
}

uv_pipe_t* iotjs_pipewrap_get_handle(iotjs_pipewrap_t* wrap) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, wrap);
  uv_handle_t* handle = iotjs_handlewrap_get_uv_handle(&_this->handlewrap);
  return (uv_pipe_t*)handle;
}

jerry_value_t iotjs_pipewrap_get_jobject(iotjs_pipewrap_t* wrap) {
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, wrap);
  return iotjs_handlewrap_jobject(&_this->handlewrap);
}

JS_FUNCTION(PipeConstructor) {
  DJS_CHECK_THIS();
  const jerry_value_t self = JS_GET_THIS();
  iotjs_pipewrap_t* pipewrap = iotjs_pipewrap_create(self);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  uv_loop_t* loop = iotjs_environment_loop(iotjs_environment_get());
  bool ipc = false;
  if (jargc >= 1 && jerry_value_is_boolean(jargv[0])) {
    ipc = jerry_get_boolean_value(jargv[0]);
  }
  int r = uv_pipe_init(loop, &_this->handle, ipc);
  if (r != 0) {
    return JS_CREATE_ERROR(COMMON, "failed to create pipe handle.");
  }
  return jerry_create_undefined();
}

JS_FUNCTION(PipeBind) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  iotjs_string_t sock_name = JS_GET_ARG(0, string);
  int r = uv_pipe_bind(&_this->handle, iotjs_string_data(&sock_name));
  return jerry_create_number(r);
}

JS_FUNCTION(PipeOpen) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  int fd = (int)jerry_get_number_value(jargv[0]);
  int r = uv_pipe_open(&_this->handle, fd);
  if (r != 0) {
    return JS_CREATE_ERROR(COMMON, "uv_pipe_open() failed");
  }
  return jerry_create_undefined();
}

JS_FUNCTION(PipeListen) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  int backlog = (int)jerry_get_number_value(jargv[0]);
  int r = uv_listen((uv_stream_t*)&_this->handle, backlog,
                    iotjs_pipe_on_connection);
  return jerry_create_number(r);
}

void iotjs_pipe_allocator(uv_handle_t* handle, size_t suggested_size,
                          uv_buf_t* buf) {
  if (suggested_size > IOTJS_MAX_READ_BUFFER_SIZE) {
    suggested_size = IOTJS_MAX_READ_BUFFER_SIZE;
  }
  buf->base = iotjs_buffer_allocate(suggested_size);
  buf->len = suggested_size;
}

void iotjs_pipe_onread(uv_stream_t* stream, ssize_t nread,
                       const uv_buf_t* buf) {
  iotjs_pipewrap_t* wrap = iotjs_pipewrap_from_handle((uv_pipe_t*)stream);
  jerry_value_t jthis = iotjs_pipewrap_get_jobject(wrap);
  jerry_value_t jsocket =
      iotjs_jval_get_property(jthis, IOTJS_MAGIC_STRING_OWNER);
  jerry_value_t fn = iotjs_jval_get_property(jthis, "onread");

  iotjs_jargs_t jargs = iotjs_jargs_create(4);
  iotjs_jargs_append_jval(&jargs, jsocket);
  iotjs_jargs_append_number(&jargs, nread);
  iotjs_jargs_append_bool(&jargs, false);

  if (nread <= 0) {
    if (buf->base != NULL) {
      iotjs_buffer_release(buf->base);
    }
    if (nread < 0) {
      if (nread == UV__EOF) {
        iotjs_jargs_replace(&jargs, 2, jerry_create_boolean(true));
      }
      iotjs_make_callback(fn, jthis, &jargs);
    }
  } else {
    jerry_value_t jbuffer = iotjs_bufferwrap_create_buffer((size_t)nread);
    iotjs_bufferwrap_t* buffer_wrap = iotjs_bufferwrap_from_jbuffer(jbuffer);

    iotjs_bufferwrap_copy(buffer_wrap, buf->base, (size_t)nread);
    iotjs_jargs_append_jval(&jargs, jbuffer);
    iotjs_make_callback(fn, jthis, &jargs);

    jerry_release_value(jbuffer);
    iotjs_buffer_release(buf->base);
  }
  iotjs_jargs_destroy(&jargs);
  jerry_release_value(fn);
  jerry_release_value(jsocket);
}

JS_FUNCTION(PipeReadStart) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  uv_read_start((uv_stream_t*)&_this->handle, iotjs_pipe_allocator,
                iotjs_pipe_onread);
  return jerry_create_undefined();
}

// JS_FUNCTION(PipeConnect) {
//   // TODO
//   return jerry_create_undefined();
// }

// void iotjs_pipewrap_write_cb(uv_write_t* req, int status) {
//   printf("write done with status: %d\n", status);
// }

static void iotjs_pipe_after_write(uv_write_t* req, int status) {
  iotjs_write_wrap* write_wrap = (iotjs_write_wrap*)req->data;
  char* data = write_wrap->data;
  jerry_value_t callback = write_wrap->callback;
  if (jerry_value_is_function(callback)) {
    iotjs_jargs_t jargs = iotjs_jargs_create(1);
    if (status) {
      const char* uv_err = uv_strerror(status);
      iotjs_jargs_append_error(&jargs, uv_err);
    } else {
      iotjs_jargs_append_undefined(&jargs);
    }
    iotjs_make_callback(callback, jerry_create_undefined(), &jargs);
    iotjs_jargs_destroy(&jargs);
  }
  IOTJS_RELEASE(data);
  jerry_release_value(callback);
  IOTJS_RELEASE(write_wrap);
}

#define PIPE_DO_WRITE(_this, chunk, size, callback)              \
  iotjs_write_wrap* write_wrap = IOTJS_ALLOC(iotjs_write_wrap);  \
  write_wrap->data = iotjs_buffer_allocate(sizeof(char) * size); \
  memcpy(write_wrap->data, chunk, size);                         \
  write_wrap->callback = jerry_acquire_value(callback);          \
  write_wrap->req.data = write_wrap;                             \
  uv_stream_t* handle = (uv_stream_t*)&_this->handle;            \
  uv_buf_t buf = uv_buf_init(write_wrap->data, size);            \
  uv_write(&write_wrap->req, handle, &buf, 1, iotjs_pipe_after_write);

JS_FUNCTION(Write) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  iotjs_bufferwrap_t* wrap = iotjs_bufferwrap_from_jbuffer(jargv[0]);
  char* chunk = iotjs_bufferwrap_buffer(wrap);
  size_t size = iotjs_bufferwrap_length(wrap);

  jerry_value_t callback = JS_GET_ARG_IF_EXIST(1, function);

  PIPE_DO_WRITE(_this, chunk, size, callback);

  return jerry_create_undefined();
}

JS_FUNCTION(WriteUtf8String) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  iotjs_string_t data = JS_GET_ARG(0, string);
  const char* chunk = iotjs_string_data(&data);
  unsigned int size = iotjs_string_size(&data);

  jerry_value_t callback = JS_GET_ARG_IF_EXIST(1, function);

  PIPE_DO_WRITE(_this, chunk, size, callback);

  // free the data firstly
  iotjs_string_destroy(&data);

  return jerry_create_undefined();
}

// Socket close result handler.
void iotjs_pipewrap_after_close(uv_handle_t* handle) {
  iotjs_handlewrap_t* wrap = iotjs_handlewrap_from_handle(handle);
  jerry_value_t jthis = iotjs_handlewrap_jobject(wrap);

  // callback function.
  jerry_value_t jcallback = iotjs_jval_get_property(jthis, "onclose");
  if (jerry_value_is_function(jcallback)) {
    iotjs_make_callback(jcallback, jerry_create_undefined(),
                        iotjs_jargs_get_empty());
  }
  jerry_release_value(jcallback);
}

JS_FUNCTION(PipeClose) {
  JS_DECLARE_THIS_PTR(handlewrap, wrap);
  iotjs_handlewrap_close(wrap, iotjs_pipewrap_after_close);
  return jerry_create_undefined();
}

JS_FUNCTION(PipeRef) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  uv_ref((uv_handle_t*)&_this->handle);
  return jerry_create_undefined();
}

JS_FUNCTION(PipeUnref) {
  JS_DECLARE_THIS_PTR(pipewrap, pipewrap);
  IOTJS_VALIDATED_STRUCT_METHOD(iotjs_pipewrap_t, pipewrap);

  uv_unref((uv_handle_t*)&_this->handle);
  return jerry_create_undefined();
}

jerry_value_t InitPipe() {
  jerry_value_t pipewrap = jerry_create_object();
  jerry_value_t pipeConstructor =
      jerry_create_external_function(PipeConstructor);
  iotjs_jval_set_property_jval(pipewrap, "Pipe", pipeConstructor);

  jerry_value_t proto = jerry_create_object();
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_BIND, PipeBind);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_OPEN, PipeOpen);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_LISTEN, PipeListen);
  // iotjs_jval_set_method(proto, "connect", PipeConnect);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_READSTART, PipeReadStart);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_WRITE, Write);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_WRITEUTF8STRING,
                        WriteUtf8String);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_CLOSE, PipeClose);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_REF, PipeRef);
  iotjs_jval_set_method(proto, IOTJS_MAGIC_STRING_UNREF, PipeUnref);
  iotjs_jval_set_property_jval(pipeConstructor, "prototype", proto);

  jerry_release_value(proto);
  jerry_release_value(pipeConstructor);
  return pipewrap;
}

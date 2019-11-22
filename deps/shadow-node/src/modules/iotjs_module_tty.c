#include "iotjs_def.h"
#include <uv.h>
#include <errno.h>
#include <unistd.h>

JS_FUNCTION(Isatty) {
  int fd = JS_GET_ARG(0, number);
  int rc = isatty(fd);
  if (rc == 0 && errno == EBADF) {
    return JS_CREATE_ERROR(COMMON, "fd is invalid descriptor");
  }
  return jerry_create_boolean(rc);
}

jerry_value_t InitTty() {
  jerry_value_t jtty = jerry_create_object();
  iotjs_jval_set_method(jtty, IOTJS_MAGIC_STRING_ISATTY, Isatty);
  return jtty;
}

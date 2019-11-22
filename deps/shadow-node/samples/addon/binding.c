#include <stdlib.h>
#include <stdio.h>
#include <shadow-node/iotjs.h>
#include <shadow-node/iotjs_def.h>
#include <shadow-node/iotjs_binding.h>

JS_FUNCTION(Cal) {
  for (int i = 0; i < 100000; i++) {
    // just no
  }
  return jerry_create_boolean(true);
}

void init(jerry_value_t exports) {
  iotjs_jval_set_property_number(exports, "foobar", 10);
  iotjs_jval_set_method(exports, "cal", Cal);
}

NODE_MODULE(binding, init)

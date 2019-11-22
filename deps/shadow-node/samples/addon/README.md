# Native Addon

This tutorial shows how to write a native module.

## Requirements

- CMake

## Steps

### Write the `CMakeLists.txt` under your project

```cmake
cmake_minimum_required(VERSION 2.8.12)
project(mybinding C)

set(ADDON_HEADERS 
  ../../include/
  ../../src/
  ../../deps/jerry/jerry-core/include
  ../../deps/libtuv/include
)

add_library(mybinding MODULE binding.c)
set_target_properties(mybinding PROPERTIES
  PREFIX "" 
  SUFFIX ".node" 
  OUTPUT_NAME "binding"
  LINK_FLAGS "-undefined dynamic_lookup"
)
target_include_directories(mybinding PUBLIC ${ADDON_HEADERS})
```

Be notice the following:

- must include the `deps/jerry/include`, `src` and `include`, if you are working with async handle by libtuv, please include `deps/libtuv/include`.
- keep the following `PROPERTIES`:
  - PREFIX to be ""
  - SUFFIX to be ".node", we keep consistent with Node.js as well
  - Add the link flag "-undefined dynamic_lookup", which let the build system not check the
    symbols when linking to library.

Note: the "-undefined dynamic_lookup" might be unsafe as I noticed, so be carefully when you
are writing your native module.

### Write your binding source

```c
#include <stdlib.h>
#include <stdio.h>
#include "iotjs.h"
#include "iotjs_def.h"
#include "iotjs_binding.h"

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

NODE_MODULE(mybinding, init)
```

Every native module requires a function `iotjs_module_register(exports)`, which is the entry
function for initializing modules which likes `NODE_MODULE(module, init)`.

### Compile with CMake

```sh
$ cmake ./
$ make
```

### Require & Run

```js
var binding = require('./binding.node');
binding.cal();  // true
```


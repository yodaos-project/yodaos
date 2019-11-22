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

#ifndef IOTJS_IOTJS_H
#define IOTJS_IOTJS_H


#ifdef __cplusplus
#define IOTJS_EXTERN_C extern "C"
#else /* !__cplusplus */
#define IOTJS_EXTERN_C extern
#endif /* !__cplusplus */

#include "jerryscript.h"

IOTJS_EXTERN_C int iotjs_entry(int argc, char** argv);
IOTJS_EXTERN_C void iotjs_module_register(jerry_value_t exports);

#define NODE_MODULE(module_name, entry)               \
  void iotjs_module_register(jerry_value_t exports) { \
    return entry(exports);                            \
  }

#endif /* IOTJS_IOTJS_H */

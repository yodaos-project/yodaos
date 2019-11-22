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
#include "iotjs_exception.h"

#include <stdio.h>

#include "uv.h"


jerry_value_t iotjs_create_uv_exception(int errorno, const char* syscall) {
  static char msg[256];
  const char* err_name = uv_err_name(errorno);
  const char* err_msg = uv_strerror(errorno);

  snprintf(msg, sizeof(msg), "%s %s(%s)", syscall, err_name, err_msg);
  jerry_value_t jmsg = iotjs_jval_create_error(msg);
  iotjs_jval_set_property_string_raw(jmsg, "code", err_name);
  return jmsg;
}

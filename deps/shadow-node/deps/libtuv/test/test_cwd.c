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

#include <uv.h>
#include "runner.h"

#if defined(__NUTTX__) || defined(__TIZENRT__)
# define DIR_BUFF_LENGTH 64
#else
# define DIR_BUFF_LENGTH 512
#endif

static char buffer[DIR_BUFF_LENGTH];

TEST_IMPL(cwd) {
  size_t size;
  int err;

  size = sizeof(buffer);
  err = uv_cwd(buffer, &size);
  TUV_ASSERT(err == 0);
  TDDDLOG("uv_cwd: %s\n", buffer);

  return 0;
}
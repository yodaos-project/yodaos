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
#include "iotjs_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__)) || \
    defined(__APPLE__)
#include <execinfo.h>
#endif // (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__)) ||
       // defined(__APPLE__)

char* iotjs__file_read(const char* path, size_t* outlen) {
  FILE* file = fopen(path, "rb");
  if (file == NULL) {
    *outlen = 0;
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long ftell_ret = ftell(file);
  IOTJS_ASSERT(ftell_ret >= 0);

  size_t len = (size_t)ftell_ret;
  fseek(file, 0, SEEK_SET);

  char* buffer = iotjs_buffer_allocate(len + 1);
  *outlen = len;

#if defined(__NUTTX__) || defined(__TIZENRT__)
  char* ptr = buffer;
  unsigned nread = 0;
  unsigned read = 0;

  while ((nread = fread(ptr, 1, IOTJS_MAX_READ_BUFFER_SIZE, file)) > 0) {
    read += nread;
    ptr = buffer + read;
  }
#else
  size_t read = fread(buffer, 1, len, file);
#endif
  IOTJS_ASSERT(read == len);

  *(buffer + len) = 0;

  fclose(file);
  return buffer;
}


iotjs_string_t iotjs_file_read(const char* path) {
  size_t len = 0;
  char* buffer = iotjs__file_read(path, &len);
  if (buffer == NULL) {
    iotjs_string_t empty_content = iotjs_string_create();
    return empty_content;
  }

  iotjs_string_t contents = iotjs_string_create_with_buffer(buffer, len);
  return contents;
}


char* iotjs_buffer_allocate(size_t size) {
  char* buffer = (char*)(calloc(size, sizeof(char)));
  IOTJS_ASSERT(buffer != NULL);
  return buffer;
}


char* iotjs_buffer_reallocate(char* buffer, size_t size) {
  IOTJS_ASSERT(buffer != NULL);
  return (char*)(realloc(buffer, size));
}


void iotjs_buffer_release(char* buffer) {
  IOTJS_ASSERT(buffer != NULL);
  free(buffer);
}

void print_stacktrace() {
#if (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__)) || \
    defined(__APPLE__)
  void* bt[IOTJS_BACKTRACE_LEN];
  int size = backtrace(bt, IOTJS_BACKTRACE_LEN);
  char** bt_sym_strs = backtrace_symbols(bt, size);
  for (int idx = 0; idx < size; ++idx) {
    fprintf(stderr, "%s\n", bt_sym_strs[idx]);
  }
  free(bt_sym_strs);
#endif // (defined(__linux__) && defined(__GLIBC__) && !defined(__ANDROID__)) ||
       // defined(__APPLE__))
}

void force_terminate() {
  exit(EXIT_FAILURE);
}

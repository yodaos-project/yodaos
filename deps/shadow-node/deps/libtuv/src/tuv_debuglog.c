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

#include <stdio.h>
#include <stdlib.h>

#include "tuv__debuglog.h"

#ifdef ENABLE_DEBUG_LOG

#if defined(__NUTTX__) || defined(__TUV_MBED__) || defined(__TIZENRT__)
int tuv_debug_level = TDBGLEV_INFO;
#else
int tuv_debug_level = TDBGLEV_ERR;
#endif

FILE *tuv_log_stream;
const char* tuv_debug_prefix[4] = { "", "ERR", "WRN", "INF" };
#endif // ENABLE_DEBUG_LOG


void InitDebugSettings() {
#ifdef ENABLE_DEBUG_LOG
  const char* dbglevel = NULL;
  const char* dbglogfile = NULL;

  dbglevel = getenv("TUV_DEBUG_LEVEL");
  dbglogfile = getenv("TUV_DEBUG_LOGFILE");

  tuv_log_stream = stderr;

  if (dbglevel) {
    tuv_debug_level = atoi(dbglevel);
    if (tuv_debug_level < 0) tuv_debug_level = 0;
    if (tuv_debug_level > TDBGLEV_INFO) tuv_debug_level = TDBGLEV_INFO;
  }
  if (dbglogfile) {
    FILE* logstream;
    logstream  = fopen(dbglogfile, "w+");
    if (logstream != NULL) {
      tuv_log_stream = logstream;
    }
  }
  //fprintf(stderr, "DBG LEV = %d\r\n", tuv_debug_level);
  //fprintf(stderr, "DBG OUT = %s\r\n", (dbglogfile==NULL?"(stderr)":dbglogfile));
#endif // ENABLE_DEBUG_LOG
}


void ReleaseDebugSettings() {
#ifdef ENABLE_DEBUG_LOG
  if (tuv_log_stream != stderr && tuv_log_stream != stdout) {
    fclose(tuv_log_stream);
  }
  // some embed systems(ex, nuttx) may need this
  tuv_log_stream = stderr;
  tuv_debug_level = TDBGLEV_ERR;
#endif // ENABLE_DEBUG_LOG
}

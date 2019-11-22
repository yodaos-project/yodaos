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

#ifndef __tuv_debuglog_header__
#define __tuv_debuglog_header__


#ifdef ENABLE_DEBUG_LOG

#include <stdio.h>

#if defined(DEBUG)
#define ABORT()                  \
  do {                           \
    TDLOG("!!!!! ABORT !!!!!");  \
    exit(-1);                    \
  } while(0)
#else
#define ABORT() abort()
#endif


#ifdef __cplusplus
extern "C" {
#endif


extern int tuv_debug_level;
extern FILE *tuv_log_stream;
extern const char* tuv_debug_prefix[4];

#define TDBGLEV_ERR  1
#define TDBGLEV_WARN 2
#define TDBGLEV_INFO 3

#define TUV_DLOG(lvl, ...) \
  do { \
    int errback = errno; \
    if (0 <= lvl && lvl <= tuv_debug_level && tuv_log_stream) { \
      fprintf(tuv_log_stream, "[%s] ", tuv_debug_prefix[lvl]); \
      fprintf(tuv_log_stream, __VA_ARGS__); \
      fprintf(tuv_log_stream, "\r\n"); \
      fflush(tuv_log_stream); \
    } \
    errno = errback; \
  } while (0)
#define TDLOG(...)   TUV_DLOG(TDBGLEV_ERR, __VA_ARGS__)
#define TDDLOG(...)  TUV_DLOG(TDBGLEV_WARN, __VA_ARGS__)
#define TDDDLOG(...) TUV_DLOG(TDBGLEV_INFO, __VA_ARGS__)

/*
  Use DLOG for errors, default you will see them
  Use DDLOG for warnings, set tuv_debug_level=2 to see them
  USE DDDLOG for informations, set tuv_debug_level=3 to see them
*/


#ifdef __cplusplus
}
#endif


#else /* !ENABLE_DEBUG_LOG */


#ifdef __cplusplus
extern "C" {
#endif

#define ABORT()
#define TUV_DLOG(...)
#define TDLOG(...)
#define TDDLOG(...)
#define TDDDLOG(...)


#ifdef __cplusplus
}
#endif


#endif /* ENABLE_DEBUG_LOG */


#ifdef __cplusplus
extern "C" {
#endif


void InitDebugSettings();
void ReleaseDebugSettings();


#ifdef __cplusplus
}
#endif


#endif /* __tuv_debuglog_header__ */

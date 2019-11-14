#ifndef _CUTILS_LOG_H_
#define _CUTILS_LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#define LOG_BUF_SIZE 1024

typedef enum android_LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;

static inline void dateString(struct timeval* time, char* tbuf, int len) {
    struct tm* timeinfo = localtime(&(time->tv_sec));
    strftime(tbuf, len, "%m-%d %H:%M:%S", timeinfo);
    return;
}

static inline struct timeval currentTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time;
}

static inline int __android_log_print(int prio, const char *tag, const char *fmt, ...) {
     va_list ap;
     char buf[LOG_BUF_SIZE];
     char tbuf[64] = {0};
     struct timeval t = currentTime();
     dateString(&t, tbuf, 64);
 
     va_start(ap, fmt);
     vsnprintf(buf, LOG_BUF_SIZE, fmt, ap);
     va_end(ap);
 
     return fprintf(stdout, "%s.%03d %s\n", tbuf, (int)(t.tv_usec), buf);
}

#define android_printLog(prio, tag, x...) \
     __android_log_print(prio, tag, x)

//#define android_printLog(prio, tag, x...) ((void)0)

#ifndef LOG_NDEBUG
#define LOG_NDEBUG 1
#endif

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

#ifndef __predict_false
#define __predict_false(exp) __builtin_expect((exp) != 0, 0)
#endif

#ifndef ALOG
#define ALOG(priority, tag, ...) \
    LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif

#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...) \
    android_printLog(priority, tag, __VA_ARGS__)
#endif

#ifndef ALOGV
#define __ALOGV(...) ((void)ALOG(LOG_VERBOSE, LOG_TAG, __VA_ARGS__))
#if LOG_NDEBUG
#define ALOGV(...) do { if (0) { __ALOGV(__VA_ARGS__); } } while (0)
#else
#define ALOGV(...) __ALOGV(__VA_ARGS__)
#endif
#endif

#ifndef ALOGD
#define ALOGD(...) ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGI
#define ALOGI(...) ((void)ALOG(LOG_INFO, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGW
#define ALOGW(...) ((void)ALOG(LOG_WARN, LOG_TAG, __VA_ARGS__))
#endif

#ifndef ALOGE
#define ALOGE(...) ((void)ALOG(LOG_ERROR, LOG_TAG, __VA_ARGS__))
#endif

#ifndef SLOGV
#define SLOGV(...) ((void)0)
#endif

#ifndef SLOGD
#define SLOGD(...) ((void)0)
#endif

#ifndef SLOGI
#define SLOGI(...) ((void)0)
#endif

#ifndef SLOGW
#define SLOGW(...) ((void)0)
#endif

#ifndef SLOGE
#define SLOGE(...) ((void)0)
#endif

#ifndef ALOGV_IF
#define ALOGV_IF(cond, ...) ((void)0)
#endif

#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) ((void)0)
#endif

#ifndef ALOGI_IF
#define ALOGI_IF(cond, ...) ((void)0)
#endif

#ifndef ALOGW_IF
#define ALOGW_IF(cond, ...) ((void)0)
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) ((void)0)
#endif

#ifndef ALOG_ASSERT
#define ALOG_ASSERT(cond, ...) ((void)0)
#endif

#ifndef LOG_FATAL_IF
#define LOG_FATAL_IF(cond, ...) ((void)0)
#endif

#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(cond, ...) (void)0
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL(...) ((void)0)
#endif

#ifndef IF_ALOG
#define IF_ALOG(priority, tag) ((void)0)
#endif

#ifndef IF_ALOGV
#define IF_ALOGV() if (false)
#endif

#define android_errorWriteWithInfoLog(tag, subTag, uid, data, dataLen) ((void)0)

#endif

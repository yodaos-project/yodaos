//
//  RKLog.h
//  C API of rklog, for native use.
//

#ifndef RKLOGC_H
#define RKLOGC_H

#include <stdio.h>
#include <stdarg.h>
#ifdef __cplusplus
  extern "C" {
#endif

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

size_t loger(int level, const char *file, int line, const char *logtag, const char *format, va_list args);

extern void jslog(int level, const char *file, int line, const char *logtag, const char *format, ...);

extern void set_cloudlog_on(unsigned int on);

extern void set_cloudlog_on_with_auth(unsigned int on, const char *auth);


#define LOG_VAR_ARGS_TAGS(_L_) ({\
    va_list args; \
    va_start(args, format); \
    int ret = loger((_L_), file, line, logtag, format, args); \
    va_end(args); \
    ret; \
})
#define DEFINE_LOG_FUNC(_F_, _L_) \
    static size_t _F_(const char *file, int line, const char *logtag, const char *format, ...) { \
        return LOG_VAR_ARGS_TAGS(_L_); \
    }

DEFINE_LOG_FUNC(ver, 1)
DEFINE_LOG_FUNC(dbg, 2)
DEFINE_LOG_FUNC(ifo, 3)
DEFINE_LOG_FUNC(war, 4)
DEFINE_LOG_FUNC(err, 5)

#define RKLogv(...) ver(__FILE__, __LINE__, LOG_TAG, __VA_ARGS__)
#define RKLogd(...) dbg(__FILE__, __LINE__, LOG_TAG, __VA_ARGS__)
#define RKLogi(...) ifo(__FILE__, __LINE__, LOG_TAG, __VA_ARGS__)
#define RKLogw(...) war(__FILE__, __LINE__, LOG_TAG, __VA_ARGS__)
#define RKLoge(...) err(__FILE__, __LINE__, LOG_TAG, __VA_ARGS__)

#define RKLog RKLogv
#define RKNotice RKLogd
#define RKInfo RKLogi
#define RKWarn RKLogw
#define RKError RKLoge

/**
 * for android easy porting
 * 
 */
#ifdef ALOGV
#undef ALOGV
#endif
#define ALOGV RKLogv

#ifdef ALOGD
#undef ALOGD
#endif
#define ALOGD RKLogd

#ifdef ALOGI
#undef ALOGI
#endif
#define ALOGI RKLogi

#ifdef ALOGW
#undef ALOGW
#endif
#define ALOGW RKLogw

#ifdef ALOGE
#undef ALOGE
#endif
#define ALOGE RKLoge

#ifdef __cplusplus
}
#endif

#endif //RKLOGC_H


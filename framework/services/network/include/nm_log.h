#ifndef __NM_LOG_H__
#define __NM_LOG_H__
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

enum {
	LEVEL_OVER_LOGE = 1,
	LEVEL_OVER_LOGW,
	LEVEL_OVER_LOGI,
	LEVEL_OVER_LOGD,
	LEVEL_OVER_LOGV,
};

extern int nm_log_level;
#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define NM_USE_RKLOG

#ifdef NM_USE_RKLOG
#include <rklog/RKLog.h>
#define NM_LOGE RKLoge
#define NM_LOGW RKLogw
#define NM_LOGI RKLogi
#else

#define NM_LOGE(format,, ...)                                                 \
do {                                                                            \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define NM_LOGW(format, ...)                                                 \
do {                                                                            \
    if (nm_log_level >= LEVEL_OVER_LOGW) \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define NM_LOGI(format, ...)                                                 \
do {                                                                            \
    if (nm_log_level >= LEVEL_OVER_LOGI) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#endif

#define NM_LOGD(format, ...)                                                 \
do {                                                                            \
    if (nm_log_level >= LEVEL_OVER_LOGD) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define NM_LOGV(format, ...)                                                 \
do {                                                                            \
    if (nm_log_level >= LEVEL_OVER_LOGV) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)


#ifdef __cplusplus
}
#endif
#endif/* __NM_LOG_H__ */

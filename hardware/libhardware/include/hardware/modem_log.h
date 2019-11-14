#ifndef __MODEM_LOG_H__
#define __MODEM_LOG_H__
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

extern int modem_log_level;
#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define MODEM_USE_RKLOG

#ifdef MODEM_USE_RKLOG
#include <rklog/RKLog.h>
#define MODEM_LOGE RKLoge
#define MODEM_LOGW RKLogw
#define MODEM_LOGI RKLogi
#else

#define MODEM_LOGE(format,, ...)                                                 \
do {                                                                            \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define MODEM_LOGW(format, ...)                                                 \
do {                                                                            \
    if (modem_log_level >= LEVEL_OVER_LOGW) \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define MODEM_LOGI(format, ...)                                                 \
do {                                                                            \
    if (modem_log_level >= LEVEL_OVER_LOGI) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#endif

#define MODEM_LOGD(format, ...)                                                 \
do {                                                                            \
    if (modem_log_level >= LEVEL_OVER_LOGD) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define MODEM_LOGV(format, ...)                                                 \
do {                                                                            \
    if (modem_log_level >= LEVEL_OVER_LOGV) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)


#ifdef __cplusplus
}
#endif
#endif/* __MODEM_LOG_H__ */

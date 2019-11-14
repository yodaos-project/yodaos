#ifndef __BT_LOG_H__
#define __BT_LOG_H__
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

extern int bt_log_level;
#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define BT_USE_RKLOG

#ifdef BT_USE_RKLOG
#include <rklog/RKLog.h>

#define BT_LOGE RKLoge
#define BT_LOGW RKLogw
#define BT_LOGI RKLogi
#else
#define BT_LOGE(format, ...)                                                 \
do {                                                                            \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define BT_LOGW(format, ...)                                                 \
do {                                                                            \
    if (bt_log_level >= LEVEL_OVER_LOGW) \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define BT_LOGI(format, ...)                                                 \
do {                                                                            \
    if (bt_log_level >= LEVEL_OVER_LOGI) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#endif

#define BT_LOGD(format, ...)                                                 \
do {                                                                            \
    if (bt_log_level >= LEVEL_OVER_LOGD) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define BT_LOGV(format, ...)                                                 \
do {                                                                            \
    if (bt_log_level >= LEVEL_OVER_LOGV) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)


#ifdef __cplusplus
}
#endif
#endif/* __BT_LOG_H__ */

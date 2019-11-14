#ifndef __INPUT_EVENT_HOT_H__
#define __INPUT_EVENT_HOT_H__

#include <stdbool.h>
#include <stdio.h>

#define PROGRAM  "input-event"
#define VERSION  "0.1.4"


#define INPUT_USE_RKLOG

extern int input_log_level;
#define DEBUG_STR     "%d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

enum {
	LEVEL_OVER_LOGE = 1,
	LEVEL_OVER_LOGW,
	LEVEL_OVER_LOGI,
	LEVEL_OVER_LOGD,
	LEVEL_OVER_LOGV,
};

#ifdef INPUT_USE_RKLOG
#include <rklog/RKLog.h>

#define INPUT_LOGE RKLoge
#define INPUT_LOGW RKLogw
#define INPUT_LOGI RKLogi
#else
#define INPUT_LOGE(format, ...)                                                 \
do {                                                                            \
    printf("ERROR: " DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define INPUT_LOGW(format, ...)                                                 \
do {                                                                            \
    if (input_log_level >= LEVEL_OVER_LOGW) \
        printf("WARN:"DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define INPUT_LOGI(format, ...)                                                 \
do {                                                                            \
    if (input_log_level >= LEVEL_OVER_LOGI) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#endif

#define INPUT_LOGD(format, ...)                                                 \
do {                                                                            \
    if (input_log_level >= LEVEL_OVER_LOGD) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)

#define INPUT_LOGV(format, ...)                                                 \
do {                                                                            \
    if (input_log_level >= LEVEL_OVER_LOGV) \
        printf(DEBUG_STR format "\n", DEBUG_ARGS, ##__VA_ARGS__);                 \
} while (0)


#define test_bit(array, bit) ((array)[(bit)/8] & (1 << ((bit)%8)))

#define ACTION_ORDINARY_EVENT  0
#define ACTION_SINGLE_CLICK 1
#define ACTION_DOUBLE_CLICK 2
#define ACTION_LONG_CLICK 3
#define ACTION_SLIDE 4

#define MAX_SW_KEY 8

//#define MAX_LEN 5


struct gesture {
    bool new_action;
    int action;
    int type;
    int key_code;
    int slide_value;
    long long_press_time;
};

struct keyevent {
    int type;
    int key_code;
    int value;
    long key_time;//ms
};



/*eg : pls see demo/input_hot.c*/


/**
 * Function:  input_event_create
 * input:
 *          have_slide_key: slide key for mini
 *          double_click_timeout: millisecond   suggest:400
 *          slide_timeout: millisecond
 * return:  (void *)input_handle
 */
void *input_event_create(bool have_slide_key,int double_click_timeout,int slide_timeout);

/**
 * Function:  input_event_destroy
 */
int input_event_destroy(void *input_handle);

/**
 * Function:  read_input_events
 *
 * return: read event numbers
 */
int read_input_events(void *input_handle, struct keyevent * buffer, int buffersize, struct gesture * gest);

#endif /* __INPUT_EVENT_HOT_H__ */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "input-event-hot.h"

#define MAX_KEY 20



int main(void)
{
    int key_count = 0;
    int i;
    void *input = NULL;
    struct keyevent event[MAX_KEY];
    struct gesture gesture;

    input = input_event_create(false, 400, 300);
    if (!input) return -1;

    while(1) {
        memset(event, 0, sizeof(event));
        memset(&gesture, 0, sizeof(gesture));
        key_count = read_input_events(input, event, MAX_KEY, &gesture);
        if (key_count > 0) {
            for (i = 0; i < key_count; i++) {
                if (event[i].type == EV_SW) {//switch key
                    printf("sw key(%d):%d time:%ld\n", event[i].key_code, event[i].value, event[i].key_time);
                } else if (event[i].type == EV_KEY) {//key
                    printf("key(%d):%s time:%ld\n", event[i].key_code, event[i].value ? "pressed":"released", event[i].key_time);
                }
            }
        }
        if (gesture.new_action) {
                    switch(gesture.action) {
                        case ACTION_SINGLE_CLICK:
                            printf("====single click(%d)===\n", gesture.key_code);
                            break;
                        case ACTION_DOUBLE_CLICK:
                            printf("===double click(%d)===\n", gesture.key_code);
                            break;
                        case ACTION_LONG_CLICK:
                            printf("===longpress(%d) time:%ld===\n", gesture.key_code, gesture.long_press_time);
                            break;
                        case ACTION_SLIDE:
                            printf("===slide(%d)===\n", gesture.slide_value);
                            break;
                        default:
                            printf("action(%d) key(%d)\n", gesture.action, gesture.key_code);
                            break;
                    }
        }
    }

    input_event_destroy(input);
    return 0;
}


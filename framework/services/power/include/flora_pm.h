#ifndef _FLORA_PM_H_
#define _FLORA_PM_H_

#include <hardware/power.h>
#include <json-c/json.h>
#include <unistd.h>

#define MSG_SLEEP "power.sleep"
#define MSG_FORCE_SLEEP "power.force_sleep"
#define MSG_DEEP_IDLE "power.deep_idle"
#define MSG_LOCK "power.wakelock"
#define MSG_ALARM "rtc.alarm"
#define MSG_BATTERY "battery.info"
#define MSG_VOICE_COMMING "rokid.turen.voice_coming"
#define MSG_VOICE_END "rokid.turen.end_voice"
#define MSG_VOICE_SLEEP "rokid.turen.sleep"
#define MSG_SYS_STATUS "yodart.vui._private_visibility"

enum {
    NUM_MSG_SLEEP = 1,
    NUM_MSG_FORCE_SLEEP,
    NUM_MSG_LOCK,
    NUM_MSG_ALARM,
    NUM_MSG_BATTERY,
    NUM_MSG_VOICE_COMMING,
    NUM_MSG_VOICE_END,
    NUM_MSG_VOICE_SLEEP,
    NUM_MSG_SYS_STATUS,
};

void *pm_flora_handle(void *args);

#endif

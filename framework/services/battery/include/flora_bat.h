#ifndef _FLORA_BAT_H_
#define _FLORA_BAT_H_

#include <hardware/power.h>
#include <json-c/json.h>
#include <unistd.h>

#define MSG_BAT "battery.info"

void *bat_flora_handle(void *args);

#endif

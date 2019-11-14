/*
 * Copyright 2019, The YodaOS open source project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <errno.h>
#include <sys/reboot.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <linux/reboot.h>
#include <sys/syscall.h>
#include <sys/cdefs.h>
#include "rklog/RKLog.h"

/* Commands */
#define ROKID_RB_RESTART  0xDEAD0001
#define ROKID_RB_POWEROFF 0xDEAD0002
#define ROKID_RB_RESTART2 0xDEAD0003
#define CHG_MODE_PATH "/data/property"

int rokid_reboot(int cmd, char *arg)
{
    int ret;

    sync();

    switch (cmd) {
        case ROKID_RB_RESTART:
            ret = reboot(RB_AUTOBOOT);
            break;

        case ROKID_RB_POWEROFF:
            ret = reboot(RB_POWER_OFF);
            break;

        case ROKID_RB_RESTART2:
            ret = syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                          LINUX_REBOOT_CMD_RESTART2, arg);
            break;

        default:
            ret = -1;
    }

    return ret;
}

int rokid_charger_mode(int enable)
{
    FILE *fp;

    if (mkdir(CHG_MODE_PATH, 0600) && errno != EEXIST) {
        RKLog("reboot: failed to create CHG_MODE_PATH \n");
	return -1;
    }

    fp = fopen("/data/property/persist.sys.charger.mode", "w+");

    chmod("/data/property/persist.sys.charger.mode", 0600);

    if(fp) {
	if (enable)
	    fputc('1', fp);
	else
	    fputc('0', fp);

	fclose(fp);
	return 0;
    } else {
        RKLog("reboot: failed to open persist.sys.charger.mode\n");
	return -1;
    }
}

int main(int argc, char **argv)
{
    int ret = 0;
    sync();

    if (argc > 2) {
        RKLog("rokid reboot param  error\n");
        return -1;
    }

    if (strcmp(argv[1], "charging") == 0) {
	rokid_charger_mode(1);
	sync();
    }

    ret = rokid_reboot(ROKID_RB_RESTART2, argv[1]);

    RKLog("reboot: %s error: %s\n", argv[1], strerror(errno));

    return 0;
}

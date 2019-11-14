#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <linux/input.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <common.h>

#define FD_MAX 16
#define MSEC_PER_SEC 1000
#define NSEC_PER_MSEC (1000 * 1000)
#define MSG_LEN 1024
#define RB_DURATION 2700 //ms: long press time
#define EVENT_PATH "/dev/input/event"
#define CHG_MODE_PATH "/data/property/"

/* current time in milliseconds */
static int64_t curr_time_ms(void)
{
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

static int mBatterySetChgMode(int enable)
{
    FILE *fp;

    fp = fopen("/data/property/persist.sys.charger.mode", "w");

    if(fp) {
        if (enable)
            fputc('1', fp);
        else
            fputc('0', fp);

	fclose(fp);
        return 0;
    } else {
        KLOG_INFO(LOG_TAG "reboot: failed to open persist.sys.charger.mode\n");
        return -1;
    }
}

static int mBatteryGetChgMode(void)
{
    FILE *fp;
    int ch;

    fp = fopen("/data/property/persist.sys.charger.mode", "r");

    if (fp) {
	ch = fgetc(fp);
	fclose(fp);
	if (ch == '1')
	    return BOOT_MODE_CHARGING;
	else
	    return BOOT_MODE_NORMAL;
    }

    return -1;
}

int mBootModeUpdate (void) {
    int boot_mode;

    if (mBatteryGetChgMode() == BOOT_MODE_CHARGING) {
        KLOG_INFO(LOG_TAG "start enter poweroff charging mode...\n");
        boot_mode = BOOT_MODE_CHARGING;
    } else
	boot_mode = BOOT_MODE_NORMAL;

    return boot_mode;
}

void *mPowerOffChargingPoll(void *arg) {

	int fd_cnt = 0;
	int i = 0;
	int down = 0;
	int retval;
	int fd[FD_MAX] = {0};
	const int SIZE = 32;
	char path[32];
	int64_t now = 0;
	int64_t now_now = 0;
	int64_t duration = 0;
	fd_set rfds;
	struct timeval tv;
	struct input_event t;
	struct BatteryProperties *bp = &props;

	mBatteryPropertiesUpdate();

        while (fd_cnt < FD_MAX) {
            strcpy(path,"");
            snprintf(path, SIZE, "/dev/input/event%d", fd_cnt);
            fd[fd_cnt] = open(path, O_RDONLY | O_NONBLOCK);

            if (fd[fd_cnt] < 0)
                break;

            fd_cnt++;
        }

	mBootModeUpdate();
	KLOG_INFO(LOG_TAG "event count %d\n", fd_cnt);

        while (1) {
            tv.tv_sec = 3;
            tv.tv_usec = 0;

            FD_ZERO(&rfds);

            for (i = 0; i < fd_cnt; i++) {
                FD_SET(fd[i], &rfds);
            }
            retval = select(fd[fd_cnt - 1] + 1, &rfds, NULL, NULL, &tv);

            switch (retval) {
            case 0:
                KLOG_INFO(LOG_TAG "No data available!!!\n");
                break;
            case -1:
                perror("select()\n");
                break;
            default:
                KLOG_INFO(LOG_TAG "select() data available\n");
                for (i = 0; i < fd_cnt; i++)
                    if((FD_ISSET(fd[i] , &rfds)) && (read(fd[i], &t, sizeof(t)) == sizeof(t))) {
                        if ((t.type==EV_KEY) && (t.code == KEY_POWER)) {
                            if (t.value) {
                                if (down == 0) {
                                    now = curr_time_ms();
				}
                                    down = 1;
				    KLOG_INFO(LOG_TAG "pressed: down:%d %"PRId64" ms \n",down, now);
                           } else {
                               down = 0;
                               KLOG_INFO(LOG_TAG "down = %d \n", down);
                           }
                           KLOG_INFO(LOG_TAG "power key :%s %d %s \n", path, t.code,
				     (t.value) ? "Pressed" : "Released");
                       }
                }
                break;
            }

            if (down){
		now_now = curr_time_ms();
	        duration = now_now - now;
		KLOG_INFO(LOG_TAG "pressed: %"PRId64" ms \n", duration);
	    }

	    if (duration > RB_DURATION) {
		mBatterySetChgMode(0);
		system("reboot -f");
	    }

	    if (!(bp->chargerAcOnline | bp->chargerUsbOnline
		  | bp->chargerWirelessOnline)) {
		mBatterySetChgMode(0);
		system("poweroff -f");
            }
	    usleep(50000);
	}
	return NULL;
}


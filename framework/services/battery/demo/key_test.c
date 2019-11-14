#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/time.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>

#define EVENT_PATH "/dev/input/event"
#define FD_MAX 16
#define MSEC_PER_SEC 1000
#define NSEC_PER_MSEC (1000 * 1000)
#define KLOG_INFO printf
#define KLOG_ERROR printf
#define TAG "bat: "
#define MSG_LEN 1024
#define CHARGER_MODE "androidboot.mode=charger"

enum {
    BOOT_MODE_NORMAL = 0,
    BOOT_MODE_CHARGING,
};
/* current time in milliseconds */
static int64_t curr_time_ms(void)
{
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return tm.tv_sec * MSEC_PER_SEC + (tm.tv_nsec / NSEC_PER_MSEC);
}

int mBootModeUpdate (void) {
    char cmdline[MSG_LEN] = {0};
    char *ptr;
    char fd;

    fd = open("/proc/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        cmdline[0] = 0;
    KLOG_ERROR(TAG "could not open cmdline\n");
    } else {
    int size = read(fd, cmdline, sizeof(cmdline) - 1);

    if (size < 0)
        size = 0;

    cmdline[size] = 0;
    close(fd);
    }

    ptr = cmdline;
    char *pstr = strstr(ptr, "androidboot.mode");
    if (pstr == NULL)
    return BOOT_MODE_NORMAL;

    if (strncmp(pstr, CHARGER_MODE, strlen(CHARGER_MODE)) == 0)
    KLOG_INFO(TAG "start enter poweroff charging mode...\n");
        return BOOT_MODE_CHARGING;

    return BOOT_MODE_NORMAL;
}

int main()
{
	int fd_cnt = 0;
	int i = 0;
	int rfd = 0;
	int down = 0;
	int fd[FD_MAX] = {0};
	const int SIZE = 32;
	char path[32];
	int64_t now;
	int64_t now_now;
	int64_t duration;
	fd_set rfds;
	struct timeval tv;
	int retval;
	struct input_event t;

	while (fd_cnt < FD_MAX) {
	    strcpy(path,"");
	    snprintf(path, SIZE, "/dev/input/event%d", fd_cnt);
            fd[fd_cnt] = open(path, O_RDONLY | O_NONBLOCK);

	    if (fd[fd_cnt] < 0)
		break;

	    fd_cnt++;
	}

	mBootModeUpdate();
	printf("event count %d\n", fd_cnt);

	while (1) {
	    tv.tv_sec = 3;
	    tv.tv_usec = 0;

	    FD_ZERO(&rfds);

	    for (i = 0; i < fd_cnt; i++) {
		printf("fd = %d\n", fd[i]);
		FD_SET(fd[i], &rfds);
	    }
	    retval = select(fd[fd_cnt - 1] + 1, &rfds, NULL, NULL, &tv);

	    switch (retval) {
	    case 0:
		printf("No data available!!!\n");
		break;
	    case -1:
		perror("select()\n");
		break;
	    default:
		printf("select() data available\n");
		for (i = 0; i < fd_cnt; i++)
		    if((FD_ISSET(fd[i] , &rfds)) && (read(fd[i], &t, sizeof(t)) == sizeof(t))) {
			printf("select %d data available\n", i);
			if ((t.type==EV_KEY) && (t.code == KEY_POWER)) {
			    printf("#####PowerKey is available now.\n");
                            if (t.value) {
                                if (down == 0)
                                    now = curr_time_ms();
                                    down = 1;
                                    printf("pressed: down: %d %ld ms \n",down, now);
                           } else {
                               down = 0;
                               printf("down = %d \n", down);
                           }
                           printf("power key :%s %d %s \n", path, t.code,
                                         (t.value) ? "Pressed" : "Released");
		       }
	        }
		break;
	    }

            if (down){
		now_now = curr_time_ms();
	        duration = now_now -now;

		printf("now_now: %ld ms \n", now_now);
		printf("now: %ld ms \n", now);
		printf("pressed: %ld ms \n", duration);
	    }
	    usleep(20000);
	}
	return 0;
}


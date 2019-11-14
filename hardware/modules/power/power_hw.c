#define LOG_TAG "PowerHAL"

#include <hardware/hardware.h>
#include <hardware/power.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/atomic.h>

#define MODULE_NAME "PowerHAL"
#define MODULE_AUTHOR "chao.ma@rokid.com"

enum {
	ACQUIRE_PARTIAL_WAKE_LOCK = 0,
	RELEASE_WAKE_LOCK,
	SET_AUTO_SLEEP,
	SET_SLEEP,
	OUR_FD_COUNT
};

const char * const POWER_PATHS[] = {
	"/sys/power/wake_lock",
	"/sys/power/wake_unlock",
	"/sys/power/autosleep",
	"/sys/power/state"
};

static int g_initialized = 0;
static int g_fds[OUR_FD_COUNT];
static int g_error = -1;

static int power_device_open(const struct hw_module_t *module, const char *name,
			     struct hw_device_t **device);
static int power_device_close(struct hw_device_t *device);

static int open_file_descriptors(const char * const paths[])
{
    int i;
    for (i=0; i<OUR_FD_COUNT; i++) {
        int fd = open(paths[i], O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            g_error = -errno;
            fprintf(stderr, "fatal error opening \"%s\": %s\n", paths[i],
                strerror(errno));
            return -1;
        }
        g_fds[i] = fd;
    }

    g_error = 0;
    return 0;
}

static inline void initialize_fds(void)
{
    if (g_initialized == 0) {
        if(open_file_descriptors(POWER_PATHS) == 0)
            g_initialized = 1;
    }
}

int acquire_wake_lock(int lock, const char* id)
{
    initialize_fds();

//    ALOGI("acquire_wake_lock lock=%d id='%s'\n", lock, id);

    if (g_error)
	    return g_error;

    int fd;
    ssize_t ret;

    if (lock != PARTIAL_WAKE_LOCK) {
        return -EINVAL;
    }

    fd = g_fds[ACQUIRE_PARTIAL_WAKE_LOCK];

    ret = write(fd, id, strlen(id));
    if (ret < 0) {
        return -errno;
    }

    return ret;
}

int release_wake_lock(const char* id)
{
    initialize_fds();

//    ALOGI("release_wake_lock id='%s'\n", id);

    if (g_error)
	    return g_error;

    ssize_t len = write(g_fds[RELEASE_WAKE_LOCK], id, strlen(id));
    if (len < 0) {
        return -errno;
    }
    return len;
}

int set_auto_sleep(const char* id)
{
    initialize_fds();

//    ALOGI("set_auto_sleep id='%s'\n", id);

    if (g_error)
	    return g_error;

    ssize_t len = write(g_fds[SET_AUTO_SLEEP], id, strlen(id));
    if (len < 0) {
        return -errno;
    }
    return len;
}

int set_sleep(const char* id)
{
    initialize_fds();

//    ALOGI("set_auto_sleep id='%s'\n", id);

    if (g_error)
            return g_error;

    ssize_t len = write(g_fds[SET_SLEEP], id, strlen(id));
    if (len < 0) {
        return -errno;
    }
    return len;
}

int set_idle(int idle)
{
    initialize_fds();

//    ALOGI("set_idle\n");

    if (g_error)
            return g_error;

    /*Todo: implement idle_enter/idle_exit scheme according to platform dvfs/cpu_idle/cpu_hotplug scheme*/
    /* if idle is 1, system enter idle state; if idle is 0, system exit from idle state*/

    return 0;

}

static void sysfs_write(const char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}

static void sysfs_read(const char *path, char *s)
{
        char buf[80];
        int len;
        int fd = open(path, O_RDONLY);

        if (fd < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error opening %s: %s\n", path, buf);
                return;
        }

        len = read(fd, s, 100);
        ALOGV("Reading from %s: %s\n", path, s);
        if (len < 0) {
                strerror_r(errno, buf, sizeof(buf));
                ALOGE("Error Reading to %s: %s\n", path, buf);
        }
        close(fd);
}

static int power_device_close(struct hw_device_t *device)
{
	struct power_device_t *power_device = (struct power_device_t *)device;

	if (power_device) {
		free(power_device);
	}

	return 0;

}

static int power_device_open(const struct hw_module_t *module, const char *name,
			     struct hw_device_t **device)
{
	struct power_device_t *dev;
	dev = (struct power_device_t *)malloc(sizeof(struct power_device_t));

	if (!dev) {
		printf("Power HAL: failed to alloc space");
		return -EFAULT;
	}

	memset(dev, 0, sizeof(struct power_device_t));
	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (hw_module_t *) module;
	dev->common.close = power_device_close;
	dev->acquire_wakelock = acquire_wake_lock;
	dev->release_wakelock = release_wake_lock;
	dev->set_autosleep = set_auto_sleep;
	dev->set_sleep = set_sleep;
	dev->set_idle = set_idle;

	*device = &(dev->common);
	printf("Power HAL: open successfully.\n");

	return 0;
}

static struct hw_module_methods_t power_module_methods = {
	.open =	power_device_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag =	 HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id =	 POWER_HARDWARE_MODULE_ID,
	.name =	 MODULE_NAME,
	.author = MODULE_AUTHOR,
	.methods = &power_module_methods,
};

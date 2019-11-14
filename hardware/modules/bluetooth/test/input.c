#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <dirent.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

#define NAME_MAX         255	/* # chars in a file name */
#define PATH_MAX        4096	/* # chars in a path name including nul */
#define EPOLL_SIZE_HINT 8

// Maximum number of signalled FDs to handle at a time.
#define EPOLL_MAX_EVENTS  16
#define DEVICE_PATH "/dev/input"


struct input_device {
    int fd;
    char *name;
    struct input_device *pre;
    struct input_device *next;
};

static struct input_device dev_head;
static int  mUsingEpollWakeup = 1;
static int  mEpollFd = -1;

static void getLinuxRelease(int* major, int* minor) {
    struct utsname info;
    if (uname(&info) || sscanf(info.release, "%d.%d", major, minor) <= 0) {
        *major = 0, *minor = 0;
        printf("Could not get linux version: %s", strerror(errno));
    }
}

static int add_to_dev_list(struct input_device *device)
{
    struct input_device *pos = &dev_head;
 
    while(pos->next != NULL) {
        pos = pos->next;
    }

    pos->next = device;
    device->pre = pos;
    device->next = NULL;

    return 0;
}

static int del_from_dev_list(struct input_device *device)
{
    struct input_device *pre = device->pre;
    struct input_device *next = device->next;

    if (pre)
        pre->next = next;
    if (next)
        next->pre = pre;

    if (device->name)
        free(device->name);
    free(device);

    return 0;
}

static struct input_device* getDeviceByPathLocked(const char* devicePath)
{
    struct input_device *pos = dev_head.next;

    while (pos) {
        if (strcmp(pos->name, devicePath) == 0) {
            return pos;
        }
        pos = pos->next;
    }

    return NULL;
}

static struct input_device* getDeviceByFd(int fd)
{
    struct input_device *pos = dev_head.next;

    while (pos) {
        if (pos->fd == fd) {
            return pos;
        }
        pos = pos->next;
    }

    return NULL;
}


int openDeviceLocked(const char *devicePath)
{
    struct input_device *device = NULL;
    int fd = open(devicePath, O_RDWR | O_CLOEXEC);
    if(fd < 0) {
        printf("could not open %s\n", devicePath);
        return -1;
    }

    device = calloc(1, sizeof(struct input_device));
    device->fd = fd;
    device->name = malloc(strlen(devicePath)+1);
    strcpy(device->name, devicePath);
    add_to_dev_list(device);

    //add with epoll.
    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    if (mUsingEpollWakeup) {
        eventItem.events |= EPOLLWAKEUP;
    }
    eventItem.data.u32 = fd;
    printf("%s, name %s, fd 0x%x\n", __func__, device->name, eventItem.data.u32);
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, fd, &eventItem)) {
        printf("Could not add device fd to epoll instance.  errno=%d", errno);
        return -1;
    }

    return 0;
}

static int closeDeviceLocked(struct input_device *device)
{
    if (device) {
        if (epoll_ctl(mEpollFd, EPOLL_CTL_DEL, device->fd, NULL)) {
            printf("Could not remove device fd from epoll instance.  errno=%d", errno);
        }

        printf("%s, name %s, fd 0x%x\n", __func__, device->name, device->fd);
        if (device->fd >= 0) {
            close(device->fd);
        }

        del_from_dev_list(device);
    }
    return 0;
}

static int closeAllDeviceLocked(void)
{
    struct input_device *pos = dev_head.next;
    struct input_device *next = dev_head.next;

    while (pos) {
        next = pos->next;
        closeDeviceLocked(pos);
        pos = next;
    }
    return 0;
}

static int closeDeviceLockedbyPath(const char *devicePath)
{
    struct input_device *device = getDeviceByPathLocked(devicePath);
    if (device) {
        return closeDeviceLocked(device);
    } else {
        printf("Remove device: %s not found, device may already have been removed.\n", devicePath);
        return -1;
    }
    return 0;
}

static int scanDirLocked(const char *dirname)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        openDeviceLocked(devname);
    }
    closedir(dir);
    return 0;
}

int readNotifyLocked(int mINotifyFd)
{
    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    res = read(mINotifyFd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        printf("could not get event\n");
        return -1;
    }
    //printf("got %d bytes of event information\n", res);

    strcpy(devname, DEVICE_PATH);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        //printf("%d: %08x \"%s\"\n", event->wd, event->mask, event->len ? event->name : "");
        if(event->len) {
            strcpy(filename, event->name);
            if(event->mask & IN_CREATE) {
                printf("add device '%s' due to inotify event\n", devname);
                openDeviceLocked(devname);
            } else {
                printf("Removing device '%s' due to inotify event\n", devname);
                closeDeviceLockedbyPath(devname);
            }
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return 0;
}


int main(void)
{
    int ret, wd;
    int mINotifyFd= -1;
    struct epoll_event eventItem;
    struct epoll_event mPendingEventItems[EPOLL_MAX_EVENTS];
    int mPendingEventCount = 0;
    int mPendingEventIndex = 0;

    bool deviceChanged = false;
    bool mPendingINotify = false;

    struct input_device* device = NULL;

    int buffersize = 20;
    int capacity = buffersize;
    struct input_event event;
    struct input_event readBuffer[20];
    int readSize;
    int count, i;

    mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    if (mEpollFd < 0) {
        printf("failed to epoll_create\n");
        return -1;
    }
    mINotifyFd = inotify_init();
    if (mINotifyFd < 0) {
        printf("failed to inotify_init\n");
        return -1;
    }
    wd = inotify_add_watch(mINotifyFd, DEVICE_PATH, IN_DELETE | IN_CREATE);
    if (wd < 0) {
        printf("failed to inotify_add_watch\n");
        return -1;
    }

    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = mINotifyFd;
    ret = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, mINotifyFd, &eventItem);

    int major, minor;
    getLinuxRelease(&major, &minor);
    // EPOLLWAKEUP was introduced in kernel 3.5
    mUsingEpollWakeup = major > 3 || (major == 3 && minor >= 5);

    scanDirLocked(DEVICE_PATH);
    while (1) {
        deviceChanged = false;
        while (mPendingEventIndex < mPendingEventCount) {
            eventItem = mPendingEventItems[mPendingEventIndex++];
            printf("epoll even fd, 0x%x\n", eventItem.data.u32);
            if (eventItem.data.u32 == mINotifyFd) {
                if (eventItem.events & EPOLLIN) {
                    mPendingINotify = true;
                } else {
                    printf("Received unexpected epoll event 0x%08x for INotify.", eventItem.events);
                }
                continue;
            }

            device = getDeviceByFd(eventItem.data.u32);
            if (!device) {
                printf("Received unexpected epoll event 0x%08x for unknown device id %d.",
                        eventItem.events, eventItem.data.u32);
                continue;
            }
            if (eventItem.events & EPOLLIN) {
                readSize = read(device->fd, readBuffer,
                        sizeof(struct input_event) * capacity);
                if (readSize == 0 || (readSize < 0 && errno == ENODEV)) {
                    // Device was removed before INotify noticed.
                    printf("could not get event, removed? (fd: %d size: %d,bufferSize: %d capacity: %d errno: %d)\n",
                        device->fd, readSize, buffersize, capacity, errno);
                    deviceChanged = true;
                    closeDeviceLocked(device);
                } else if (readSize < 0) {
                    if (errno != EAGAIN && errno != EINTR) {
                        printf("could not get event (errno=%d)", errno);
                    }
                } else if ((readSize % sizeof(struct input_event)) != 0) {
                    printf("could not get event (wrong size: %d)", readSize);
                } else {
                    count = (readSize) / sizeof(struct input_event);
                    for (i = 0; i < count; i++) {
                        event = readBuffer[i];
                        printf("%s got: time=%d.%06d, type=%d, code=%d, value=%d\n",
                                device->name,
                                (int) event.time.tv_sec, (int) event.time.tv_usec,
                                event.type, event.code, event.value);
                        if (event.type == EV_KEY) { //KEY
                            switch(event.code) {
                            case KEY_NEXTSONG:
                                printf("next key %s\n", event.value? "pressed" : "released");
                                break;
                            case KEY_PREVIOUSSONG:
                                printf("Backward key %s\n", event.value? "pressed" : "released");
                                break;
                            case KEY_PLAYCD:
                                printf("play key %s\n", event.value? "pressed" : "released");
                                break;
                            case KEY_PAUSECD:
                                printf("pause key %s\n", event.value? "pressed" : "released");
                                break;
                            default:
                                printf("key code %d, value=%d, %s\n", event.code, event.value, event.value? "pressed" : "released");
                                break;
                            }
                        }
                    }
                }
            } else if (eventItem.events & EPOLLHUP) {
                printf("Removing device %s due to epoll hang-up event.\n",
                        device->name);
                deviceChanged = true;
                closeDeviceLocked(device);
            } else {
                printf("Received unexpected epoll event 0x%08x for device %s.",
                        eventItem.events, device->name);
            }
        }
        // readNotify() will modify the list of devices so this must be done after
        // processing all other events to ensure that we read all remaining events
        // before closing the devices.
        if (mPendingINotify && mPendingEventIndex >= mPendingEventCount) {
            mPendingINotify = false;
            readNotifyLocked(mINotifyFd);
            deviceChanged = true;
        }
        // Report added or removed devices immediately.
        if (deviceChanged) {
            continue;
        }
        mPendingEventIndex = 0;
        mPendingEventCount = epoll_wait(mEpollFd, mPendingEventItems, EPOLL_MAX_EVENTS, -1);

        if (mPendingEventCount < 0) {
            // An error occurred.
            mPendingEventCount = 0;

            // Sleep after errors to avoid locking up the system.
            // Hopefully the error is transient.
            if (errno != EINTR) {
                printf("poll failed (errno=%d)\n", errno);
                usleep(100000);
            }
        }
    }

    closeAllDeviceLocked();
    ret = inotify_rm_watch(mINotifyFd, wd);

    if (mINotifyFd >= 0) {
        close(mINotifyFd);
        mINotifyFd =-1;
    }
    if (mEpollFd >= 0) {
        close(mEpollFd);
        mEpollFd = -1;
    }
    return 0;
}


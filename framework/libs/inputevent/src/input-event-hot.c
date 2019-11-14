#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <dirent.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include "input-event-hot.h"
//#include "input-event-table.h"


#define LOG_TAG "input-event"
#define NAME_MAX         255	/* # chars in a file name */
#define PATH_MAX        4096	/* # chars in a path name including nul */
#define EPOLL_SIZE_HINT 8

// Maximum number of signalled FDs to handle at a time.
#define EPOLL_MAX_EVENTS  16
#define DEVICE_PATH "/dev/input"

/*SWITCH KEY*/
#define SW_CONFIG  "/etc/gpio.config"

struct input_device {
    int fd;
    char *name;
    struct input_device *pre;
    struct input_device *next;
};

struct click_gesture {
    bool use;
    int type;
    int key_code;
    int key_value;
    unsigned long downtime;
};


struct input_event_handle {
    bool have_slide_key;
    int slide_timeout;
    int double_click_timeout;

    int mUsingEpollWakeup;
    int mEpollFd;
    int mINotifyFd;
    int wd;


    int sw_keycode[MAX_SW_KEY];
    int sw_keyvalue[MAX_SW_KEY];
    int sw_keynumber;

    struct click_gesture gesture_key;

    struct epoll_event mPendingEventItems[EPOLL_MAX_EVENTS];
    int mPendingEventCount;
    int mPendingEventIndex;
    bool mPendingINotify;

    struct input_device dev_list;
};

int input_log_level = LEVEL_OVER_LOGI;

static int parse_switch_config(struct input_event_handle *handle)
{
    FILE *fp;
    char line[1024];
    int j = 0;
    char ch = ':';
    char *gpio;

    if((fp = fopen(SW_CONFIG,"r")) != NULL)
    {
        while(!feof(fp))
        {
            fgets(line, sizeof(line), fp);
            if(line[strlen(line)-1] == '\n')
            {
                line[strlen(line)-1] = '\0';
            }
            if(line[0] == '\n' || line[0] == '\0' || line[0] == '\r')
            {
                continue;
            }
            else
            {
                gpio = strchr(line, ch);
                if(gpio != NULL)
                {
                    *gpio = '\0';
                    gpio++;
                    while((*gpio != '/') && (*gpio != '\0'))
                    {
                        gpio++;
                    }
                    if(*gpio == '/')
                    {
                        int fd = open(gpio, O_RDONLY|O_NONBLOCK);
                        if(fd < 0)
                        {
                            INPUT_LOGE("open failed:%s\n",strerror(errno));
                            continue;
                        }
                        else
                        {
                            unsigned char keyvalue;
                            if (j >= MAX_SW_KEY) {
                                INPUT_LOGW("break, only support max(%d) switch keys\n", MAX_SW_KEY);
                                break;
                            }
                            if(read(fd, &keyvalue, sizeof(keyvalue)) > 0)
                            {
                                handle->sw_keycode[j] = atoi(line);
                                if(keyvalue == '0')
                                {
                                    handle->sw_keyvalue[j] = 0;
                                }
                                else if(keyvalue == '1')
                                {
                                    handle->sw_keyvalue[j] = 1;
                                }
                                else
                                {
                                    INPUT_LOGE("wrong value\n");
                                    continue;
                                }
                                j++;
                            }
                            else
                            {
                                INPUT_LOGE(" read failed\n");
                            }

                        }
                    }
                }
            }
        }
    }
    return j;
}

static void getLinuxRelease(int* major, int* minor) {
    struct utsname info;
    if (uname(&info) || sscanf(info.release, "%d.%d", major, minor) <= 0) {
        *major = 0, *minor = 0;
        printf("Could not get linux version: %s", strerror(errno));
    }
}

static int add_to_dev_list(struct input_device *list, struct input_device *device)
{
    struct input_device *pos = list;

    while(pos->next != NULL) {
        pos = pos->next;
    }

    pos->next = device;
    device->pre = pos;
    device->next = NULL;

    return 0;
}

static int del_from_dev_list(struct input_device *list, struct input_device *device)
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

static struct input_device* getDeviceByPathLocked(struct input_device *list, const char* devicePath)
{
    struct input_device *pos = list->next;

    while (pos) {
        if (strcmp(pos->name, devicePath) == 0) {
            return pos;
        }
        pos = pos->next;
    }

    return NULL;
}

static struct input_device* getDeviceByFd(struct input_device *list, int fd)
{
    struct input_device *pos = list->next;

    while (pos) {
        if (pos->fd == fd) {
            return pos;
        }
        pos = pos->next;
    }

    return NULL;
}


static int openDeviceLocked(struct input_event_handle *handle, const char *devicePath)
{
    struct input_device *device = NULL;
    int fd = open(devicePath, O_RDWR | O_CLOEXEC);
    if(fd < 0) {
        INPUT_LOGI("could not open %s\n", devicePath);
        return -1;
    }

    device = calloc(1, sizeof(struct input_device));
    device->fd = fd;
    device->name = malloc(strlen(devicePath)+1);
    strcpy(device->name, devicePath);
    add_to_dev_list(&handle->dev_list, device);

    //add with epoll.
    struct epoll_event eventItem;
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    if (handle->mUsingEpollWakeup) {
        eventItem.events |= EPOLLWAKEUP;
    }
    eventItem.data.u32 = fd;
    INPUT_LOGI("name %s, fd 0x%x\n", device->name, eventItem.data.u32);
    if (epoll_ctl(handle->mEpollFd, EPOLL_CTL_ADD, fd, &eventItem)) {
        INPUT_LOGI("Could not add device fd to epoll instance.  errno=%d", errno);
        return -1;
    }

    return 0;
}

static int closeDeviceLocked(struct input_event_handle *handle, struct input_device *device)
{
    if (device) {
        if (epoll_ctl(handle->mEpollFd, EPOLL_CTL_DEL, device->fd, NULL)) {
            INPUT_LOGI("Could not remove device fd from epoll instance.  errno=%d", errno);
        }

        INPUT_LOGI("name %s, fd 0x%x\n", device->name, device->fd);
        if (device->fd >= 0) {
            close(device->fd);
        }

        del_from_dev_list(&handle->dev_list,device);
    }
    return 0;
}

static int closeAllDeviceLocked(struct input_event_handle *handle)
{
    struct input_device *pos = handle->dev_list.next;
    struct input_device *next = handle->dev_list.next;

    while (pos) {
        next = pos->next;
        closeDeviceLocked(handle, pos);
        pos = next;
    }
    return 0;
}

static int closeDeviceLockedbyPath(struct input_event_handle *handle, const char *devicePath)
{
    struct input_device *device = getDeviceByPathLocked(&handle->dev_list, devicePath);
    if (device) {
        return closeDeviceLocked(handle, device);
    } else {
        INPUT_LOGI("Remove device: %s not found, device may already have been removed.\n", devicePath);
        return -1;
    }
    return 0;
}

static int scanDirLocked(struct input_event_handle *handle, const char *dirname)
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
        openDeviceLocked(handle, devname);
    }
    closedir(dir);
    return 0;
}

static int readNotifyLocked(struct input_event_handle *handle)
{
    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    res = read(handle->mINotifyFd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) {
        INPUT_LOGI("could not get event\n");
        return -1;
    }
    //INPUT_LOGI("got %d bytes of event information\n", res);

    strcpy(devname, DEVICE_PATH);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event)) {
        event = (struct inotify_event *)(event_buf + event_pos);
        //INPUT_LOGI("%d: %08x \"%s\"\n", event->wd, event->mask, event->len ? event->name : "");
        if(event->len) {
            strcpy(filename, event->name);
            if(event->mask & IN_CREATE) {
                INPUT_LOGI("add device '%s' due to inotify event\n", devname);
                openDeviceLocked(handle, devname);
            } else {
                INPUT_LOGI("Removing device '%s' due to inotify event\n", devname);
                closeDeviceLockedbyPath(handle, devname);
            }
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return 0;
}

static int input_event_open(struct input_event_handle *handle)
{
    int ret;
    struct epoll_event eventItem;

    handle->mEpollFd = epoll_create(EPOLL_SIZE_HINT);
    if (handle->mEpollFd < 0) {
        INPUT_LOGE("failed to epoll_create\n");
        return -1;
    }
    handle->mINotifyFd = inotify_init();
    if (handle->mINotifyFd < 0) {
        INPUT_LOGE("failed to inotify_init\n");
        return -1;
    }
    handle->wd = inotify_add_watch(handle->mINotifyFd, DEVICE_PATH, IN_DELETE | IN_CREATE);
    if (handle->wd < 0) {
        INPUT_LOGE("failed to inotify_add_watch\n");
        return -1;
    }

    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = handle->mINotifyFd;
    ret = epoll_ctl(handle->mEpollFd, EPOLL_CTL_ADD, handle->mINotifyFd, &eventItem);
    if (ret) {
        INPUT_LOGE("failed to inotify_add_watch");
    }
    int major, minor;
    getLinuxRelease(&major, &minor);
    // EPOLLWAKEUP was introduced in kernel 3.5
    handle->mUsingEpollWakeup = major > 3 || (major == 3 && minor >= 5);
    ret = scanDirLocked(handle, DEVICE_PATH);
    return ret;
}

//handle single/double/long click
static bool click_event(struct input_event_handle *handle, struct gesture * gesture, struct keyevent *event)
{
    /*timout no event, report already up click gesture*/
    if (!event) {
        if (handle->gesture_key.use && handle->gesture_key.key_value == 0) {
            gesture->new_action = true;
            gesture->action = ACTION_SINGLE_CLICK;
            gesture->type = EV_KEY;
            gesture->key_code = handle->gesture_key.key_code;
            handle->gesture_key.use = false;
            INPUT_LOGI("=====single click(%d)======", handle->gesture_key.key_code);
        }
        return gesture->new_action;
    }

    if(event->value == 1) {/*key press*/
        if (handle->gesture_key.use == false) {//cache event
            handle->gesture_key.use = true;
            handle->gesture_key.downtime = event->key_time;
            handle->gesture_key.key_code = event->key_code;
            handle->gesture_key.key_value = event->value;
        } else {// have cache event , analyse it
            if (event->key_code == handle->gesture_key.key_code) {
                if ((event->key_time - handle->gesture_key.downtime) > handle->double_click_timeout) {//timeout send sing click
                    gesture->new_action = true;
                    gesture->action = ACTION_SINGLE_CLICK;
                    gesture->type = EV_KEY;
                    gesture->key_code = event->key_code;
                    handle->gesture_key.downtime = event->key_time;// replace new downtime
                    handle->gesture_key.key_value = event->value;
                    INPUT_LOGI("=====single click(%d)======", event->key_code);
                } else {//double click
                    gesture->new_action = true;
                    gesture->action = ACTION_DOUBLE_CLICK;
                    gesture->type = EV_KEY;
                    gesture->key_code = event->key_code;
                    handle->gesture_key.use = false;
                    INPUT_LOGI("=====double click(%d)======", event->key_code);
                }
            } else {//a new key press, report old, and replace new
                gesture->new_action = true;
                gesture->type = EV_KEY;
                gesture->key_code = handle->gesture_key.key_code;
                if (handle->gesture_key.key_value ==  2) {//longpress
                    gesture->action = ACTION_LONG_CLICK;
                    gesture->long_press_time = event->key_time -handle->gesture_key.downtime;
                    INPUT_LOGW("=====new key(%d) press abort long key(%d)======", event->key_code, gesture->key_code);
                } else {
                    gesture->action = ACTION_SINGLE_CLICK;
                }
                INPUT_LOGI("=====%s(%d)======", (handle->gesture_key.key_value ==  2) ? "longpress":"single click", gesture->key_code);
                /*replace new*/
                handle->gesture_key.key_code = event->key_code;
                handle->gesture_key.downtime = event->key_time;
                handle->gesture_key.key_value = event->value;
            }
        }
    } else if (event->value == 0) {/*click up*/
        if (handle->gesture_key.use) {
            if (event->key_code == handle->gesture_key.key_code) {
                if (handle->gesture_key.key_value ==  2) {//longpress up
                    gesture->new_action = true;
                    gesture->action = ACTION_LONG_CLICK;
                    gesture->type = EV_KEY;
                    gesture->key_code = handle->gesture_key.key_code;
                    gesture->long_press_time = event->key_time -handle->gesture_key.downtime;
                    handle->gesture_key.use = false;
                    INPUT_LOGI("=====long press(%d) %ld======", handle->gesture_key.key_code, gesture->long_press_time);
                } else  if ((event->key_time - handle->gesture_key.downtime) > handle->double_click_timeout) {//timeout send sing click
                    gesture->new_action = true;
                    gesture->action = ACTION_SINGLE_CLICK;
                    gesture->type = EV_KEY;
                    gesture->key_code = handle->gesture_key.key_code;
                    handle->gesture_key.use = false;
                    INPUT_LOGI("=====single click(%d)======", handle->gesture_key.key_code);
                }
                handle->gesture_key.key_value = event->value;
            }
        }
    } else if (event->value == 2) {/*repeat key*/
        if (handle->gesture_key.use && (handle->gesture_key.key_code == event->key_code)) {
                handle->gesture_key.key_value = event->value;
                gesture->new_action = true;
                gesture->action = ACTION_LONG_CLICK;
                gesture->type = EV_KEY;
                gesture->key_code = handle->gesture_key.key_code;
                gesture->long_press_time = event->key_time -handle->gesture_key.downtime;
                INPUT_LOGI("=====long press(%d) %ld ms======", handle->gesture_key.key_code, gesture->long_press_time);
        } else {//some thing lossed , we should cache new
            INPUT_LOGW("key(%d) longpress, but lossed some gesture cache, %d, oldkey(%d)",
                                event->key_code, handle->gesture_key.use, handle->gesture_key.key_code);
            if (handle->gesture_key.use &&  handle->gesture_key.key_value == 0) {//report old up key
                gesture->new_action = true;
                gesture->action = ACTION_SINGLE_CLICK;
                gesture->type = EV_KEY;
                gesture->key_code = handle->gesture_key.key_code;
                INPUT_LOGI("=====single click(%d)======", handle->gesture_key.key_code);
            }
            handle->gesture_key.use = true;
            handle->gesture_key.key_value = event->value;
            handle->gesture_key.key_code = event->key_code;
            handle->gesture_key.downtime = event->key_time;
        }
    }

    return gesture->new_action;
}

int input_event_destroy(void *input_handle)
{
    struct input_event_handle *handle = input_handle;
    if (handle) {
        closeAllDeviceLocked(handle);

        if (handle->mINotifyFd >= 0) {
            if (epoll_ctl(handle->mEpollFd, EPOLL_CTL_DEL, handle->mINotifyFd, NULL)) {
                INPUT_LOGE("Could not remove mINotifyFd from epoll instance.  errno=%d", errno);
            }
            inotify_rm_watch(handle->mINotifyFd, handle->wd);
            close(handle->mINotifyFd);
            handle->mINotifyFd =-1;
        }
        if (handle->mEpollFd >= 0) {
            close(handle->mEpollFd);
            handle->mEpollFd = -1;
        }
        free(handle);
    }
    return 0;
}

void *input_event_create(bool have_slide_key,int double_click_timeout,int slide_timeout)
{
    struct input_event_handle *handle = calloc(1, sizeof(struct input_event_handle));
    int ret;

    if (!handle) return NULL;
    handle->double_click_timeout = double_click_timeout;
    handle->have_slide_key = have_slide_key;
    handle->slide_timeout = slide_timeout;
    handle->mEpollFd = -1;
    handle->mINotifyFd = -1;
    handle->wd = -1;
    ret = input_event_open(handle);
    if (ret) {
        INPUT_LOGE("failed to input_event_open");
        input_event_destroy(handle);
        return NULL;
    }
    handle->sw_keynumber = parse_switch_config(handle);
    return  (void *)handle;
}

int read_input_events(void *input_handle, struct keyevent * buffer, int buffersize, struct gesture * gest)
{
    int ret;
    struct epoll_event eventItem;
    bool deviceChanged = false;
    struct keyevent * event = buffer;
    struct input_device* device = NULL;
    int capacity = buffersize;
    struct input_event iev;
    struct input_event readBuffer[buffersize];
    int readSize;
    int count, i;
    struct timeval tv={0};
    unsigned long tms_start = 0;
    struct input_event_handle *handle = input_handle;
    gest->new_action = false;

    if (!handle) return 0;

    /*first report init sw key*/
    if (handle->sw_keynumber > 0) {
        gettimeofday(&tv, NULL);
        tms_start = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        while (handle->sw_keynumber && capacity > 0)
        {
                handle->sw_keynumber--;
                event->type = EV_SW;
                event->key_code = handle->sw_keycode[handle->sw_keynumber];
                event->value = handle->sw_keyvalue[handle->sw_keynumber];
                event->key_time = tms_start;
                event += 1;
                capacity -= 1;
        }
        return event - buffer;
    }
    while (1) {
        deviceChanged = false;
        while (handle->mPendingEventIndex < handle->mPendingEventCount) {
            eventItem = handle->mPendingEventItems[handle->mPendingEventIndex++];
            INPUT_LOGV("epoll even fd, 0x%x", eventItem.data.u32);
            if (eventItem.data.u32 == handle->mINotifyFd) {
                if (eventItem.events & EPOLLIN) {
                    handle->mPendingINotify = true;
                } else {
                    INPUT_LOGW("Received unexpected epoll event 0x%08x for INotify.", eventItem.events);
                }
                continue;
            }

            device = getDeviceByFd(&handle->dev_list, eventItem.data.u32);
            if (!device) {
                INPUT_LOGW("Received unexpected epoll event 0x%08x for unknown device id %d.",
                        eventItem.events, eventItem.data.u32);
                continue;
            }
            if (eventItem.events & EPOLLIN) {
                readSize = read(device->fd, readBuffer,
                        sizeof(struct input_event) * capacity);
                if (readSize == 0 || (readSize < 0 && errno == ENODEV)) {
                    // Device was removed before INotify noticed.
                    INPUT_LOGW("could not get event, removed? (fd: %d size: %d,bufferSize: %d capacity: %d errno: %d)",
                        device->fd, readSize, buffersize, capacity, errno);
                    deviceChanged = true;
                    closeDeviceLocked(handle, device);
                } else if (readSize < 0) {
                    if (errno != EAGAIN && errno != EINTR) {
                        INPUT_LOGE("could not get event (errno=%d)", errno);
                    }
                } else if ((readSize % sizeof(struct input_event)) != 0) {
                    INPUT_LOGE("could not get event (wrong size: %d)", readSize);
                } else {
                    count = (readSize) / sizeof(struct input_event);
                    for (i = 0; i < count; i++) {
                        iev = readBuffer[i];
                        if (iev.type == EV_SW || iev.type == EV_KEY) {
                            INPUT_LOGI("%s got: time=%d.%06d, type=%d, code=%d, value=%d\n",
                                device->name,
                                (int) iev.time.tv_sec, (int) iev.time.tv_usec,
                                iev.type, iev.code, iev.value);
                            event->type = iev.type;
                            event->key_code = iev.code;
                            event->value = iev.value;
                            event->key_time = iev.time.tv_sec * 1000 + iev.time.tv_usec/1000;
                            if (iev.type == EV_KEY) {
                                click_event(handle, gest, event);
                            }
                            event += 1;
                            capacity -= 1;
                        }
                    }
                    if (capacity == 0) {
                        // The result buffer is full.  Reset the pending event index
                        // so we will try to read the device again on the next iteration.
                        handle->mPendingEventIndex -= 1;
                        break;
                    }
                }
            } else if (eventItem.events & EPOLLHUP) {
                INPUT_LOGI("Removing device %s due to epoll hang-up event.\n",
                        device->name);
                deviceChanged = true;
                closeDeviceLocked(handle, device);
            } else {
                INPUT_LOGI("Received unexpected epoll event 0x%08x for device %s.",
                        eventItem.events, device->name);
            }
        }
        // readNotify() will modify the list of devices so this must be done after
        // processing all other events to ensure that we read all remaining events
        // before closing the devices.
        if (handle->mPendingINotify && handle->mPendingEventIndex >= handle->mPendingEventCount) {
            handle->mPendingINotify = false;
            readNotifyLocked(handle);
            deviceChanged = true;
        }
        // Report added or removed devices immediately.
        if (deviceChanged) {
            continue;
        }
        // Return now if we have collected any events or if we were explicitly awoken.
        if (event != buffer /*|| awoken*/) {
            break;
        }
        handle->mPendingEventIndex = 0;
        handle->mPendingEventCount = epoll_wait(handle->mEpollFd, handle->mPendingEventItems, EPOLL_MAX_EVENTS, handle->double_click_timeout);

        if (handle->mPendingEventCount == 0) {
            //timeout
            click_event(handle, gest, NULL);
            break;
        }
        if (handle->mPendingEventCount < 0) {
            // An error occurred.
            handle->mPendingEventCount = 0;

            // Sleep after errors to avoid locking up the system.
            // Hopefully the error is transient.
            if (errno != EINTR) {
                INPUT_LOGI("poll failed (errno=%d)\n", errno);
                usleep(100000);
            }
        }
    }

    // All done, return the number of events we read.
    return event - buffer;
}
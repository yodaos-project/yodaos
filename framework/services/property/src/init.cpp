/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <mtd/mtd-user.h>
//#include <base/file.h>
//#include <base/stringprintf.h>
//#include <cutils/android_reboot.h>
//#include <cutils/fs.h>
//#include <cutils/iosched_policy.h>
//#include <cutils/list.h>
#include <utils/Log.h>
#include <cutils/sockets.h>
#include <cutils/memory.h>
#include "property_service.h"
#include "util.h"
#include "init.h"
//#include "log.h"

enum {
    BOOT_MODE_NORMAL,
    BOOT_MODE_CHARING,
    BOOT_MODE_FACTORY,
} boot_mode_state;

#define FTM_MODE_COMMAND "fw_setenv ftm_mode 1"
static int epoll_fd = -1;
static char qemu[32];

void register_epoll_handler(int fd, void (*fn)()) {
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = reinterpret_cast<void*>(fn);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        ALOGE("epoll_ctl failed\n");
    }
}

void property_changed(const char *name, const char *value)
{
    if (strcmp(name, "sys.ubootenv.reload") == 0
          && (strcmp(value, "0") == 0)) {
        //bootenv_reinit();
        init_property_set("sys.ubootenv.reload","1");
    }
//todo
    //if (property_triggers_enabled)
    //    queue_property_triggers(name, value);
}

static int get_boot_mode(void)
{
    char cmdline[2048] = {0};
    char *ptr;
    int fd;

    fd = open("/proc/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        int n = read(fd, cmdline, sizeof(cmdline) - 1);
        if (n < 0) n = 0;

        /* get rid of trailing newline, it happens */
        if (n > 0 && cmdline[n-1] == '\n') n--;

        cmdline[n] = 0;
        close(fd);
    } else {
        cmdline[0] = 0;
    }

    ptr = cmdline;
    char *x = strstr(ptr, "systemd.unit");
    if (x == 0) return BOOT_MODE_NORMAL;
    printf("%s\n", x);
    if (!strncmp(x, "systemd.unit=sysinit.target", 27)) {
        printf("in power off charging mode\n");
        return BOOT_MODE_CHARING;
    }
    if (!strncmp(x, "systemd.unit=rescue.target", 26)) {
        printf("in factory mode, set %s\n", FTM_MODE_COMMAND);
        system(FTM_MODE_COMMAND);
        return BOOT_MODE_FACTORY;
    }

    return BOOT_MODE_NORMAL;
}

static void import_kernel_nv(char *name, bool for_emulator)
{
    char *value = strchr(name, '=');
    int name_len = strlen(name);

    if (value == 0) return;
    *value++ = 0;
    if (name_len == 0) return;

    if (for_emulator) {
        /* in the emulator, export any kernel option with the
         * ro.kernel. prefix */
        char buff[PROP_NAME_MAX];
        int len = snprintf( buff, sizeof(buff), "ro.kernel.%s", name );

        if (len < (int)sizeof(buff))
            init_property_set( buff, value );
        return;
    }

    if (!strcmp(name,"qemu")) {
        strlcpy(qemu, value, sizeof(qemu));
    } else if (!strncmp(name, "androidboot.", 12) && name_len > 12) {
        const char *boot_prop_name = name + 12;
        char prop[PROP_NAME_MAX];
        int cnt;

        cnt = snprintf(prop, sizeof(prop), "ro.boot.%s", boot_prop_name);
        if (cnt < PROP_NAME_MAX)
            init_property_set(prop, value);
    }
}
#if 0
static void export_kernel_boot_props() {
    struct {
        const char *src_prop;
        const char *dst_prop;
        const char *default_value;
    } prop_map[] = {
        //ROKID BEGIN
        { "ro.boot.serialno",   "ro.serialno",   "unknown", },
        { "ro.boot.serialno",   "ro.sys.rokid.serialno", "unknown", },
        { "ro.boot.rokidseed",  "ro.rokidseed", "unknown", },
        { "ro.boot.rokidseed",  "ro.sys.rokid.seed", "unknown", },
	{ "ro.boardinfo.hardwareid", "ro.sys.hardwareid", "0" },
	{ "ro.boot.ftm",       "ro.bootftm",   "0", },
	{ "ro.boot.factory_date",       "ro.rokid.factory_date",   "1970-1-1", },
        // ROKID END
        { "ro.boot.mode",       "ro.bootmode",   "unknown", },
        { "ro.boot.baseband",   "ro.baseband",   "unknown", },
        { "ro.boot.bootloader", "ro.bootloader", "unknown", },
        { "ro.boot.hardware",   "ro.hardware",   "amlogic", },
        { "ro.boot.revision",   "ro.revision",   "0", },
        { "ro.boot.firstboot",  "ro.firstboot",  "0"},
    };
    for (size_t i = 0; i < ARRAY_SIZE(prop_map); i++) {
        char value[PROP_VALUE_MAX];
        int rc = init_property_get(prop_map[i].src_prop, value);
        init_property_set(prop_map[i].dst_prop, (rc > 0) ? value : prop_map[i].default_value);
    }
}
#endif
static void process_kernel_cmdline(void)
{
    /* don't expose the raw commandline to nonpriv processes */
    chmod("/proc/cmdline", 0440);

    /* first pass does the common stuff, and finds if we are in qemu.
     * second pass is only necessary for qemu to export all kernel params
     * as props.
     */
    import_kernel_cmdline(false, import_kernel_nv);
    if (qemu[0])
        import_kernel_cmdline(true, import_kernel_nv);
}


static int aml_firstbootinit()
{
    char is_firstboot[PROP_VALUE_MAX] = {0};
    init_property_get("ro.firstboot", is_firstboot);
    if (strncmp(is_firstboot, "1", 1) == 0) {
        ALOGE("aml-firstboot-init insert aml-firstboot-init\n");
    }

    return 0;
}

int main(int argc, char** argv) {
    mkdir(ANDROID_SOCKET_DIR, 0755);
    mkdir(PERSISTENT_PROPERTY_DIR, 0700);
    property_init();
    process_kernel_cmdline();

    // Propogate the kernel variables to internal variables
    // used by init as well as the current required properties.
    //export_kernel_boot_props();

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        ALOGE("epoll_create1 failed\n");
        exit(1);
    }

    property_load_boot_defaults();
    load_system_props();
    load_persist_props();
    start_property_service();
    aml_firstbootinit();
    get_boot_mode();

    while (true) {
        int timeout = -1;

        epoll_event ev;
        int nr = TEMP_FAILURE_RETRY(epoll_wait(epoll_fd, &ev, 1, timeout));
        if (nr == -1) {
            ALOGE("epoll_wait failed\n");
        } else if (nr == 1) {
            ((void (*)()) ev.data.ptr)();
        }
    }

    return 0;
}

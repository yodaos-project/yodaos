#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <asm/ioctl.h>
#include <mtd/ubi-user.h>
#include <dirent.h>

#include "sha1.h"
#include "rokid_package.h"
#include "recovery/recovery.h"

struct bin_head *bin_head[36];
struct file_head *file_head = NULL;

int main(int argc, char **argv)
{
    int fd = 0;
    int ret = 0;
    int choice = 0;
    char file_name[256] = {0};
    int i = 0;

    printf("begin to enter update....\n");
    choice = judge_need_upgrade(file_name);
    if (choice < 0) {
        set_upgrade_statue(BOOTSTATE_RES_ERROR);
        set_recovery_end();
        goto reboot;
    }

    ret = system("echo 1 > /sys/ota_light/ota_start");
    ret = system("echo 1 > /sys/devices/platform/led-stage/ota_ctl");
    printf("begin to enter update rest %d....\n", ret);
    set_recovery_end();

    sync();
    if (choice == DATA_FLUSHED) {
        // flush data part
        printf("begin to flush data part.\n");
        flush_data(YODABASE_FSTAB);
        printf("finish flushing data part.\n");
    } else if (choice == OTA_UPDATE){
        // upgrade the flash
        mount_data(YODABASE_FSTAB);
        printf("file_name Path: %s\n",file_name);
        fd = open(file_name, O_RDONLY);
        if (fd < 0) {
            printf("open image error %d %s \n", errno, strerror(errno));
            set_upgrade_statue(BOOTSTATE_RES_ERROR);
            goto reboot;
        }

        file_head = (struct file_head*)calloc(1, sizeof(struct file_head));
        if (file_head == NULL) {
            return -1;
        }

        ret = get_file_head(fd, file_head);
        if (ret < 0) {
            printf("get file head error\n");
            set_upgrade_statue(BOOTSTATE_RES_ERROR);
            goto free_file;
        }

        for (i = 0; i < file_head->sum; i++) {
            bin_head[i] = (struct bin_head*)calloc(1, sizeof(struct bin_head));
            if (bin_head[i] == NULL) {
                printf("malloc bin head error\n");
                set_upgrade_statue(BOOTSTATE_RES_ERROR);
                goto free_head;
            }
        }

        ret = get_bin_head(fd, file_head, bin_head);
        if (ret < 0) {
            printf("get bin head error\n");
            set_upgrade_statue(BOOTSTATE_RES_ERROR);
            goto free_head;
        }

        ret = parse_update_bin(fd, file_head, bin_head);
        if (ret < 0) {
            printf("parse update bin  error\n");
            set_upgrade_statue(BOOTSTATE_RES_ERROR);
            goto free_head;
        }

    }
    set_upgrade_statue(BOOTSTATE_RES_OK);
    ret = system("echo 1 > /sys/ota_light/ota_end");
    ret = system("echo 0 > /sys/devices/platform/led-stage/ota_ctl");

free_head:
    free_alloc_buf(file_head, bin_head);
free_file:
    free(file_head);
    close(fd);
reboot:
    sync();
    sleep(2);
    ret = system("busybox reboot");

    return 0;
}

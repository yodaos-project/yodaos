#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../rokid_package.h"
#include "../recovery/recovery.h"

#ifdef OTA_AUTH
extern int ota_auth_check(int fd, struct file_head *file, struct bin_head **bin);
#endif
extern int get_file_head(int fd, struct file_head *head);
extern int get_bin_head(int fd, struct file_head *head, struct bin_head **bin);

int main(int argc, char **argv){
    int ret = 0;
    struct boot_cmd cmd;
    char mtd_name[32] = {0};
    struct partition_info part_tab[MAX_PARTITIONS_NUM];
    char data_partname[32] = {0};
    char misc_partname[32] = {0};
    char env_partname[32] = {0};
    int part_num = 0;
    int i = 0;
    int fd;
    struct bin_head *bin_head[36];
    struct file_head *file_head = NULL;

    memset(&cmd, 0, sizeof(struct boot_cmd));

    if (argc < 2) {
        printf("param error\n");

        return -1;
    }

    if (get_recovery_cmd_status(&cmd)) {
        return -1;
    }

    part_num = get_partitions_info(part_tab, MAX_PARTITIONS_NUM, YODABASE_FSTAB);
    for(i=0; i<part_num; i++) {
        printf("%s:%s:%s:%s:%s:0x%x:%d\n", part_tab[i].part_name, part_tab[i].part_mount_info.mount_point, part_tab[i].part_mount_info.mount_format, \
            part_tab[i].part_mount_info.mount_path, part_tab[i].part_mount_info.mount_param, get_partition_size(part_tab[i].part_mount_info.mount_point), \
	    part_tab[i].part_format);
        if (get_upgrade_partition_type(YODABASE_FSTAB) == MTD_AB_FORMAT)
            printf("%s mtd num is %d\n", part_tab[i].part_mount_info.mount_point, get_mtd_num(part_tab[i].part_mount_info.mount_point));
    }

    if (strcmp(argv[1], "set") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY) + 1);
        strncpy(cmd.recovery_path, "/data/ota_upgrade.img", strlen("/data/ota_upgrade.img") + 1);
        ret = set_recovery_cmd_status(&cmd);
    }else if (strcmp(argv[1], "auth") == 0) {
#ifdef OTA_AUTH
        fd = open("/data/ota_upgrade.img",O_RDONLY);
        if (fd < 0) {
            printf("open image error %d %s \n", errno, strerror(errno));
            return -1;
        }
        file_head = (struct file_head*)calloc(1, sizeof(struct file_head));
        if (file_head == NULL) {
            printf("malloc file_head fail\n");
            close(fd);
            return -1;
        }
        ret = get_file_head(fd, file_head);
        if (ret < 0) {
            printf("get file head error\n");
            close(fd);
            return -1;
        }
        for (i = 0; i < file_head->sum; i++) {
            bin_head[i] = (struct bin_head*)calloc(1, sizeof(struct bin_head));
            if (bin_head[i] == NULL) {
                printf("malloc bin head error\n");
                close(fd);
                return -1;
            }
        }
        ret = get_bin_head(fd, file_head, bin_head);
        if (ret < 0) {
            printf("get bin head error\n");
            close(fd);
            return -1;
        }
        if (ota_auth_check(fd, file_head, bin_head) != 0) {
            printf("ota_auth fail\n");
            close(fd);
            return -1;
        }
        printf("ota_auth success\n");
        close(fd);
#endif
    }else if (strcmp(argv[1], "set_diff") == 0) {
        if (argc < 3) {
            printf("param error\n");
            printf("ex: unpack_cli set_diff  ${version}\n");
            return -1;
        }
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY) + 1);
        strncpy(cmd.recovery_path, "/data/ota_diff.img", strlen("/data/ota_diff.img") + 1);
        printf("set ro.build.version.release=%s\n", argv[2]);
        strncpy(cmd.version, argv[2], strlen(argv[2]) + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "dataflush") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_FLUSHDATA, strlen(BOOTMODE_FLUSHDATA) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY) + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "seta") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY) + 1);
        strncpy(cmd.recovery_path, "/data/ota_upgrade.img", strlen("/data/ota_upgrade.img") + 1);
        strncpy(cmd.recovery_partition, "recovery_partition_a", strlen("recovery_partition_a") + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "setb") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY) + 1);
        strncpy(cmd.recovery_path, "/data/ota_upgrade.img", strlen("/data/ota_upgrade.img") + 1);
        strncpy(cmd.recovery_partition, "recovery_partition_b", strlen("recovery_partition_b") + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "get") == 0) {
        ret = get_recovery_cmd_status(&cmd);
        if (ret == 0) {
            printf("ota mode: %s\n", cmd.boot_mode);
            printf("ota status: %s\n", cmd.recovery_state);
            printf("ota path: %s\n", cmd.recovery_path);
            printf("flashed a or b: %s\n", cmd.recovery_partition);
        }
    } else if (strcmp(argv[1], "clear") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_OK, strlen(BOOTSTATE_OK) + 1);
        strncpy(cmd.recovery_path, "/data/ota_upgrade.img", strlen("/data/ota_upgrade.img") + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "format") == 0) {
        ret = get_data_partname(YODABASE_FSTAB, data_partname);
        if(ret) {
            printf("get_data_partname failed %d\n",ret);
            return ret;
        }
        ret = get_mtd_name(data_partname, mtd_name, YODABASE_FSTAB);
        if (ret) {
            printf("get mtd error\n");
            return -3;
        }
        printf("format mtd name %s\n", mtd_name);

        ret = nand_erase(mtd_name, 0);
        if (ret) {
            printf("nand erase %s error\n", mtd_name);
            return -4;
        }
    } else if (strcmp(argv[1], "test") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_RUN, strlen(BOOTSTATE_RUN) + 1);
        strncpy(cmd.recovery_path, "/data/ota_upgrade.img", strlen("/data/ota_upgrade.img") + 1);
        ret = set_recovery_cmd_status(&cmd);
    }else if(strcmp(argv[1],"test_data") == 0) {
        strncpy(cmd.boot_mode, BOOTMODE_FLUSHDATA, strlen(BOOTMODE_FLUSHDATA) + 1);
        strncpy(cmd.recovery_state, BOOTSTATE_RUN, strlen(BOOTSTATE_RUN) + 1);
        ret = set_recovery_cmd_status(&cmd);
    } else if (strcmp(argv[1], "partname") == 0) {
        ret = get_data_partname(YODABASE_FSTAB, data_partname);
        if(ret) {
            printf("get_data_partname failed %d\n",ret);
            return ret;
	}
	ret = get_misc_partname(YODABASE_FSTAB, misc_partname);
	if(ret) {
            printf("get_misc_partname failed %d\n",ret);
            return ret;
	}
	ret = get_env_partname(YODABASE_FSTAB, env_partname);
	if(ret) {
            printf("get_env_partname failed %d\n",ret);
            return ret;
	}
	printf("data_partname:%s, misc_partname:%s, env_partname:%s\n", data_partname, misc_partname, env_partname);
    } else if (strcmp(argv[1], "mount_data") == 0) {
        ret = mount_data(YODABASE_FSTAB);
        if(ret) {
            printf("mount_data failed %d\n",ret);
            return ret;
        }
    }
    return ret;
}

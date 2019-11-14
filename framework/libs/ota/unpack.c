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
#include <stdbool.h>
#ifdef OTA_AUTH
#include <libsignv/sign_verify.h>
#define VERSION_OFFSET 1
#else
#define VERSION_OFFSET 0
#endif

#include "sha1.h"
#include "rokid_package.h"
#include "recovery/recovery.h"

int set_upgrade_statue(int sta)
{
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    if (get_recovery_cmd_status(&cmd))
        return -1;

    if (sta == BOOTSTATE_RES_OK) {
        strncpy(cmd.recovery_state, BOOTSTATE_OK, strlen(BOOTSTATE_OK) + 1);
    } else {
/*
        if (get_recovery_cmd_status(&cmd)) {
            return -1;
        }
*/
        strncpy(cmd.recovery_state, BOOTSTATE_ERROR, strlen(BOOTSTATE_ERROR) + 1);
    }

    if (set_recovery_cmd_status(&cmd)) {
        return -1;
    }

    return 0;
}

int set_flashed_partition(int status)
{
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    if (get_recovery_cmd_status(&cmd))
        return -1;

    if (status == FLASHED_PART_A) {
        strncpy(cmd.recovery_partition, RECOVERY_PART_A, strlen(RECOVERY_PART_A) + 1);
    } else {
        strncpy(cmd.recovery_partition, RECOVERY_PART_B, strlen(RECOVERY_PART_B) + 1);
    }

    if (set_recovery_cmd_status(&cmd)) {
        return -1;
    }

    return 0;
}

int set_recovery_end()
{
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    if (get_recovery_cmd_status(&cmd))
        return -1;

    if (strncmp(cmd.recovery_status, RECOVERYB_START, strlen(RECOVERYB_START)) == 0) {
        strncpy(cmd.recovery_status, RECOVERYB_END, strlen(RECOVERYB_END) + 1);
    } else {
        strncpy(cmd.recovery_status, RECOVERYA_END, strlen(RECOVERYA_END) + 1);
    }
    if (set_recovery_cmd_status(&cmd)) {
        return -1;
    }

    return 0;
}

int get_file_head(int fd, struct file_head *head) {
    int ret = 0;
    int len = 0;

    ret = lseek(fd, 0, SEEK_SET);
    if (ret < 0) {
        return -1;
    }

    len = read(fd, head, sizeof(struct file_head));
    if (len != sizeof(struct file_head)) {
        return -2;
    }

    printf("num :%d  position : %"PRIu64" size %"PRIu64"\n", head->sum, \
	   head->start_position, head->file_size);

    if (head->sum > MAX_BIN_SUM || head->file_size > MAX_BIN_SIZE)
        return -3;

    return 0;
}

int get_bin_head(int fd, struct file_head *head, struct bin_head **bin) {
    int i = 0;
    int ret = 0;
    int len = 0;

    ret = lseek(fd, sizeof(struct file_head), SEEK_SET);
    if (ret < 0) {
        return -1;
    }


    for (i = 0; i < head->sum; i++) {
        len = read(fd, bin[i], sizeof(struct bin_head));
        if (len < sizeof(struct bin_head)) {
            return -3;
        }

        printf("name :: %s bin_size :: %"PRIu64" start_position :: %"PRIu64" flash_start :: %"PRIu64" flash size :: %"PRIu64" sub type :: %s diff_pack_dev::%s ubi_dev::%s raw_dev::%s\n", bin[i]->file_name, bin[i]->bin_size, bin[i]->start_position, bin[i]->flash_start_position, bin[i]->flash_part_size, bin[i]->sub_type, bin[i]->diff_pack_dev, bin[i]->ubi_dev, bin[i]->raw_dev);
    }

    return 0;
}

void free_alloc_buf(struct file_head *file, struct bin_head **bin) {
    int i = 0;

    if (file != NULL) {
        for (i = 0; i < file->sum; i++) {
          if(bin[i] != NULL)
          {

            free(bin[i]);
            bin[i]= NULL;
        }
     }
//        free(file);
    }
}

static int scan_dir_install(const char *dirname)
{
    char *cmd = NULL;
    char *devname = NULL;
    char *filename = NULL;
    DIR *dir = NULL;
    struct dirent *de = NULL;
    int ret = -1;
    int ipk_sum = 0;
    int ipk_ok = 0;

     /*start install diff packages using "opkg install -d ota xxx.ipk" */
    devname = malloc(1024);
    if (!devname)
        return -1;
    cmd = malloc(2048);
    if (!cmd)
        goto exit;

    memset(devname, 0x0, 1024);
    memset(cmd, 0x0, 2048);
    dir = opendir(dirname);
    if(dir == NULL) {
        goto exit;
    }
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(strstr(de->d_name, ".ipk")) {
            ipk_sum ++;
            strcpy(filename, de->d_name);
            snprintf(cmd, 2048, "opkg install -d ota %s", devname);
            ret = system(cmd);
            if (ret==0)
                ipk_ok++;
        }
    }
exit:

    if (dir) {
        closedir(dir);
    }
    if (cmd)
        free(cmd);
    if (devname)
        free(devname);
    printf("finish diff_pack_install %d packs, total (%d), ret(%d)\n",ipk_ok, ipk_sum, ret);

    return (ret? ret : ipk_ok- ipk_sum);
}

static int diff_pack_install(int fd, struct bin_head *bin, uint8_t *buff)
{
    char sys_cmd[256];
    struct partition_info part_info;
    char *mount_point[2] = {0};
    char ubi_dev[32] = {0};
    int mount_point_cnt = -1;
    int dev_ubinum = -1;
    int ubi_num = -1;
    int mtd_num = -1;
    char *line = NULL;
    char *index = NULL;
    int diff_fd;
    int len;
    int ret;

    get_rawflash_partition_info(bin->sub_type, &part_info);
    if(part_info.part_format == RAW_FS || part_info.part_format == SQUASH_FS || \
       part_info.part_format == SQUASHFS_OVERLAY_UBIFS || part_info.part_format == UNKNOWN_FS) {
        printf("invalid partition format(%d) for diff_pack_install\n",part_info.part_format);
        return -1;
    }
    /*mount rootfs partition to /mnt using rw mode*/
    if(part_info.part_format == UBIFS_OVERLAY_UBIFS || part_info.part_format == UBI_FS) {
        strcpy(ubi_dev,bin->diff_pack_dev);
        ubi_num = get_max_ubinum();
        dev_ubinum = get_dev_ubinum(ubi_dev);
        if(dev_ubinum < 0) {
            printf("diff_pack_install error:invalid dev_ubinum.\n");
            return -1;
        }
        if(ubi_num == dev_ubinum-1) {
            mtd_num = get_mtd_num(part_info.part_mount_info.mount_point);
            if(mtd_num < 0) {
                printf("diff_pack_install error:invalid mtd number.\n");
                return -1;
            }
            memset(sys_cmd, 0, sizeof(sys_cmd));
            sprintf(sys_cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            system(sys_cmd);
        } else if(ubi_num < dev_ubinum-1) {
            printf("diff_pack_install error:invalid ubi_num\n");
            return -1;
        }
        memset(sys_cmd, 0, sizeof(sys_cmd));
        sprintf(sys_cmd, "mount -t ubifs -o rw %s /mnt", bin->diff_pack_dev);
    } else if(part_info.part_format == VOL_OVERLAY_UBIFS) {
        line = part_info.part_mount_info.mount_point;
        for(index = strsep(&line, ", \t\n"); index != NULL; index = strsep(&line, ", \t\n")) {
            if(!strcmp(index,""))
                continue;
            ++mount_point_cnt;
            if(mount_point_cnt > 1)
                break;
            mount_point[mount_point_cnt] = index;
        }
        if(mount_point_cnt < 1) {
            printf("diff_pack_install error:invalid mount point\n");
            return -1;
        }
        strcpy(ubi_dev,bin->diff_pack_dev);
        ubi_num = get_max_ubinum();
        dev_ubinum = get_dev_ubinum(ubi_dev);
        if(dev_ubinum < 0) {
            printf("diff_pack_install error:invalid dev_ubinum.\n");
            return -1;
        }
        if(ubi_num == dev_ubinum-1) {
            mtd_num = get_mtd_num(mount_point[0]);
            if(mtd_num < 0) {
                printf("diff_pack_install error:invalid mtd number.\n");
                return -1;
            }
            memset(sys_cmd, 0, sizeof(sys_cmd));
            sprintf(sys_cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            system(sys_cmd);
        } else if(ubi_num < dev_ubinum-1) {
            printf("diff_pack_install error:invalid ubi_num\n");
            return -1;
        }
        memset(sys_cmd, 0, sizeof(sys_cmd));
        sprintf(sys_cmd, "mount -t ubifs -o rw %s /mnt", bin->diff_pack_dev);
    } else {
        memset(sys_cmd, 0, sizeof(sys_cmd));
        sprintf(sys_cmd, "mount -t %s -o rw %s /mnt",part_info.part_mount_info.mount_format,bin->diff_pack_dev);
    }
    system(sys_cmd);

    diff_fd = open("/tmp/diff.tar.gz", O_RDWR|O_CREAT|O_TRUNC , 0666);
    if (diff_fd < 0) {
        printf("open file name /tmp/diff.tar.gz error %d\n", errno);
        return -2;
    }
    len = write(diff_fd, buff, bin->bin_size);
    if (len != bin->bin_size) {
        printf("write /tmp/diff.tar.gz file error %d\n", errno);
        close(diff_fd);
        return -2;
    }
    close(diff_fd);
    sync();
    snprintf(sys_cmd, sizeof(sys_cmd), "cd /tmp && tar -zxvf  /tmp/diff.tar.gz");
    ret = system(sys_cmd);
    if (ret)
        printf("faile to unpack tar\n");
    sync();
    ret = scan_dir_install("/tmp/ota");
    if (ret)
        printf("faile to intstall packages\n");
    sync();
    return ret;
}

void string_left_space_trim_oneline(char *pStr)
{
    char *pTmp = pStr;

    while (*pTmp == ' ')
    {
        pTmp++;
    }

    while(*pTmp != '\0' && *pTmp != 0x0A)
    {
        *pStr = *pTmp;
        pStr++;
        pTmp++;
    }
    *pStr = '\0';
}

#define DIFFRE_TYPE "TYPE=differ"
#define DIFFRE_BASE_VERSION "BASE_VERSION="

static int rokid_check_diff_version(int fd, struct file_head *file, struct bin_head **bin, struct boot_cmd *cmd)
{
    uint8_t *buf = NULL;
    int len = 0;
    char sha1sum[48] = {0};
    char *version = NULL;

    buf = (uint8_t *)malloc(bin[VERSION_OFFSET]->bin_size + 1);
    if (buf == NULL) {
        printf("malloc error %d\n", errno);
        return -1;
    }
    memset(buf, 0, bin[VERSION_OFFSET]->bin_size + 1);
    lseek(fd, bin[VERSION_OFFSET]->start_position, SEEK_SET);

    len = read(fd, buf, bin[VERSION_OFFSET]->bin_size);
    if (len != bin[VERSION_OFFSET]->bin_size) {
        goto failed;
    }

    calc_sha1sum(buf, bin[VERSION_OFFSET]->bin_size, sha1sum);
    if (strncmp(sha1sum, bin[VERSION_OFFSET]->bin_sha1sum, sizeof(sha1sum)) == 0) {
        if(strstr((char *)buf, DIFFRE_TYPE) != NULL) {
            version = strstr((char *)buf, DIFFRE_BASE_VERSION);
            if(version != NULL) {
                version += strlen(DIFFRE_BASE_VERSION);
                string_left_space_trim_oneline(version);
                if (strcmp(cmd->version, version)) {
                    printf("%s, failed to check base version %s, %s\n", __func__, cmd->version, version);
                    goto failed;
                }
            } else {
                printf("%s, failed to get base version\n", __func__);
                goto failed;
            }
        } else {
            printf("%s, failed to check type differ\n", __func__);
            goto failed;
        }
    } else {
        printf("%s, failed to check sha1\n", __func__);
        goto failed;
    }

    if (buf)
        free(buf);

    return 0;

failed:

    if (buf)
        free(buf);

    return -1;
}

#ifdef OTA_AUTH
int ota_auth_check(int fd, struct file_head *file, struct bin_head **bin) {
    int ret = -1;
    int i = 0;
    int image_start = 1;
    int len = 0;
    int image_len = 0;
    int sha1sum_len = 0;
    uint8_t *image_buf = NULL;
    uint8_t *sigbuf_in = NULL;
    uint8_t *image_write_buf = NULL;
    uint8_t *sha1sum_buf = NULL;
    char image_sha1sum[48] = {0};

    sigbuf_in = (uint8_t *)malloc(bin[0]->bin_size + 1);
    if (sigbuf_in == NULL) {
        printf("sigbuf_in malloc error %d\n", errno);
        goto failed;
    }
    memset(sigbuf_in, 0, bin[0]->bin_size + 1);
    lseek(fd, bin[0]->start_position, SEEK_SET);
    len = read(fd, sigbuf_in, bin[0]->bin_size);
    if (len != bin[0]->bin_size) {
        goto failed;
    }
    printf("sign.txt: %s\n",sigbuf_in);
    if (strcmp(bin[VERSION_OFFSET]->sub_type, "version") == 0) {
        image_start = 2;
    }
    image_len = file->sum - image_start;
    printf("image_size: %d\n",image_len);
    image_buf = (uint8_t *)malloc(image_len*48);
    if (!image_buf) {
        printf("image_buf malloc error %d\n", errno);
        goto failed;
    }
    memset(image_buf, 0, image_len*48);
    image_write_buf = image_buf;
    for (i = image_start; i < file->sum; i++) {
        sha1sum_buf = bin[i]->bin_sha1sum + strlen("sha1sum ");
        memcpy(image_write_buf, sha1sum_buf, strlen(sha1sum_buf));
        image_write_buf += strlen(sha1sum_buf);
        sha1sum_len += strlen(sha1sum_buf);
    }
    /* There is '\n' at end of sha1sum file */
    image_buf[sha1sum_len] = '\n';
    printf("image_buf: %s\n",image_buf);
    if(verify_sign(PUB_KEY_FILE, image_buf, sigbuf_in)) {
        ret = 0;
    }

failed:
    if (image_buf)
        free(image_buf);
    if (sigbuf_in)
        free(sigbuf_in);
    return ret;
}
#endif
int parse_update_bin(int fd, struct file_head *file, struct bin_head **bin) {
    int i = 0;
    uint8_t *buf = NULL;
    char sha1sum[48] = {0};
    int len = 0;
    int ret = 0;
    int ptype = 0;
    int uboot_ab_flashed = 0;
    int recovery_ab_flashed = 0;
    struct boot_cmd cmd;
    char dev_name[256] = {0};
    int diff = 0;

    memset(&cmd, 0, sizeof(cmd));

    if (get_recovery_cmd_status(&cmd)) {
        return -1;
    }
    printf("get last flashed partition: %s\n", cmd.recovery_partition);

    ptype = get_upgrade_partition_type(YODABASE_FSTAB);

#ifdef OTA_AUTH
    if (strcmp(bin[0]->sub_type, "sign") == 0) {
        if (ota_auth_check(fd, file, bin) != 0) {
            printf("ota_auth_check fail\n");
            return -1;
        }
        printf("ota_auth_check success\n");
    } else {
        printf("no signature section for ota_auth\n");
        return -1;
    }
#endif
    if (strcmp(bin[VERSION_OFFSET]->sub_type, "version") == 0) {
        if (rokid_check_diff_version(fd, file, bin, &cmd) != 0) {
            return -1;
        }
        diff = 1;
    }
    for (i = 0; i < file->sum; i++) {
        buf = (uint8_t *)malloc(bin[i]->bin_size);
        if (buf == NULL) {
            printf("malloc error %d\n", errno);
            return -1;
        }
        memset(buf, 0, bin[i]->bin_size);

        lseek(fd, bin[i]->start_position, SEEK_SET);

        len = read(fd, buf, bin[i]->bin_size);
        if (len != bin[i]->bin_size) {
            return -2;
        }

        calc_sha1sum(buf, bin[i]->bin_size, sha1sum);

        memset(dev_name, 0, sizeof(dev_name));

        if (strncmp(sha1sum, bin[i]->bin_sha1sum, sizeof(sha1sum)) == 0) {
            if (ptype == MTD_AB_FORMAT) {
	        printf("ota_unpack: this is mtd type\n");
            if (strncmp(cmd.recovery_partition, RECOVERY_PART_B, strlen(RECOVERY_PART_B)) == 0) {
                if (strncmp(bin[i]->sub_type, BIN_UBOOT, strlen(BIN_UBOOT)) == 0) {
                    strncpy(bin[i]->sub_type, UBOOT_A, strlen(UBOOT_A));
                    erase_uboot_env_part();
		    uboot_ab_flashed = 1;
                }
                if (strncmp(bin[i]->sub_type, BIN_RECOVERY, strlen(BIN_RECOVERY)) == 0) {
                    strncpy(bin[i]->sub_type, RECOVERY_A, strlen(RECOVERY_A));
		    recovery_ab_flashed = 1;
                }
            } else {
                if (strncmp(bin[i]->sub_type, BIN_UBOOT, strlen(BIN_UBOOT)) == 0) {
                    strncpy(bin[i]->sub_type, UBOOT_B, strlen(UBOOT_B));
                    erase_uboot_env_part();
		    uboot_ab_flashed = 1;
                }
                if (strncmp(bin[i]->sub_type, BIN_RECOVERY, strlen(BIN_RECOVERY)) == 0) {
                    strncpy(bin[i]->sub_type, RECOVERY_B, strlen(RECOVERY_B));
		    recovery_ab_flashed = 1;
                }
            }
	    }

	    if (ptype == UBI_OVERLAY_FORMAT) {
	        if (strcmp(bin[i]->ubi_dev, "")) {
                    if(diff && strcmp(bin[i]->diff_pack_dev, "")) {
                        diff_pack_install(fd, bin[i], buf);
                    } else {
                        int fd = open(bin[i]->ubi_dev, O_RDWR);
                        if (fd < 0) {
                            printf("open %s failed %s\n", bin[i]->ubi_dev, bin[i]->sub_type);
                            continue;
                        }
                        ioctl(fd, UBI_IOCVOLUP, &bin[i]->bin_size);
                        ret = write(fd, buf, bin[i]->bin_size);
                        printf("ota_unpack: updating sytem...\n");
                        close(fd);
                    }
		} else if (strcmp(bin[i]->raw_dev, "")) {
		    int len = 0;
		    int size = 0;
	            FILE *f = fopen(bin[i]->raw_dev, "w");
		    if (f == NULL) {
			printf("open %s error\n",bin[i]->raw_dev);
		    }
		    while(len < bin[i]->bin_size) {
			size = fwrite(buf + len, 2048, 1, f);
			printf("size :: %d\n", size);
			len += 2048; //block size
		    }
		    fclose(f);
		} else {
                    ret = get_mtd_name(bin[i]->sub_type, dev_name, YODABASE_FSTAB);
                    if (ret < 0) {
                        printf("get dev_file %s\n", bin[i]->sub_type);
                        continue;
                    }
                    nand_erase(dev_name, 0);
                    nand_write(dev_name, 0, buf, bin[i]->bin_size);
		}
                printf("UBI: finish updating partition name: %s, %s\n",
		       bin[i]->sub_type, dev_name);

	    } else if (ptype == MTD_AB_FORMAT) {
                ret = get_mtd_name(bin[i]->sub_type, dev_name, YODABASE_FSTAB);
                if (ret < 0) {
                    printf("get dev_file %s\n", bin[i]->sub_type);
                    continue;
                }
                if(diff && strcmp(bin[i]->diff_pack_dev, "")) {
                    diff_pack_install(fd, bin[i], buf);
                } else {
                    nand_erase(dev_name, 0);
                    nand_write(dev_name, 0, buf, bin[i]->bin_size);
                    printf("MTD_AB: finish updating partition name: %s:%s\n",
		            bin[i]->sub_type, dev_name);
                }
            } else if (ptype == MTD_FORMAT) {
                ret = get_mtd_name(bin[i]->sub_type, dev_name, YODABASE_FSTAB);
                if (ret < 0) {
                    printf("get dev_file %s\n", bin[i]->sub_type);
                    continue;
                }
                if(diff && strcmp(bin[i]->diff_pack_dev, "")) {
                    diff_pack_install(fd, bin[i], buf);
                } else {
                    nand_erase(dev_name, 0);
                    nand_write(dev_name, 0, buf, bin[i]->bin_size);
                    printf("MTD: finish updating partition name: %s:%s\n",
		            bin[i]->sub_type, dev_name);
                }
	    } else if(ptype == RAW_FORMAT) {
                int size;

                printf("start updating raw format partition: %s\n", bin[i]->sub_type);

                ret =  rawflash_get_name(bin[i]->sub_type,dev_name);

                if (ret < 0) {
                    printf("get dev_file %s fail\n", bin[i]->sub_type);
                    free(buf);
                    continue;
                }

                size = get_partition_size(dev_name);

                if(size < bin[i]->bin_size)
                {
                    printf("%s:partition size: %d < file size %"PRIu64"\n",dev_name,size,bin[i]->bin_size);
                    free(buf);
                    return -1;
                }

                if(diff && strcmp(bin[i]->diff_pack_dev, "")) {
                    diff_pack_install(fd, bin[i], buf);
                } else {
                    if(rawflash_write(dev_name,0,buf, bin[i]->bin_size))
                    {
                        printf("rawflash_write %s fail\n",dev_name);
                        free(buf);
                        return -1;
                    }
                }
            printf("finish updating raw format partition %s\n", bin[i]->sub_type);
       }

     }

        free(buf);
    }

    if (set_recovery_cmd_status(&cmd)) {
        return -1;
    }

    if (uboot_ab_flashed && recovery_ab_flashed) {
        if (strncmp(cmd.recovery_partition, RECOVERY_PART_B, strlen(RECOVERY_PART_B)) == 0)
            set_flashed_partition(FLASHED_PART_A);
        else
            set_flashed_partition(FLASHED_PART_B);
    }

    return 0;
}

int judge_need_upgrade(char *file)
{
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    if (get_recovery_cmd_status(&cmd)) {
        return -1;
    }

    printf("cmd: %s path: %s \nstate: %s flashed part: %s\n", cmd.boot_mode, cmd.recovery_path,
            cmd.recovery_state, cmd.recovery_partition);

    if ((strncmp(cmd.boot_mode, BOOTMODE_RECOVERY, strlen(BOOTMODE_RECOVERY)) == 0) &&
        (strncmp(cmd.recovery_state, BOOTSTATE_RUN, strlen(BOOTSTATE_RUN)) == 0)) {
        if (cmd.recovery_path != NULL) {
            strncpy(file, cmd.recovery_path, strlen(cmd.recovery_path) + 1);
            return OTA_UPDATE;
        }
    }

    if ((strncmp(cmd.boot_mode, BOOTMODE_FLUSHDATA, strlen(BOOTMODE_FLUSHDATA)) == 0) &&
        (strncmp(cmd.recovery_state, BOOTSTATE_RUN, strlen(BOOTSTATE_RUN)) == 0)) {
        printf("begin to flush data partition.\n");
        return DATA_FLUSHED;
    }

    return -1;
}

int flush_data_part() {
    int ret;
    int ptype;
    char data_device[32] = {0};
    char data_partname[32] = {0};

    ptype = get_upgrade_partition_type(YODABASE_FSTAB);

    if (ptype == UBI_OVERLAY_FORMAT) {
	ret = system("ubiattach /dev/ubi_ctrl -m 5");
	printf("ota_unpack: ubi attach %d %d\n", ret, errno);
	ret = system("ubiupdatevol -t /dev/ubi0_1");
	printf("ota_unpack: ubiupdatevol flash %d %d\n", ret, errno);
    } else if (ptype == RAW_FORMAT){
        ret = rawflash_format_userdata();
 
        if(ret < 0)
        {
            printf("rawflash_format_userdata fail\n");
            return ret;
        }
    }else {
        ret = get_data_partname(YODABASE_FSTAB, data_partname);
        if(ret < 0 || !strcmp(data_partname, ""))
        {
            printf("get_data_partname fail\n");
            return -1;
        }
        ret = get_mtd_name(data_partname, data_device, YODABASE_FSTAB);
        if (ret < 0) {
            printf("ota:failed to get data mtd name.\n");
            return ret;
        }
        nand_erase(data_device, DATA_FLUSH_PART_OFFSET);
    }

    return 0;
}

int ubi_data_mount() {
    int ret = 0;
    //ret = mkdir("/data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (ret < 0) {
        printf("mkdir data error\n");
        set_upgrade_statue(BOOTSTATE_RES_ERROR);
        return ret;
    }

    ret = system("ubiattach /dev/ubi_ctrl -m 5");
    printf("ota_unpack: ubi attach %d %d %d\n", __LINE__, ret, errno);

    ret = mount("ubi0:data", "/data", "ubifs", 0, NULL);
    if (ret < 0) {
        printf("ota_unpack: mount error\n");
        set_upgrade_statue(BOOTSTATE_RES_ERROR);
        return ret;
    }
    // ret = system("mount -t ubifs ubi0:data /data");
    printf("ota_unpack: mount result %d %d %d\n", __LINE__, ret, errno);
    return ret;
}

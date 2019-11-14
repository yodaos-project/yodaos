
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#include "recovery.h"
#include <mtd/mtd-user.h>

#if (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 0)
    static char *misc_mtd_device = "/dev/misc";
#endif

static  int g_partition_info_num = 0;
static struct partition_info  g_partition_info[MAX_PARTITIONS_NUM] = {0};
int get_mtd_name(char *logic_name, char *mtd_device, const char *fstab_file)
{
    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    char *index = NULL;
    char mtd_name[256] = {0};
    char part_name[64] = {0};
    int ret = -1;

    memset(mtd_device, 0, strlen(mtd_device));
    strcpy(part_name, "\"");
    strcat(part_name, logic_name);
    strcat(part_name, "\"");

    fp = fopen(fstab_file, "r");
    if (fp == NULL) {
        printf("%s open error %d %s \n", fstab_file, errno, strerror(errno));
        return ret;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if(strstr(line, "#") != NULL)
            continue;
        if (strstr(line, part_name) != NULL) {
            index = strchr(line, ':');
            if (index == NULL) {
                return -2;
            }

            strncpy(mtd_name, line, index - line);

            sprintf(mtd_device, "%s", mtd_name);
            ret = 0;
            //printf(" %s : mtd_device: %s\n", __func__, mtd_device);
            break;
        }
    }

    fclose(fp);
    return ret;
}

int get_mtd_num(char *mtd_device)
{
    int ret = -1;
    char *mtd_name;

    if (!mtd_device) {
        printf("No mtd device\n");
        return -1;
    }
    if ((mtd_name = strstr(mtd_device, "/dev/mtdblock")) != NULL) {
        mtd_name += strlen("/dev/mtdblock");
        ret = atoi(mtd_name);
    } else if ((mtd_name = strstr(mtd_device, "/dev/mtd")) != NULL) {
        mtd_name += strlen("/dev/mtd");
        ret = atoi(mtd_name);
    } else {
        printf("Invalid mtd device\n");
        return -1;
    }
    return ret;
}

int get_dev_ubinum(char *ubi_device)
{
    int ret = -1;
    char *ubi_name;

    if(!ubi_device) {
        printf("No ubi device\n");
        return -1;
    }
    if (strstr(ubi_device, "/dev/ubi") != NULL) {
        if((ubi_name = strsep(&ubi_device,"_")) != NULL) {
            ubi_name += strlen("/dev/ubi");
            ret = atoi(ubi_name);
        }
    }
    return ret;
}

int get_upgrade_partition_type(const char *fstab_file)
{
    FILE *fp;
    char *line =NULL;
    size_t len = 0;
    ssize_t read = 0;
    int partition_type = -1;

    fp = fopen(fstab_file, "r");
    if (fp == NULL) {
        printf("%s open error %d %s \n", fstab_file, errno,
	       strerror(errno));
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if(strstr(line, "#") != NULL)
            continue;
	if (strstr(line, "ubi_format") != NULL) {
            partition_type = UBI_OVERLAY_FORMAT;
	    printf("ota_unpack: this is ubi format!\n");
	    break;
        } else if (strstr(line, "mtd_format") != NULL) {
            partition_type = MTD_FORMAT;
            printf("ota_unpack: this is mtd format!\n");
	    break;
	} else if (strstr(line, "raw_format") != NULL) {
            partition_type = RAW_FORMAT;
            printf("ota_unpack: this is raw format!\n");
	    break;
	}
    }
    if(partition_type == -1)
        partition_type = MTD_AB_FORMAT;

    fclose(fp);
    return partition_type;
}

int get_max_ubinum(void)
{
    FILE * fp;
    int ubi_num = 0;
    char ubidev[32] = {0};

    while(ubi_num<MAX_PARTITIONS_NUM) {
        memset(ubidev, 0, sizeof(ubidev));
        sprintf(ubidev, "/dev/ubi%d", ubi_num);
        fp = fopen(ubidev, "r");
        if (fp == NULL) {
            printf("%s open error %d %s \n", ubidev, errno, strerror(errno));
            return ubi_num-1;
        } else {
            ubi_num++;
            fclose(fp);
        }
    }
    return ubi_num;
}

int get_max_ubivolnum(int ubi_num)
{
    FILE * fp;
    int ubivol_num = 0;
    char ubidev[32] = {0};

    while(ubivol_num<MAX_PARTITIONS_NUM) {
        memset(ubidev, 0, sizeof(ubidev));
        sprintf(ubidev, "/dev/ubi%d_%d", ubi_num, ubivol_num);
        fp = fopen(ubidev, "r");
        if (fp == NULL) {
            printf("%s open error %d %s \n", ubidev, errno, strerror(errno));
            return ubivol_num-1;
        } else {
            ubivol_num++;
            fclose(fp);
        }
    }
    return ubivol_num;
}

static int select_partname(const char *fstab_file, char *part_name, int select_num)
{
    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    char *index = NULL;

    if(!part_name) {
        printf("select_partname: invalid parameter part_name\n");
        return -1;
    }
    memset(part_name, 0, strlen(part_name));
    fp = fopen(fstab_file, "r");
    if (fp == NULL) {
        printf("%s open error %d %s \n", fstab_file, errno, strerror(errno));
        return -1;
    }
    while ((read = getline(&line, &len, fp)) != -1) {
        int count = 0;
        if(strstr(line, "#") != NULL)
            continue;
        if(strstr(line, "block_type") != NULL) {
            for(index = strsep(&line, " \t\n"); index != NULL; index = strsep(&line, " \t\n")) {
                if(!strcmp(index,""))
                    continue;
                ++count;
                if(count==select_num) {
                    strcpy(part_name, index);
                    break;
                }
            }
            break;
        }
    }
    fclose(fp);
    return 0;
}

int get_data_partname(const char *fstab_file, char *part_name)
{
    int ret;

    ret = select_partname(fstab_file, part_name, DATA_PARTNAME_POS);
    return ret;
}

int get_misc_partname(const char *fstab_file, char *part_name)
{
    int ret;

    ret = select_partname(fstab_file, part_name, MISC_PARTNAME_POS);
    return ret;
}

int get_env_partname(const char *fstab_file, char *part_name)
{
    int ret;

    ret = select_partname(fstab_file, part_name, ENV_PARTNAME_POS);
    return ret;
}


int get_partition_size(char *part_devname)
{
    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    int size = -1;
    char partitions_name[256] = {0};

    fp = fopen("/proc/partitions", "r");
    if (fp == NULL) {
	printf("/proc/partitions open error %d %s \n", errno, strerror(errno));
	return -1;
    }

    if(strstr(part_devname, "/dev/") != NULL)
	part_devname = part_devname + strlen("/dev/");

    if(strstr(part_devname, "mtd")!=NULL) {
        if(strstr(part_devname, "mtdblock")!=NULL) {
            strcpy(partitions_name, part_devname);
        }
        else {
            part_devname = part_devname + strlen("mtd");
            strcpy(partitions_name, "mtdblock");
            strcat(partitions_name, part_devname);
        }
    } else {
        strcpy(partitions_name, part_devname);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, partitions_name) != NULL) {
            char *token;
            int i = 0;
            for(token = strsep(&line, " \t"); token != NULL; token = strsep(&line, " \t")) {
                if(!strcmp(token,""))
                    continue;
                if(++i==3) {
                    size = atoi(token);
                    break;
                }
            }
            break;
        }
    }

    if(size != -1)
        size = size *1024;

    fclose(fp);
    return size;
}

int get_partitions_info(struct partition_info *part_tab, int max_partition_num, const char *fstab_file)
{
    FILE * fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read = 0;
    char *index = NULL;
    int i = 0;

    memset(part_tab, 0, sizeof(struct partition_info)*max_partition_num);
    fp = fopen(fstab_file, "r");
    if (fp == NULL) {
        printf("%s open error %d %s \n", fstab_file, errno, strerror(errno));
        return -1;
    }

    while ((read = getline(&line, &len, fp)) != -1 && i < max_partition_num) {
        int count = 0;
        if(strstr(line, "block_type") != NULL)
            continue;
        if(strstr(line, "#") != NULL)
            continue;
        if((index = strchr(line, ':'))!=NULL) {
            strncpy(part_tab[i].part_mount_info.mount_point, line, index - line);
        }
        if((index = strchr(line, '\"'))!=NULL) {
            char *end = NULL;
            index++;
            if((end = strchr(index, '\"'))!=NULL) {
                strncpy(part_tab[i].part_name, index, end - index);
            }
        }
        for(index = strsep(&line, " \t\n"); index != NULL; index = strsep(&line, " \t\n")) {
                if(!strcmp(index,""))
                    continue;
                ++count;
                if(count==3) {
                    strcpy(part_tab[i].part_mount_info.mount_format, index);
                    if(strcmp(part_tab[i].part_mount_info.mount_format, RAWFS)==0 || strcmp(part_tab[i].part_mount_info.mount_format, "")==0)
                        part_tab[i].part_format = RAW_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, UBIFS)==0)
                        part_tab[i].part_format = UBI_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, SQUASHFS)==0)
                        part_tab[i].part_format = SQUASH_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, EXT2FS)==0)
                        part_tab[i].part_format = EXT2_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, EXT3FS)==0)
                        part_tab[i].part_format = EXT3_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, EXT4FS)==0)
                        part_tab[i].part_format = EXT4_FS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, SQUASHOVERLAYUBIFS)==0)
                        part_tab[i].part_format = SQUASHFS_OVERLAY_UBIFS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, UBIOVERLAYUBIFS)==0)
                        part_tab[i].part_format = UBIFS_OVERLAY_UBIFS;
                    else if(strcmp(part_tab[i].part_mount_info.mount_format, VOLOVERLAYUBIFS)==0)
                        part_tab[i].part_format = VOL_OVERLAY_UBIFS;
                    else
                        part_tab[i].part_format = UNKNOWN_FS;
                } else if(count==4) {
                    strcpy(part_tab[i].part_mount_info.mount_path, index);
                } else if(count==5) {
                    strcpy(part_tab[i].part_mount_info.mount_param, index);
                }
        }
        i++;
    }

    fclose(fp);
    return i;
}

int flush_data(const char *fstab_file)
{
    char data_partname[32] = {0};
    char cmd[2048] = {0};
    struct partition_info part_info;
    int ret;
    int ubi_num = -1;
    int ubivol_num = -1;
    int mtd_num = -1;
    char *line = NULL;
    char *index = NULL;

    ret = get_data_partname(fstab_file, data_partname);
    if(ret) {
        printf("flush_data: get_data_partname failed %d\n",ret);
        return ret;
    }
    ret = get_rawflash_partition_info(data_partname, &part_info);
    if(ret) {
        printf("flush_data: get_rawflash_partition_info failed %d\n",ret);
        return ret;
    }
    if (part_info.part_format == SQUASHFS_OVERLAY_UBIFS || part_info.part_format == UBIFS_OVERLAY_UBIFS) {
        mtd_num = get_mtd_num(part_info.part_mount_info.mount_point);
        if(mtd_num < 0) {
            printf("mount_data:invalid mtd number.\n");
            return -1;
        }
        sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
        ret = system(cmd);
        ubi_num = get_max_ubinum();
        if(ubi_num < 0) {
            printf("flush_data: no ubidev for data partition\n");
            return -1;
        }
        ubivol_num = get_max_ubivolnum(ubi_num);
        if(ubivol_num < 1) {
            printf("flush_data: no ubivol for data partition");
            return -1;
        }
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "ubiupdatevol -t /dev/ubi%d_1", ubi_num);
        ret = system(cmd);
        if (ret) {
            printf("flush_data: ubiupdatevol error %d %d %s \n", ret, errno, strerror(errno));
            return -1;
        }
    } else if (part_info.part_format == VOL_OVERLAY_UBIFS) {
        char *mount_point[2] = {0};
        char ubi_dev[32] = {0};
        int mount_point_cnt = -1;
        int dev_ubinum = -1;

        if(!strcmp(part_info.part_mount_info.mount_point, "")) {
            printf("no mount point.\n");
            return 0;
        }
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
            printf("volume_overlay_ubifs error:invalid mount point\n");
            return -1;
        }
        strcpy(ubi_dev,mount_point[1]);
        ubi_num = get_max_ubinum();
        dev_ubinum = get_dev_ubinum(ubi_dev);
        if(dev_ubinum < 0) {
            printf("volume_overlay_ubifs error:invalid dev_ubinum.\n");
            return -1;
        }
	if(ubi_num == dev_ubinum-1) {
            mtd_num = get_mtd_num(mount_point[0]);
            if(mtd_num < 0) {
                printf("volume_overlay_ubifs error:invalid mtd number.\n");
                return -1;
            }
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            system(cmd);
        } else if(ubi_num < dev_ubinum-1) {
            printf("volume_overlay_ubifs error:invalid ubi_num\n");
            return -1;
        }
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "ubiupdatevol -t %s", mount_point[1]);
        ret = system(cmd);
        if (ret) {
            printf("flush_data: ubiupdatevol error %d %d %s \n", ret, errno, strerror(errno));
            return -1;
        }
    } else if (part_info.part_format == UBI_FS) {
        mtd_num = get_mtd_num(part_info.part_mount_info.mount_point);
        if(mtd_num < 0) {
            printf("mount_data:invalid mtd number.\n");
            return -1;
        }
        sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
        ret = system(cmd);
        ubi_num = get_max_ubinum();
        if(ubi_num < 0) {
            printf("flush_data: no ubidev for data partition\n");
            return -1;
        }
        ubivol_num = get_max_ubivolnum(ubi_num);
        if(ubivol_num < 0) {
            printf("flush_data: no ubivol for data partition");
            return -1;
        }
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "ubiupdatevol -t /dev/ubi%d_0", ubi_num);
        ret = system(cmd);
        if (ret) {
            printf("flush_data: ubiupdatevol error %d %d %s \n", ret, errno, strerror(errno));
            return -1;
        }
    } else if (part_info.part_format == EXT2_FS || part_info.part_format == EXT3_FS || part_info.part_format == EXT4_FS) {
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "umount %s", part_info.part_mount_info.mount_point);
        ret = system(cmd);
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "mkfs.%s -F %s", part_info.part_mount_info.mount_format, part_info.part_mount_info.mount_point);
        ret = system(cmd);
        if (ret) {
            printf("flush_data: %s: error %d %d %s \n", cmd, ret, errno, strerror(errno));
            return -1;
        }
    } else {
        printf("flush_data:unknown fs flush\n");
        return -1;
    }
    return 0;
}

int mount_data(const char *fstab_file)
{
    char data_partname[32] = {0};
    char cmd[2048] = {0};
    char mount_param[128] = {0};
    struct partition_info part_info;
    FILE * fp;
    char *line = NULL;
    char *index = NULL;
    size_t len = 0;
    ssize_t read = 0;
    int ret;
    int ubi_num = -1;
    int ubivol_num = -1;
    int mtd_num = -1;

    ret = get_data_partname(fstab_file, data_partname);
    if(ret) {
        printf("mount_data: get_data_partname failed %d\n",ret);
        return ret;
    }
    ret = get_rawflash_partition_info(data_partname, &part_info);
    if(ret) {
        printf("mount_data: get_rawflash_partition_info failed %d\n",ret);
        return ret;
    }
    if(!strcmp(part_info.part_mount_info.mount_path, "")) {
        printf("mount_data %s no mount path\n", part_info.part_name);
        return -1;
    }
    ret = system("mount > /tmp/mount_info");
    if(ret) {
        printf("mount_data:mount fail %d %d %s\n", __LINE__, errno, strerror(errno));
        return ret;
    }
    fp = fopen("/tmp/mount_info", "r");
    if (fp == NULL) {
	printf("/tmp/mount_info open error %d %s \n", errno, strerror(errno));
	return -1;
    }
    while ((read = getline(&line, &len, fp)) != -1) {
        if (strstr(line, part_info.part_mount_info.mount_path) != NULL) {
            break;
        }
    }
    if(read != -1) {
        printf("mount_data: %s \n", line);
        fclose(fp);
        return 0;
    }
    fclose(fp);
    if (part_info.part_format == SQUASHFS_OVERLAY_UBIFS || part_info.part_format == UBIFS_OVERLAY_UBIFS) {
        char *mount_path[2] = {0};
        int mount_path_cnt = -1;

        mtd_num = get_mtd_num(part_info.part_mount_info.mount_point);
        if(mtd_num < 0) {
            printf("mount_data:invalid mtd number.\n");
            return -1;
        }
        ubi_num = get_max_ubinum();
        sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
        ret = system(cmd);
        if(++ubi_num != get_max_ubinum()) {
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiformat /dev/mtd%d -y", mtd_num);
            ret = system(cmd);
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            ret = system(cmd);
            ubi_num = get_max_ubinum();
        }
        if(ubi_num < 0) {
            printf("mount_data: no ubi device avail %d\n", __LINE__);
            return -1;
        }
        ubivol_num = get_max_ubivolnum(ubi_num);
        if(ubivol_num < 1) {
            printf("mount_data: no enough ubi volume avail\n");
            return -1;
        }
        ret = system("sync");
        line = part_info.part_mount_info.mount_path;
        for(index = strsep(&line, ",\n"); index != NULL; index = strsep(&line, ",\n")) {
            if(!strcmp(index,""))
                continue;
            ++mount_path_cnt;
            if(mount_path_cnt > 1)
                break;
            mount_path[mount_path_cnt] = index;
        }
        if(mount_path_cnt < 1) {
            printf("mount_data:overlay_ubifs:invalid mount path\n");
            return -1;
        }
        memset(cmd, 0, sizeof(cmd));
        if(!strcmp(part_info.part_mount_info.mount_param, "")) {
            sprintf(cmd, "mount -t ubifs ubi%d:%s /%s", ubi_num, mount_path[1], mount_path[1]);
        } else {
            sprintf(cmd, "mount -t ubifs -o %s ubi%d:%s /%s", part_info.part_mount_info.mount_param, ubi_num, mount_path[1], \
				 mount_path[1]);
        }
        ret = system(cmd);
        if(ret) {
            printf("mount_data: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
            return -1;
        }
    } else if (part_info.part_format == UBI_FS) {
        mtd_num = get_mtd_num(part_info.part_mount_info.mount_point);
        if(mtd_num < 0) {
            printf("mount_data:invalid mtd number.\n");
            return -1;
        }
        ubi_num = get_max_ubinum();
        sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
        ret = system(cmd);
        if(++ubi_num != get_max_ubinum()) {
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiformat /dev/mtd%d -y", mtd_num);
            ret = system(cmd);
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            ret = system(cmd);
            ubi_num = get_max_ubinum();
        }
        if(ubi_num < 0) {
            printf("mount_data: no ubi device avail %d\n", __LINE__);
            return -1;
        }
        ubivol_num = get_max_ubivolnum(ubi_num);
        if(ubivol_num < 0) {
            printf("mount_data: no ubi volume avail %d\n", __LINE__);
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubimkvol /dev/ubi%d -N %s -m", ubi_num, part_info.part_name);
            ret = system(cmd);
        }
        ret = system("sync");
        memset(cmd, 0, sizeof(cmd));
        if(!strcmp(part_info.part_mount_info.mount_param, "")) {
            sprintf(cmd, "mount -t ubifs ubi%d_0 %s", ubi_num, part_info.part_mount_info.mount_path);
        } else {
            sprintf(cmd, "mount -t ubifs -o %s ubi%d_0 %s", part_info.part_mount_info.mount_param, ubi_num, part_info.part_mount_info.mount_path);
        }
        ret = system(cmd);
        if(ret) {
            printf("mount_data: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
            return -1;
        }
    } else if(part_info.part_format == VOL_OVERLAY_UBIFS) {
        char *mount_point[2] = {0};
        char ubi_dev[32] = {0};
        int mount_point_cnt = -1;
        int dev_ubinum = -1;

        if(!strcmp(part_info.part_mount_info.mount_point, "")) {
            printf("no mount point.\n");
            return 0;
        }
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
            printf("volume_overlay_ubifs error:invalid mount point\n");
            return -1;
        }
        strcpy(ubi_dev,mount_point[1]);
        ubi_num = get_max_ubinum();
        dev_ubinum = get_dev_ubinum(ubi_dev);
        if(dev_ubinum < 0) {
            printf("volume_overlay_ubifs error:invalid dev_ubinum.\n");
            return -1;
        }
        if(ubi_num == dev_ubinum-1) {
            mtd_num = get_mtd_num(mount_point[0]);
            if(mtd_num < 0) {
                printf("volume_overlay_ubifs error:invalid mtd number.\n");
                return -1;
            }
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            system(cmd);
        } else if(ubi_num < dev_ubinum-1) {
            printf("volume_overlay_ubifs error:invalid ubi_num\n");
            return -1;
        }
        if(!strcmp(part_info.part_mount_info.mount_path, "")) {
            printf("no mount path.\n");
            return 0;
        }
        memset(cmd, 0, sizeof(cmd));
        if(!strcmp(part_info.part_mount_info.mount_param, "")) {
            sprintf(cmd, "mount -t ubifs %s %s", mount_point[1], part_info.part_mount_info.mount_path);
        } else {
            sprintf(cmd, "mount -t ubifs -o %s %s %s", part_info.part_mount_info.mount_param, mount_point[1], part_info.part_mount_info.mount_path);
        }
        ret = system(cmd);
        if(ret) {
            printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
            return -1;
        }
    } else {
        memset(cmd, 0, sizeof(cmd));
        if(!strcmp(part_info.part_mount_info.mount_param, "")) {
            snprintf(cmd, sizeof(cmd), "mount -t %s %s %s", part_info.part_mount_info.mount_format, part_info.part_mount_info.mount_point, part_info.part_mount_info.mount_path);
        } else {
            int need_cat = 0;
            if(strstr(part_info.part_mount_info.mount_param, "e2fsck") != NULL) {
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "e2fsck -fy %s", part_info.part_mount_info.mount_point);
                ret = system(cmd);
                printf("%s:%d", cmd, ret);
                need_cat = 1;
            }
            if(strstr(part_info.part_mount_info.mount_param, "resize2fs") !=NULL) {
                memset(cmd, 0, sizeof(cmd));
                snprintf(cmd, sizeof(cmd), "resize2fs %s", part_info.part_mount_info.mount_point);
                ret = system(cmd);
                printf("%s:%d", cmd, ret);
                need_cat = 1;
            }
            if(need_cat) {
                int para_num = -1;
                memset(mount_param, 0, sizeof(mount_param));
                line = part_info.part_mount_info.mount_param;
                for(index = strsep(&line, ",\n"); index != NULL; index = strsep(&line, ",\n")) {
                    if(!strcmp(index,"") || !strcmp(index, "e2fsck") || !strcmp(index, "resize2fs"))
                        continue;
                    para_num++;
                    if(para_num > 0)
                        strcat(mount_param, ",");

                    strcat(mount_param, index);
                }
                printf("mount_param:%s\n", mount_param);
                if(para_num >= 0) {
                    snprintf(cmd, sizeof(cmd), "mount -t %s -o %s %s %s", part_info.part_mount_info.mount_format, mount_param, \
                        part_info.part_mount_info.mount_point, part_info.part_mount_info.mount_path);
                } else {
                    snprintf(cmd, sizeof(cmd), "mount -t %s %s %s", part_info.part_mount_info.mount_format, \
                        part_info.part_mount_info.mount_point, part_info.part_mount_info.mount_path);
                }
            } else {
                    snprintf(cmd, sizeof(cmd), "mount -t %s -o %s %s %s", part_info.part_mount_info.mount_format, part_info.part_mount_info.mount_param, \
                        part_info.part_mount_info.mount_point, part_info.part_mount_info.mount_path);
            }
        }
        ret = system(cmd);
        if(ret) {
            printf("mount_data: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int mount_partitions(struct partition_info *part_tab, int partition_count)
{
    int i = 0;
    char cmd[2048] = {0};
    char mount_param[128] = {0};
    int mtd_num = -1;
    int ret = -1;
    int ubi_num = -1;
    int ubivol_num = -1;
    char *index = NULL;
    char *line = NULL;

    if(!part_tab) {
        printf("Error: partition table is null\n");
        return -1;
    }
    if(partition_count <=0) {
        printf("Error: no partition mount\n");
        return -1;
    }
    for(i=0; i<partition_count; i++) {
        if(!strcmp(part_tab[i].part_name, ""))
            continue;
        if(part_tab[i].part_format == RAW_FS)
            continue;
        else if (part_tab[i].part_format == UBI_FS) {
            mtd_num = get_mtd_num(part_tab[i].part_mount_info.mount_point);
            if(mtd_num < 0) {
                printf("ubifs:invalid mtd number.\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_path, "")) {
                printf("ubifs %s no mount path\n", part_tab[i].part_name);
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            ubi_num = get_max_ubinum();
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            ret = system(cmd);
            if(++ubi_num != get_max_ubinum()) {
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiformat /dev/mtd%d -y", mtd_num);
                ret = system(cmd);
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
                ret = system(cmd);
                ubi_num = get_max_ubinum();
            }
            if(ubi_num < 0) {
                printf("mount_partitions: no ubidev avail %d\n", __LINE__);
                continue;
            }
            ubivol_num = get_max_ubivolnum(ubi_num);
            if(ubivol_num < 0) {
                printf("mount_partitions: no ubi volume avail %d\n", __LINE__);
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubimkvol /dev/ubi%d -N %s -m", ubi_num, part_tab[i].part_name);
                ret = system(cmd);
            }
            ret = system("sync");
            memset(cmd, 0, sizeof(cmd));
            if(!strcmp(part_tab[i].part_mount_info.mount_param, "")) {
                sprintf(cmd, "mount -t ubifs ubi%d_0 %s", ubi_num, part_tab[i].part_mount_info.mount_path);
            } else {
                sprintf(cmd, "mount -t ubifs -o %s ubi%d_0 %s", part_tab[i].part_mount_info.mount_param, ubi_num, part_tab[i].part_mount_info.mount_path);
            }
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
        } else if (part_tab[i].part_format == SQUASHFS_OVERLAY_UBIFS) {
            char *mount_path[2] = {0};
            int mount_path_cnt = -1;

            mtd_num = get_mtd_num(part_tab[i].part_mount_info.mount_point);
            if(mtd_num < 0) {
                printf("squash_overlay_ubifs:invalid mtd number.\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_path, "")) {
                printf("squash_overlay_ubifs %s no mount path.\n", part_tab[i].part_name);
                continue;
            } else {
                line = part_tab[i].part_mount_info.mount_path;
                for(index = strsep(&line, ",\n"); index != NULL; index = strsep(&line, ",\n")) {
                    if(!strcmp(index,""))
                        continue;
                    ++mount_path_cnt;
                    if(mount_path_cnt > 1)
                        break;
                    mount_path[mount_path_cnt] = index;
                }
            }
            if(mount_path_cnt < 1) {
                printf("squash_overlay_ubifs:invalid mount path\n");
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            ubi_num = get_max_ubinum();
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            ret = system(cmd);
            if(++ubi_num != get_max_ubinum()) {
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiformat /dev/mtd%d -y", mtd_num);
                ret = system(cmd);
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
                ret = system(cmd);
                ubi_num = get_max_ubinum();
            }
            if(ubi_num < 0) {
                printf("mount_partitions: no ubidev avail %d\n", __LINE__);
                continue;
            }
            ubivol_num = get_max_ubivolnum(ubi_num);
            if(ubivol_num < 1) {
                printf("mount_partitions: no enough ubi volume avail\n");
                continue;
            }
            ret = system("sync");
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "ubiblock --create /dev/ubi%d_0", ubi_num);
            ret = system(cmd);
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "mount -t squashfs -o ro /dev/ubiblock%d_0 %s", ubi_num, mount_path[0]);
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            if(!strcmp(part_tab[i].part_mount_info.mount_param, "")) {
                sprintf(cmd, "mount -t ubifs ubi%d:%s %s/%s", ubi_num, mount_path[1], mount_path[0], mount_path[1]);
            } else {
                sprintf(cmd, "mount -t ubifs -o %s ubi%d:%s %s/%s", part_tab[i].part_mount_info.mount_param, ubi_num, mount_path[1], \
				mount_path[0], mount_path[1]);
            }
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
        }else if (part_tab[i].part_format == UBIFS_OVERLAY_UBIFS) {
            char *mount_path[2] = {0};
            int mount_path_cnt = -1;

            mtd_num = get_mtd_num(part_tab[i].part_mount_info.mount_point);
            if(mtd_num < 0) {
                printf("ubi_overlay_ubifs:invalid mtd number.\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_path, "")) {
                printf("ubi_overlay_ubifs %s no mount path.\n", part_tab[i].part_name);
                continue;
            } else {
                line = part_tab[i].part_mount_info.mount_path;
                for(index = strsep(&line, ",\n"); index != NULL; index = strsep(&line, ",\n")) {
                    if(!strcmp(index,""))
                        continue;
                    ++mount_path_cnt;
                    if(mount_path_cnt > 1)
                        break;
                    mount_path[mount_path_cnt] = index;
                }
            }
            if(mount_path_cnt < 1) {
                printf("ubi_overlay_ubifs:invalid mount path\n");
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            ubi_num = get_max_ubinum();
            sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
            ret = system(cmd);
            if(++ubi_num != get_max_ubinum()) {
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiformat /dev/mtd%d -y", mtd_num);
                ret = system(cmd);
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
                ret = system(cmd);
                ubi_num = get_max_ubinum();
            }
            if(ubi_num < 0) {
                printf("mount_partitions: no ubidev avail %d\n", __LINE__);
                continue;
            }
            ubivol_num = get_max_ubivolnum(ubi_num);
            if(ubivol_num < 1) {
                printf("mount_partitions: no enough ubi volume avail\n");
                continue;
            }
            ret = system("sync");
            memset(cmd, 0, sizeof(cmd));
            sprintf(cmd, "mount -t ubifs -o rw /dev/ubi%d_0 %s", ubi_num, mount_path[0]);
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_param, "")) {
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "mount -t ubifs ubi%d:%s %s/%s", ubi_num, mount_path[1], mount_path[0], mount_path[1]);
            } else {
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "mount -t ubifs -o %s ubi%d:%s %s/%s", part_tab[i].part_mount_info.mount_param, ubi_num, mount_path[1], \
				mount_path[0], mount_path[1]);
            }
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
        } else if(part_tab[i].part_format == VOL_OVERLAY_UBIFS) {
            char *mount_point[2] = {0};
            char ubi_dev[32] = {0};
            int mount_point_cnt = -1;
            int dev_ubinum = -1;

            if(!strcmp(part_tab[i].part_mount_info.mount_point, "")) {
                printf("no mount point.\n");
                continue;
            }
            line = part_tab[i].part_mount_info.mount_point;
            for(index = strsep(&line, ", \t\n"); index != NULL; index = strsep(&line, ", \t\n")) {
                if(!strcmp(index,""))
                    continue;
                ++mount_point_cnt;
                if(mount_point_cnt > 1)
                    break;
                mount_point[mount_point_cnt] = index;
            }
            if(mount_point_cnt < 1) {
                printf("volume_overlay_ubifs error:invalid mount point\n");
                continue;
            }
            strcpy(ubi_dev,mount_point[1]);
            ubi_num = get_max_ubinum();
            dev_ubinum = get_dev_ubinum(ubi_dev);
            if(dev_ubinum < 0) {
                printf("volume_overlay_ubifs error:invalid dev_ubinum.\n");
                continue;
            }
            if(ubi_num == dev_ubinum-1) {
                mtd_num = get_mtd_num(mount_point[0]);
                if(mtd_num < 0) {
                    printf("volume_overlay_ubifs error:invalid mtd number.\n");
                    continue;
                }
                memset(cmd, 0, sizeof(cmd));
                sprintf(cmd, "ubiattach /dev/ubi_ctrl -m %d", mtd_num);
                system(cmd);
            } else if(ubi_num < dev_ubinum-1) {
                printf("volume_overlay_ubifs error:invalid ubi_num\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_path, "")) {
                printf("no mount path.\n");
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            if(!strcmp(part_tab[i].part_mount_info.mount_param, "")) {
                sprintf(cmd, "mount -t ubifs %s %s", mount_point[1], part_tab[i].part_mount_info.mount_path);
            } else {
                sprintf(cmd, "mount -t ubifs -o %s %s %s", part_tab[i].part_mount_info.mount_param, mount_point[1], part_tab[i].part_mount_info.mount_path);
            }
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
        } else {
            if(!strcmp(part_tab[i].part_mount_info.mount_point, "")) {
                printf("no mount point.\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_format, "")) {
                printf("no mount format.\n");
                continue;
            }
            if(!strcmp(part_tab[i].part_mount_info.mount_path, "")) {
                printf("no mount path.\n");
                continue;
            }
            memset(cmd, 0, sizeof(cmd));
            if(!strcmp(part_tab[i].part_mount_info.mount_param, "")) {
                sprintf(cmd, "mount -t %s %s %s", part_tab[i].part_mount_info.mount_format, part_tab[i].part_mount_info.mount_point, part_tab[i].part_mount_info.mount_path);
            } else {
                int need_cat = 0;
                if(strstr(part_tab[i].part_mount_info.mount_param, "e2fsck") != NULL) {
                    memset(cmd, 0, sizeof(cmd));
                    sprintf(cmd, "e2fsck -fy %s", part_tab[i].part_mount_info.mount_point);
                    ret = system(cmd);
                    printf("%s:%d", cmd, ret);
                    need_cat = 1;
                }
                if(strstr(part_tab[i].part_mount_info.mount_param, "resize2fs") !=NULL) {
                    memset(cmd, 0, sizeof(cmd));
                    sprintf(cmd, "resize2fs %s", part_tab[i].part_mount_info.mount_point);
                    ret = system(cmd);
                    printf("%s:%d", cmd, ret);
                    need_cat = 1;
                }
                if(need_cat) {
                    int para_num = -1;
                    memset(mount_param, 0, sizeof(mount_param));
                    line = part_tab[i].part_mount_info.mount_param;
                    for(index = strsep(&line, ",\n"); index != NULL; index = strsep(&line, ",\n")) {
                        if(!strcmp(index,"") || !strcmp(index, "e2fsck") || !strcmp(index, "resize2fs"))
                            continue;
                        para_num++;
                        if(para_num > 0)
                            strcat(mount_param, ",");

                        strcat(mount_param, index);
                    }
                    printf("mount_param:%s\n", mount_param);
                    if(para_num >= 0) {
                        sprintf(cmd, "mount -t %s -o %s %s %s", part_tab[i].part_mount_info.mount_format, mount_param, \
                            part_tab[i].part_mount_info.mount_point, part_tab[i].part_mount_info.mount_path);
                    } else {
                        sprintf(cmd, "mount -t %s %s %s", part_tab[i].part_mount_info.mount_format, \
                            part_tab[i].part_mount_info.mount_point, part_tab[i].part_mount_info.mount_path);
                    }
                } else {
                    sprintf(cmd, "mount -t %s -o %s %s %s", part_tab[i].part_mount_info.mount_format, part_tab[i].part_mount_info.mount_param, \
                        part_tab[i].part_mount_info.mount_point, part_tab[i].part_mount_info.mount_path);
                }
            }
            ret = system(cmd);
            if(ret) {
                printf("mount_partitions: %s : %d %d %d %s\n", cmd, __LINE__, ret, errno, strerror(errno));
                continue;
            }
        }
    }
    return 0;
}

int get_rawflash_partition_info(char *logic_name,struct partition_info *info)
{
    int i;
    int num=0;

    if(g_partition_info_num == 0)
    {

      if((num = get_partitions_info(g_partition_info,MAX_PARTITIONS_NUM, YODABASE_FSTAB)) <= 0)
      {
          printf("get_partitions_info error\n");
          return -1;
      }
      else
       g_partition_info_num = num;
   }

   for(i=0;i<g_partition_info_num;i++) 
   {
       struct partition_info *part = &g_partition_info[i];
//       printf("part[%d] name:%s,mount_point:%s,mount_format:%s,mount_path:%s mount_parm:%s,part_format:%d\n",i,part->part_name,part->part_mount_info.mount_point,part->part_mount_info.mount_format,part->part_mount_info.mount_path,part->part_mount_info.mount_param,part->part_format); 

      if(strcmp(logic_name,part->part_name)==0)
      {
          memcpy(info,part,sizeof(struct partition_info));
          return 0;
      }
   }
   
   return -1;
}

int rawflash_get_name(char *logic_name, char *device)
{
  struct partition_info info,*pInfo = &info;

  if(get_rawflash_partition_info(logic_name,pInfo))
  {
    printf("get partitions %s info error\n",logic_name);
    return -1;
  }

  strcpy(device,pInfo->part_mount_info.mount_point);
  
  return 0;
}

int rawflash_read(const char *device_name, const int offset, uint8_t *file_buf, uint64_t len)
{
    int ret,iret=0;
    int fd = 0;

    fd = open(device_name, O_RDONLY);

    if (fd < 0) {
        printf("open misc error\n");
        return -1;
    }

    lseek(fd, offset, SEEK_SET);

    ret = read(fd,file_buf,len);
    if (ret != len) {
        iret= -1;
    }

    close(fd);
    return iret;
}


int rawflash_write(const char *device_name,  const int offset, uint8_t *file_buf, uint64_t len)
{
    int ret,iret=0;
    int fd = 0;

    fd = open(device_name, O_WRONLY);

    if (fd < 0) {
        printf("open misc error\n");
        return -1;
    }

    lseek(fd, offset, SEEK_SET);

    ret = write(fd,file_buf,len);
    if (ret != len) {
      iret = -1;
    }

    close(fd);

   return iret;
}

int rawflash_format_userdata(void)
{
  int iret = -1;
  char buf[256];
  struct partition_info info,*pInfo = &info;
  char data_partname[32] = {0};

  iret = get_data_partname(YODABASE_FSTAB, data_partname);
  if(iret < 0 || !strcmp(data_partname, ""))
  {
    printf("get_data_partname fail\n");
    return -1;
  }
  if(get_rawflash_partition_info(data_partname,pInfo))
  {
    printf("get partitions %s info error\n",data_partname);
    return -1;
  }
  memset(buf,0,256);

  if(pInfo->part_format == EXT2_FS)
  {
    printf("format ext2 user data\n");
    snprintf(buf,256,"umount %s",pInfo->part_mount_info.mount_point);
    printf("%s\n",buf);
    iret = system(buf);
    snprintf(buf,256,"mkfs.ext2 -F %s",pInfo->part_mount_info.mount_point);
    printf("%s\n",buf);
    iret = system(buf);
  }
  return iret;  
}

int get_recovery_cmd_status(struct boot_cmd *cmd)
{

  if(get_upgrade_partition_type(YODABASE_FSTAB) == RAW_FORMAT)
  {
    char misc_device[32] = {0};
    char misc_partname[32] = {0};
    int ret;

    ret = get_misc_partname(YODABASE_FSTAB, misc_partname);
    if(ret < 0 || !strcmp(misc_partname, ""))
    {
        printf("get_misc_partname fail\n");
        return -1;
    }
    if(rawflash_get_name(misc_partname,misc_device) != 0)
    {
        printf("rawflash_get_name error\n");
        return -1;
    }

   if(rawflash_read(misc_device,0,(uint8_t *)cmd,sizeof(struct boot_cmd)))
   {
        printf("read %s error\n",misc_device);
        return -1;
   }
  }
  else
  {
    int ret = 0;
    uint64_t r_len = sizeof(struct boot_cmd);

#if (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 0)
    int fd = 0;
    fd = open(misc_mtd_device, O_RDONLY);
    if (fd < 0) {
        printf("open misc error\n");
        return -1;
    }

    ret = read(fd, cmd, sizeof(struct boot_cmd));
    if (ret != sizeof(struct boot_cmd)) {
        close(fd);
        return -1;
    }

    close(fd);

#elif (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 1)
    //fd = open("/dev/mtdblock2", O_RDONLY);
    char misc_device[32] = {0};
    char misc_partname[32] = {0};

    ret = get_misc_partname(YODABASE_FSTAB, misc_partname);
    if(ret < 0 || !strcmp(misc_partname, ""))
    {
        printf("get_misc_partname fail\n");
        return -1;
    }
    ret = get_mtd_name(misc_partname, misc_device, YODABASE_FSTAB);
    if (ret < 0) {
        return -2;
    }

    nand_read(misc_device, MISC_UPGRADE_NAND_ERASE_PART, (uint8_t *)cmd, r_len);
#endif
  }
    return 0;
}


#if (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 1)
int nand_erase_part(const char *devicename, const int offset, const int size) {
    int fd;
    int ret = 0;
    struct stat st;
    mtd_info_t meminfo;
    erase_info_t erase;
    int erase_part = 0;
    int bad_blocks = 0;

    //open mtd device
    fd = open(devicename, O_RDWR);
    if (fd < 0) {
        printf("open %s failed!\n", devicename);
        return -1;
    }

    //check is a char device
    ret = fstat(fd, &st);
    if (ret < 0) {
        printf("fstat %s failed!\n", devicename);
        close(fd);
        return -1;
    }

    if (!S_ISCHR(st.st_mode)) {
        printf("%s: not a char device", devicename);
        close(fd);
        return -1;
    }

    //get meminfo
    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    erase.length = meminfo.erasesize;

    if(offset % erase.length != 0) {
        printf("offset must mul of erasesize \n");
        return -2;
    }

    if (size % erase.length == 0) {
        erase_part = offset + size;
    } else {
        erase_part = ((size / erase.length) + 1) * erase.length + offset;
    }

    if ((erase_part >= meminfo.size) || (size == 0)) {
        erase_part = meminfo.size;
    }

    for (erase.start = offset; erase.start < erase_part+bad_blocks && erase.start < meminfo.size; erase.start += meminfo.erasesize) {
        loff_t bpos = erase.start;

        //check bad block
        ret = ioctl(fd, MEMGETBADBLOCK, &bpos);
        if (ret > 0 ) {
            printf("mtd: not erasing bad block at 0x%08"PRIu64"\n", bpos);
            bad_blocks += meminfo.erasesize;
            continue;  // Don't try to erase known factory-bad blocks.
        }

        if (ret < 0) {
            printf("MEMGETBADBLOCK error");
            close(fd);
            return -1;
        }

        //erase
        ioctl(fd, MEMUNLOCK, &erase);
        if (ioctl(fd, MEMERASE, &erase) < 0) {
            printf("mtd: erase failure at 0x%08"PRIu64"\n", bpos);
            close(fd);
            return -1;
        }
    }

    if(erase.start > meminfo.size)
       ret =  -1;

    close(fd);
    return ret;
}

int nand_erase(const char *devicename, const int offset) {
    return nand_erase_part(devicename, offset, 0);
}
#endif

int mtd_block_is_bad(int mtdtype, int fd, int offset)
{
	int r = 0;
	loff_t o = offset;

	if (mtdtype == MTD_NANDFLASH)
	{
		r = ioctl(fd, MEMGETBADBLOCK, &o);
		if (r < 0)
		{
			fprintf(stderr, "Failed to get erase block status\n");
			exit(1);
		}
	}
	return r;
}

static unsigned next_good_eraseblock(int fd, struct mtd_info_user *meminfo,
                                     unsigned block_offset)
{
    while (1) {
        loff_t offs;

        if (block_offset >= meminfo->size) {
            printf("not enough space in MTD device");
            return block_offset; /* let the caller exit */
        }

        offs = block_offset;
        if (ioctl(fd, MEMGETBADBLOCK, &offs) == 0)
            return block_offset;

        /* ioctl returned 1 => "bad block" */
        block_offset += meminfo->erasesize;
    }
}

int nand_read(const char *device_name, const int mtd_offset, uint8_t *file_buf, uint64_t len) {
    mtd_info_t meminfo;
    unsigned int blockstart;
    unsigned int limit = 0;
    int size = 0;
    int ret = 0;
    int offset = mtd_offset;

    int fd = open(device_name, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed!\n", device_name);
        return -1;
    }

    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    limit = meminfo.size;

    memset(file_buf, 0, len);

    //check offset page aligned
    if (offset & (meminfo.writesize - 1)) {
        printf("start address is not page aligned");
        close(fd);
        return -1;
    }

    char *tmp = (char *)malloc(meminfo.writesize);
    if (tmp == NULL) {
        printf("malloc %d size buffer failed!\n", meminfo.writesize);
        close(fd);
        return -1;
    }

    //if offset in a bad block, get next good block
    blockstart = offset & ~(meminfo.erasesize - 1);
    if (offset != blockstart) {
        unsigned int tmp;
        tmp = next_good_eraseblock(fd, &meminfo, blockstart);
        if (tmp != blockstart) {
     //       offset = tmp;
            offset = tmp + offset - blockstart;
        }
    }

    while(offset < limit) {
        blockstart = offset & ~(meminfo.erasesize - 1);
        if (blockstart == offset) {
            offset = next_good_eraseblock(fd, &meminfo, blockstart);

            if (offset >= limit) {
                printf("offset(%d) over limit(%d)\n", offset, limit);
                close(fd);
                free(tmp);
                return -1;
            }
        }

        lseek(fd, offset, SEEK_SET);

        size = read(fd, tmp, meminfo.writesize);
        if (size != meminfo.writesize) {
            printf("write err, need :%d, real :%d errno %d\n", meminfo.writesize, size, errno);
            close(fd);
            free(tmp);
            return -1;
        }

        if (len < meminfo.writesize) {
            memcpy(file_buf, tmp, len);
            file_buf += len;
            len = 0;
            break;
        } else {
            memcpy(file_buf, tmp, meminfo.writesize);
            len -= meminfo.writesize;
            file_buf += meminfo.writesize;
        }

        offset += meminfo.writesize;
    }
    close(fd);

    return 0;
}

#if 1
int nand_write(const char *device_name,  const int mtd_offset, uint8_t *file_buf, uint64_t len) {

    mtd_info_t meminfo;
    unsigned int blockstart;
    unsigned int limit = 0;
    int size = 0;
    int ret = 0;
    int offset = mtd_offset;
    int flags = O_RDWR | O_SYNC;

    //open mtd device
    int fd = open(device_name, flags);
    if (fd < 0) {
        printf("open %s failed!\n", device_name);
        return -1;
    }

    ret = ioctl(fd, MEMGETINFO, &meminfo);
    if (ret < 0) {
        printf("get MEMGETINFO failed!\n");
        close(fd);
        return -1;
    }

    limit = meminfo.size;
    //printf("meminfo.size 0x%x, meminfo.writesize 0x%x, meminfo.erasesize 0x%x\n", meminfo.size, meminfo.writesize, meminfo.erasesize);

    //check offset page aligned
    if (offset & (meminfo.writesize - 1)) {
        printf("start address is not page aligned");
        close(fd);
        return -1;
    }

    //malloc buffer for read

    char *tmp = (char *)malloc(meminfo.writesize);
    if (tmp == NULL) {
        printf("malloc %d size buffer failed!\n", meminfo.writesize);
        close(fd);
        return -1;
    }

    //if offset in a bad block, get next good block
    blockstart = offset & ~(meminfo.erasesize - 1);
    if (offset != blockstart) {
        unsigned int tmp_int;
        tmp_int = next_good_eraseblock(fd, &meminfo, blockstart);
        if (tmp_int != blockstart) {
          //offset = tmp_int;
            offset = tmp_int + offset - blockstart;
        }
    }

    while(offset < limit) {
        blockstart = offset & ~(meminfo.erasesize - 1);
        if (blockstart == offset) {
            offset = next_good_eraseblock(fd, &meminfo, blockstart);

            if (offset >= limit) {
                printf("offset(%d) over limit(%d)\n", offset, limit);
                close(fd);
                free(tmp);
                return -1;
            }
        }

        lseek(fd, offset, SEEK_SET);

        if (len < meminfo.writesize) {
            /* 0xff pad to end of write block */
            memcpy(tmp, file_buf, len);
            memset(tmp + len, 0xff, meminfo.writesize - len);
        } else {
            memcpy(tmp, file_buf, meminfo.writesize);
        }

        size = write(fd, tmp, meminfo.writesize);
        if (size != meminfo.writesize) {
            printf("write err, need :%d, real :%d\n", meminfo.writesize, size );
            close(fd);
            free(tmp);
            return -1;
        }

        if (len < meminfo.writesize) {
            file_buf += len;
            len = 0;
        } else {
            len -= meminfo.writesize;
            file_buf += meminfo.writesize;
        }

        offset += meminfo.writesize;
        if (len == 0) {
            break;
        }
    }

    // //free buf
    free(tmp);
    close(fd);

    return 0;//test
}
#else

int nand_write(const char *device_name,  const int mtd_offset, uint8_t *file_buf, uint64_t len) {
    int ret = -1;
    mtd_info_t mtdInfo;
    struct erase_info_user mtdLockInfo;
    int flags = O_RDWR | O_SYNC;
    int offset = mtd_offset;
    int erasesize, mtdsize;
    int result;

    //open mtd device
    int fd = open(device_name, flags);
    if (fd < 0) {
        printf("open %s failed!\n", device_name);
        return -1;
    }

    if(ioctl(fd, MEMGETINFO, &mtdInfo)) {
        fprintf(stderr, "Could not get MTD device info from %s\n", device_name);
        close(fd);
        return -1;
    }
    erasesize = mtdInfo.erasesize;
    mtdsize = mtdInfo.size;
    printf("mtdsize 0x%x, erasesize 0x%x, type %d\n", mtdsize, erasesize, mtdInfo.type);

    //check offset page aligned
    if (offset & (erasesize - 1)) {
        printf("start address is not page aligned");
        close(fd);
        return -1;
    }
    lseek(fd, offset, SEEK_SET);

    mtdLockInfo.start = offset;
    mtdLockInfo.length = mtdsize - offset;
    ioctl(fd, MEMUNLOCK, &mtdLockInfo);

    //malloc buffer for write
    char *tmp = (char *)malloc(erasesize);
    if (tmp == NULL) {
        printf("malloc %d size buffer failed!\n", erasesize);
        close(fd);
        return -1;
    }

    while(len) {
        if (mtd_block_is_bad(mtdInfo.type, fd, offset)) {
            printf("Skipping bad block at 0x%08zx\n ", offset);
            // Move the file pointer along over the bad block.
            lseek(fd, erasesize, SEEK_CUR);
            offset += erasesize;
            continue;
        }
        if (offset >= mtdsize) {
            printf("write overrun mtdsize%d, left %d not write yet!\n", mtdsize, (int)len);
            ret = -1;
            goto exit;
        }
        if (len < erasesize) {
            memcpy(tmp, file_buf, len);
            /* Pad block to eraseblock size */
            memset(&tmp[len], 0xff, erasesize - len);
            len = 0;
        } else {
            memcpy(tmp, file_buf, erasesize);
            file_buf += erasesize;
            len -= erasesize;
        }
        if ((result = write(fd, tmp, erasesize)) < erasesize) {
            ret = result;
            if (result < 0) {
                fprintf(stderr, "Error writing image.\n");
                goto exit;
            } else {
                fprintf(stderr, "Insufficient space.\n");
                goto exit;
            }
        }
        offset += erasesize;
    }

    ret = 0;

exit:
    free(tmp);
    close(fd);
    return ret;
}
#endif

int set_recovery_cmd_status(struct boot_cmd *cmd)
{

  if(get_upgrade_partition_type(YODABASE_FSTAB) == RAW_FORMAT)
  {
    char misc_device[32] = {0};
    char misc_partname[32] = {0};
    int ret;

    ret = get_misc_partname(YODABASE_FSTAB, misc_partname);
    if(ret < 0 || !strcmp(misc_partname, ""))
    {
        printf("get_misc_partname fail\n");
        return -1;
    }
    if(rawflash_get_name(misc_partname,misc_device) != 0)
    {
        printf("rawflash_get_name error\n");
        return -1;
    }

   if(rawflash_write(misc_device,0,(uint8_t *)cmd,sizeof(struct boot_cmd)))
   {
        printf("read %s error\n",misc_device);
        return -1;
   }
  }
  else {
#if (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 0)
    int fd = 0;
    int ret = 0;
    
    fd = open(misc_mtd_device, O_WRONLY);
    if (fd < 0) {
        printf("open misc error\n");
        return -1;
    }

    ret = write(fd, cmd, sizeof(struct boot_cmd));
    if (ret != sizeof(struct boot_cmd)) {
        close(fd);
        return -1;
    }

    close(fd);
#elif (ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE == 1)
    char misc_device[32] = {0};
    int ret = 0;
    char misc_partname[32] = {0};

    ret = get_misc_partname(YODABASE_FSTAB, misc_partname);
    if(ret < 0 || !strcmp(misc_partname, ""))
    {
        printf("get_misc_partname fail\n");
        return -1;
    }
    ret = get_mtd_name(misc_partname, misc_device, YODABASE_FSTAB);
    if (ret < 0) {
        return -2;
    }

    if (strncmp(cmd->recovery_partition, RECOVERY_PART_B, strlen(RECOVERY_PART_B)) != 0)
        strncpy(cmd->recovery_partition, RECOVERY_PART_A, strlen(RECOVERY_PART_A) + 1);
    else
        strncpy(cmd->recovery_partition, RECOVERY_PART_B, strlen(RECOVERY_PART_B) + 1);

    nand_erase_part(misc_device, MISC_UPGRADE_NAND_ERASE_PART, sizeof(struct boot_cmd));
    nand_write(misc_device, MISC_UPGRADE_NAND_ERASE_PART, (uint8_t*)cmd, sizeof(struct boot_cmd));
    printf("check flashed partition: %s\n", cmd->recovery_partition);

    erase_uboot_env_part();//TODO: erase uboot env
    printf("OTA prepare: Erase uboot env\n");

#endif
  }
 return 0;
}

int erase_uboot_env_part() {
    int ret;
    char env_device[32] = {0};
    char env_partname[32] = {0};

    ret = get_env_partname(YODABASE_FSTAB, env_partname);
    if(ret < 0 || !strcmp(env_partname, "")) {
        printf("get_env_partname fail\n");
        return -1;
    }

    ret = get_mtd_name(env_partname, env_device, YODABASE_FSTAB);
    if (ret < 0) {
        printf("ota:failed to get uboot env mtd name.\n");
        return ret;
    }

    if (strncmp(env_device, "/dev/", 5) != 0) {
        printf("ota: could not find env partiton, please ignore if platform is a113\n");
        return 0;
    }

    nand_erase(env_device, ENV_ERASE_PART_OFFSET);
    printf("ota: finish erasing uboot env\n");
    return 0;
}

int set_boot_flush_data()
{
    int ret = 0;
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    ret = get_recovery_cmd_status(&cmd);
    strncpy(cmd.boot_mode, BOOTMODE_FLUSHDATA, strlen(BOOTMODE_FLUSHDATA));
    strncpy(cmd.recovery_state, BOOTSTATE_READY, strlen(BOOTSTATE_READY));

    ret = set_recovery_cmd_status(&cmd);

    return ret;
}

int get_boot_flush_data()
{
    int ret = 0;
    struct boot_cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    ret = get_recovery_cmd_status(&cmd);
    if ((strncmp(cmd.boot_mode, BOOTMODE_FLUSHDATA, strlen(BOOTMODE_FLUSHDATA)) == 0) && \
        (strncmp(cmd.recovery_state, BOOTSTATE_OK, strlen(BOOTSTATE_OK)) ==0)) {
        return 0;
    }

    return ret;
}

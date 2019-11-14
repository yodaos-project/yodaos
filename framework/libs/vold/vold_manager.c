#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/ioctl.h>

#include <recovery/recovery.h>

int main(int argc, char **argv)
{
    int ret = 0;
    struct partition_info part_tab[MAX_PARTITIONS_NUM];
    int part_num = 0;
    int fstab_type = -1;

    fstab_type = get_upgrade_partition_type(YODABASE_FSTAB);
    if(fstab_type < 0) {
        printf("Invalid fstab type get\n");
        return -1;
    }
    printf("fstab type:%d\n", fstab_type);
    part_num = get_partitions_info(part_tab, MAX_PARTITIONS_NUM, YODABASE_FSTAB);
    ret = mount_partitions(part_tab, part_num);
    if(ret < 0)
        printf("mount_partitions failed\n");
    return ret;
}

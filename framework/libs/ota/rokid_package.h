#ifndef __ROKID_PACKAGE__
#define __ROKID_PACKAGE__

#include <inttypes.h>

#define ROKID_UPGRADE_CONFIGURE "./rokid_flash_package_bin.conf"
#define ROKID_OTA_CONFIGURE "./rokid_ota_package_bin.conf"

#define ROKID_DEFAULT_IMG "./upgrade.img"

/* file head 256, align by 4 bytes */
#pragma pack(4)
struct file_head {
    int sum;
    uint64_t start_position;
    uint64_t file_size;
    char reserved[236];
};
#pragma pack()


/* bin head 512, align by 4 bytes */
#pragma pack(4)
struct bin_head {
    int id;
    uint64_t start_position;
    uint64_t bin_size;
    uint64_t flash_start_position;
    uint64_t flash_part_size;
    char sub_type[32];
    char bin_sha1sum[48];
    char bin_version[64];
    char file_name[128];
    char diff_pack_dev[32];
    char ubi_dev[32];
    char raw_dev[32];
    char reserved[108];
};
#pragma pack()

#endif



#ifndef __UNPACK_H
#define __UNPACK_H

#ifdef __cplusplus
extern "C" {
#endif

#define MISC_UPGRADE_NAND_ERASE_PART 0x00000
#define DATA_FLUSH_PART_OFFSET 0x00000
#define ENV_ERASE_PART_OFFSET 0x00000

#define BOOTMODE_RECOVERY "boot_recovery"
#define BOOTMODE_FLUSHDATA "boot_flushdata"

#define BOOTSTATE_NONE "none"
#define BOOTSTATE_READY "recovery_ready"
#define BOOTSTATE_RUN "recovery_run"
#define BOOTSTATE_OK "recovery_ok"
#define BOOTSTATE_ERROR "recovery_error"

#define RECOVERY_PART_A "recovery_partition_a"
#define RECOVERY_PART_B "recovery_partition_b"

#define RECOVERYA_START "recoverya_start"
#define RECOVERYA_END "recoverya_end"
#define RECOVERYB_START "recoveryb_start"
#define RECOVERYB_END "recoveryb_end"

#define UBOOT_A "uboot_a"
#define UBOOT_B "uboot_b"
#define RECOVERY_A "recovery_a"
#define RECOVERY_B "recovery_b"

#define BIN_UBOOT "uboot"
#define BIN_RECOVERY "recovery"

#define DATA_PARTNAME_POS 3
#define MISC_PARTNAME_POS 4
#define ENV_PARTNAME_POS 5

#define MAX_PARTITIONS_NUM 32

#define RAWFS "raw"
#define UBIFS "ubifs"
#define SQUASHFS "squashfs"
#define EXT2FS "ext2"
#define EXT3FS "ext3"
#define EXT4FS "ext4"
#define SQUASHOVERLAYUBIFS "squashfs_overlay_ubifs"
#define UBIOVERLAYUBIFS "ubifs_overlay_ubifs"
#define VOLOVERLAYUBIFS "volume_overlay_ubifs"

#define YODABASE_FSTAB "/etc/yodabase.recovery.fstab"
#define PUB_KEY_FILE "/etc/rsa_public_key.pem"

/*
 * FILE HEAD is at the begining of OTA.img.
 * @MAX_BIN_SUM and @MAX_BIN_SIZE is limited for safe.
 */
#define MAX_BIN_SUM 12
#define MAX_BIN_SIZE (128 * 1024 * 1024)

enum {
    BOOTSTATE_RES_OK,
    BOOTSTATE_RES_ERROR,
};

enum {
    FLASHED_PART_A,
    FLASHED_PART_B,
};

enum {
    OTA_UPDATE = 0,
    DATA_FLUSHED,
};

struct boot_cmd {
    char boot_mode[32];
    char recovery_path[256];
    char recovery_state[32];
    char recovery_partition[32];
    char uboot_status[32];
    char recovery_status[32];
    char version[48];
    char reserve[144];
};

enum PARTITION_TYPE {
    MTD_AB_FORMAT = 0,
    MTD_FORMAT,
    UBI_OVERLAY_FORMAT,
    RAW_FORMAT,
};

typedef enum partition_fs {
    RAW_FS = 0,
    UBI_FS,
    SQUASH_FS,
    EXT2_FS,
    EXT3_FS,
    EXT4_FS,
    SQUASHFS_OVERLAY_UBIFS,
    UBIFS_OVERLAY_UBIFS,
    VOL_OVERLAY_UBIFS,
    UNKNOWN_FS,
} FS_FORMAT;

struct mount_info {
    char mount_point[256];
    char mount_format[64];
    char mount_path[64];
    char mount_param[128];
};

struct partition_info {
    char part_name[64];
    struct mount_info part_mount_info;
    FS_FORMAT part_format;
};


extern int rawflash_format_userdata(void);
extern int rawflash_get_name(char *logic_name, char *device);
extern int rawflash_read(const char *device_name, const int offset, uint8_t *file_buf, uint64_t len);
extern int rawflash_write(const char *device_name,  const int offset, uint8_t *file_buf, uint64_t len);
extern int get_upgrade_partition_type(const char *fstab_file);
extern int get_recovery_cmd_status(struct boot_cmd *cmd);
extern int set_recovery_cmd_status(struct boot_cmd *cmd);

extern int set_boot_flush_data();
extern int get_boot_flush_data();
extern int erase_uboot_env_part();
extern int nand_read(const char *device_name, const int mtd_offset, uint8_t *file_buf, uint64_t len);
extern int nand_write(const char *device_name,  const int mtd_offset, uint8_t *file_buf, uint64_t len);
extern int nand_erase(const char *devicename, const int offset);
extern int nand_erase_part(const char *devicename, const int offset, const int size);
extern int get_mtd_name(char *logic_name, char *mtd_device, const char *fstab_file);
extern int get_mtd_num(char *mtd_device);
extern int get_data_partname(const char *fstab_file, char *part_name);
extern int get_misc_partname(const char *fstab_file, char *part_name);
extern int get_env_partname(const char *fstab_file, char *part_name);
extern int get_max_ubinum(void);
extern int get_max_ubivolnum(int ubi_num);
extern int get_dev_ubinum(char *ubi_device);
extern int get_partition_size(char *part_devname);
extern int get_partitions_info(struct partition_info *part_tab, int max_partition_num, const char *fstab_file);
extern int flush_data(const char *fstab_file);
extern int mount_data(const char *fstab_file);
extern int mount_partitions(struct partition_info *part_tab, int partition_count);
extern int get_rawflash_partition_info(char *logic_name,struct partition_info *info);

#ifdef __cplusplus
}
#endif

#endif

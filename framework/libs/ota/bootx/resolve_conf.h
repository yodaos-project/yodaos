#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef WIN32
#include "winadapt.h"
#else
#include <unistd.h>
#include <stdint.h>
#endif
#include <stdlib.h>

#define FILE_NAME_MAX_LEN               256

#define FLASH_PARTITION_ADDR            (0x10000)
#define FLASH_TABLE_SEARCH_MAX          (0x400000)
#define FLASH_PARTITION_NAME_SIZE       (7)
#define MAGIC_SIZE                      (4)
#define PARTITION_NUM_SIZE              (1)

#define PARTITION_MAGIC                  0XAABCDEFA
#define FLASH_PARTITION_SIZE            (512)
#define FLASH_PARTITION_NUM             (12) // 最大只能支持 12 个分区
#define FLASH_PARTITION_HEAD_SIZE       (24) // 每一个分区表的大小
#define FLASH_PARTITION_CRC_ADDR        (FLASH_PARTITION_HEAD_SIZE * 17 + MAGIC_SIZE + PARTITION_NUM_SIZE)
#define FLASH_PARTITION_VERSION_ADDR    (FLASH_PARTITION_HEAD_SIZE * FLASH_PARTITION_NUM + MAGIC_SIZE + PARTITION_NUM_SIZE)

#define PARTITION_V2_MAGIC                  0XDD1C2BFA
#define FLASH_PARTITION_V2_SIZE            (1024)
#define FLASH_PARTITION_V2_NUM             (24)
#define FLASH_PARTITION_V2_HEAD_SIZE       (28)
#define FLASH_PARTITION_V2_CRC_ADDR   \
			(FLASH_PARTITION_V2_HEAD_SIZE * (FLASH_PARTITION_V2_NUM + 5) + MAGIC_SIZE + PARTITION_NUM_SIZE)
#define FLASH_PARTITION_V2_VERSION_ADDR   \
			(FLASH_PARTITION_V2_HEAD_SIZE * FLASH_PARTITION_V2_NUM + MAGIC_SIZE + PARTITION_NUM_SIZE)


#define RAW     0
#define ROMFS   1
#define CRAMFS  2
#define JFFS2   3
#define YAFFS2  4
#define RAMFS   5
#define HIDE    126
#define MINIFS  127

#define PARTITION_VERSION 102

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

struct partition_info {
	char     name[FLASH_PARTITION_NAME_SIZE + 1];
	uint32_t total_size;            // partition_size + reserved_size(for nand)
	uint32_t partition_size;
	uint32_t used_size;
	uint32_t reserved_size;
	uint32_t start_addr;
	char     file_system_type;
	char     mode;                   // : 7
	char     crc32_enable;           // : 1
	uint8_t  id;                     // : 7
	char     write_protect;          // : 1
	uint8_t  update_tags;            // : 2
	uint32_t crc32;
	uint16_t version;
};

/*
 * VERION 1.0.0 (<= V1.0.1-5)
 * +----------------------------+
 * |            512             |
 * +-------+-------+------------+
 * |   4   |   1   |    ...     |
 * +-------+-------+------------+
 * |       |       |            |
 * | MAGIC | COUNT | BASE TABLE |
 * |       |       |            |
 * +-------+-------+------------+
 *
 * VERION 1.0.2 ( <= V1.0.2-0-RC5)
 * +--------------------------------------------~----------------------------------------------+
 * |                                           512                                             |
 * +-------+-------+------------------------+---~----+---------+--------+----------+-----------+
 * |       |       |           360          |        |         |        |          |           |
 * |   4   |   1   +------------+-----------+  141   |    1    |    1   |    1     |     4     |
 * |       |       |  17 * 24   |  17 * 4   |        |         |        |          |           |
 * +-------+-------+------------+-----------+---~----+---------+--------+----------+-----------+
 * |       |       |            |           |        |         |        |          |           |
 * | MAGIC | COUNT | BASE TABLE | CRC TABLE | UNUSED | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+---~----+---------+--------+----------+-----------+
 * |0      |4      |5           |413                 |505      |506     |507        |508
 *
 * VERION 1.0.3
 * +-----------------------------------------------------------~----------------------------------------------+
 * |                                                   512                                                    |
 * +-------+-------+-------------------------------------------+--~-+---------+--------+----------+-----------+
 * |       |       |                                           |    |         |        |          |           |
 * |   4   |   1   +------------+-----------+------+-----------+ 45 |    1    |    1   |    1     |     4     |
 * |       |       |  12 * 24   |  12 * 2   |  96  |  12 * 4   |    |         |        |          |           |
 * +-------+-------+------------+-----------+------+-----------+--~-+---------+--------+----------+-----------+
 * |       |       |            |           |      |           |    |         |        |          |           |
 * | MAGIC | COUNT | BASE TABLE | VER TABLE |  UN  | CRC TABLE | UN | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |           |      |           |    |         |        |          |           |
 * +-------+-------+------------+-----------+------+-----------+--~-+---------+--------+----------+-----------+
 * |0      |4      |5           |293               |413                 |505      |506     |507        |508
 */


/*
 * VERION 1.0.4
 * +----------------------------~------------------------------------------------------------------------+
 * |                                                 512                                                 |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |       |       |            |      |             |         |         |        |          |           |
 * |   4   |   1   |  24 * 12   |  120 |    4 * 12   |   44    |    1    |    1   |    1     |     4     |
 * |       |       |            |      |             |         |         |        |          |           |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |       |       |            |      |             |         |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   |  UN  |  CRC TABLE  |   UN    | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |      |             |         |         |        |          |           |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |0      |4      |5           |293   |413          |461      |505      |506     |507       |508
 *
 * V2
 * +----------------------------~--------------------------------------------------------------------+
 * |                                                 1024                                            |
 * +-------+-------+------------+-----+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |     |           |        |         |        |          |           |
 * |   4   |   1   |  28 * 24   | 140 |   4 * 24  |   104  |    1    |    1   |    1     |     4     |
 * |       |       |            |     |           |        |         |        |          |           |
 * +-------+-------+------------+-----+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |     |           |        |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   | UN  | CRC TABLE |   UN   | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |     |           |        |         |        |          |           |
 * +-------+-------+------------+-----+-----------+--------+---------+--------|----------|-----------+
 * |0      |4      |5           |677  |817        |913     |1017     |1018    |1019      |1020
 */

struct partition_table {
	uint32_t              magic;                             // 0-3
	uint8_t               count;                             // 4
	struct partition_info tables[FLASH_PARTITION_V2_NUM];       // 5-364
	uint8_t               write_protect;                     // 505
	uint8_t               crc32_enable;                      // 506
	char                  version;                           // 507
	unsigned int          crc_verify;                        // 508-512
};

struct partition_data {
	struct partition_info *info;
	void                  *buf;
	void                  *fs;
	int                   crc32_passed;
	char                  *filename;
};

typedef struct {
	struct partition_data list[FLASH_PARTITION_V2_NUM];
	int count;

	size_t flash_size;
	size_t block_size;
	size_t table_addr;
	int    table_enable;
	struct partition_table flash_table;
	int    new_style;
	int    table_version;
}flash;

struct compress_partition_head_s{
	char     name[FLASH_PARTITION_NAME_SIZE + 1];
	uint32_t compress_size;
};

#define MERGE_HEAD_MAGIC              "mergebin"
#define MERGE_HEAD_MAGIC_SIZE         8

struct merge_sub_file_s {
	char name[FILE_NAME_MAX_LEN];
	uint32_t start_addr;
	uint32_t len;
};

struct merge_head_s {
	char magic[MERGE_HEAD_MAGIC_SIZE];
	int sub_file_num;
};

int image_split(const char *file_name, const char *dir, char *conf_file);
int rm_split_dir(const char *file_name, const char *dir);

#if 0
struct signature_partition_s{
	char sig_parts_name[56];
	char sig_reserved[200];
	char sig_key[256];
};
#endif

DLLEXPORT flash *flash_load_conf(const char *conf, char *bin_path);
DLLEXPORT flash *flash_load_bin (const char *filename);
DLLEXPORT flash *flash_simple_load_bin (const char *filename);
DLLEXPORT int    flash_free     (flash *entry);
DLLEXPORT int    flash_save     (flash *entry, const char *filename);
DLLEXPORT int    flash_simple_save     (flash *entry, const char *filename);
DLLEXPORT int    flash_dump     (flash *entry, const char *path);
DLLEXPORT int    flash_update   (flash *entry, const char *partition_name, void *buf, size_t size);
DLLEXPORT int    flash_update_f (flash *entry, const char *partition_name, const char *filename);
DLLEXPORT void   flash_printf   (flash *entry);
DLLEXPORT int    flash_rm       (flash *entry, const char *filename);
DLLEXPORT int    flash_add      (flash *entry, const char *full_file, const char *local_file);
DLLEXPORT int    flash_read     (flash *entry, const char *full_file, void *buf, size_t size, off_t offset);
DLLEXPORT int    flash_write    (flash *entry, const char *full_file, void *buf, size_t size);

DLLEXPORT struct partition_data *flash_get_partition(flash *entry, const char *partition_name);

DLLEXPORT int tools_default_db(flash *entry, const char *filename);
DLLEXPORT int tools_keymap    (flash *entry, const char *filename);
DLLEXPORT int tools_language  (flash *entry, const char *language);
DLLEXPORT int tools_dbtodef   (flash *entry);
DLLEXPORT int tools_datareset (flash *entry);
DLLEXPORT int tools_dbcopy(flash *entry, const char *dir);

DLLEXPORT int flash_add_release(flash *entry, unsigned int release_flag, char *str);

#endif


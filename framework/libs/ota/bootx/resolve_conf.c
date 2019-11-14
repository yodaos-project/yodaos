#include <stdio.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <libgen.h>
#include "util.h"
#include "resolve_conf.h"
#include <errno.h>

static int str2bool(const char *value)
{
	return (strcasecmp(value, "true") == 0 || strcasecmp(value, "TRUE") == 0) ? 1: 0;
}

static const char *get_fstype(int type)
{
	switch (type) {
		case RAW:    return "RAW"     ;
		case CRAMFS: return "CRAMFS"  ;
		case ROMFS:  return "ROMFS"   ;
		case JFFS2:  return "JFFS2"   ;
		case YAFFS2: return "YAFFS2"  ;
		case MINIFS: return "MINIFS"  ;
		case HIDE:   return "HIDE"    ;
		default:     return "UNKNOWN" ;
	}
}

static char *crc32str(uint32_t crc)
{
	static char buf[9] = {0, };

	if (crc == 0)
		memset(buf, ' ', 8);
	else
		sprintf(buf, "%08X", crc);

	return buf;
}

struct table_ver_adapt {
	unsigned int version;
	unsigned int magic;
	unsigned int size;
	unsigned int part_num;
};
struct table_ver_adapt table_sect = {
		1,
		PARTITION_MAGIC,
		FLASH_PARTITION_SIZE,
		FLASH_PARTITION_NUM,
};

static void init_table_sect(int version)
{
	if (version == 2) {
		table_sect.version = 2;
		table_sect.magic = PARTITION_V2_MAGIC;
		table_sect.size = FLASH_PARTITION_V2_SIZE;
		table_sect.part_num = FLASH_PARTITION_V2_NUM;
	} else {
		table_sect.version = 1;
		table_sect.magic = PARTITION_MAGIC;
		table_sect.size = FLASH_PARTITION_SIZE;
		table_sect.part_num = FLASH_PARTITION_NUM;
	}
}

static char *get_size_str(char *size_str, size_t size)
{
	if (size >= 1024 * 1024)
		sprintf(size_str, "%6.2f MB", size * 1.0 / 1024.0 / 1024.0);
	else if (size >= 1024)
		sprintf(size_str, "%6.2f KB", size * 1.0 / 1024.0);
	else
		sprintf(size_str, "%6zd  B", size);

	return size_str;
}

void flash_printf(flash* entry)
{
	int i;
	char size_str1[32], size_str2[32], size_str3[32], size_str4[32];

	printf("\n");
	printf("Partition Version   :  %d\n", entry->flash_table.version);
	printf("Flase Size          :%s\n", get_size_str(size_str1, entry->flash_size));
	printf("Partition Count     :  %d\n", entry->flash_table.count);
	printf("Partition MAX Count :  %d\n", table_sect.part_num);
	printf("Table Enable        :  %s\n", entry->table_enable ? "TRUE": "FALSE");
	printf("Write Protect       :  %s\n", entry->flash_table.write_protect ? "TRUE": "FALSE");
	printf("CRC32 Enable        :  %s\n", entry->flash_table.crc32_enable ? "TRUE": "FALSE");
	printf("Table CRC32         :  %X\n", entry->flash_table.crc_verify);

	printf("\n");

#define T_ID     "%-3s"
#define T_NAME   "%-8s"
#define T_MODE   "%-3s"
#define T_FS     "%-8s"
#define T_CRC    "%8s"
#define T_UPDATE "%8s"
#define T_VER    "%7s"
#define T_START  "%9s"
#define T_TSIZE  "%10s"
#define T_PSIZE  "%10s"
#define T_USIZE  "%10s"
#define T_PER    "%8s"
#define T_RSIZE  "%10s"
#define T_PASS   "%3s"
#define T_STR_FORMAT T_ID T_NAME T_MODE T_FS T_CRC T_UPDATE T_VER T_START T_TSIZE T_PSIZE T_USIZE T_PER T_RSIZE T_PASS"\n"

#define _ID     "%-3d"
#define _NAME   "%-8s"
#define _MODE   "%-3s"
#define _FS     "%-8s"
#define _CRC    "%-8s"
#define _UPDATE "%8u"
#define _VER    "%7u"
#define _START  " %08x"
#define _TSIZE  " %-10s"
#define _PSIZE  " %-10s"
#define _USIZE  "%-10s"
#define _PER    "%-5.1f%%  "
#define _RSIZE  "%-7s"
#define _PASS   "%3s"
#define STR_FORMAT _ID _NAME _MODE _FS _CRC _UPDATE _VER _START _TSIZE _PSIZE _USIZE _PER _RSIZE _PASS"\n"

	printf("==========================================================================================================\n");
	printf(T_STR_FORMAT,
			"ID", "NAME", "", "FS", "CRC32", "UPDATE", "VERION", "START", "SIZE", "PART", "USED", "USE%", "RESERVED", "OK");
	printf("==========================================================================================================\n");
	for (i=0; i < entry->count; i++) {
		printf(STR_FORMAT,
				entry->list[i].info->id,
				entry->list[i].info->name,
				entry->list[i].info->mode == 0 ? "RO" : "RW",
				get_fstype(entry->list[i].info->file_system_type),
				crc32str(entry->list[i].info->crc32),
				entry->list[i].info->update_tags,
				entry->list[i].info->version,
				entry->list[i].info->start_addr,
				get_size_str(size_str1, entry->list[i].info->total_size),
				get_size_str(size_str2, entry->list[i].info->partition_size),
				get_size_str(size_str3, entry->list[i].info->used_size),
				entry->list[i].info->used_size * 100.0 / entry->list[i].info->partition_size,
				get_size_str(size_str4, entry->list[i].info->reserved_size),
				entry->list[i].crc32_passed ? "*" : " "
				);
	}
	printf("----------------------------------------------------------------------------------------------------------\n\n");
}

int flash_compr(const void *a, const void *b)
{
	struct partition_data *a1 = (struct partition_data*)a;
	struct partition_data *b1 = (struct partition_data*)b;

	return a1->info->start_addr - b1->info->start_addr;
}

void flash_sort(flash *entry)
{
	qsort(entry->list, entry->count, sizeof(struct partition_data), flash_compr);
}

#define keycmp(s1, s2) strncasecmp(s1, s2, strlen(s2))

#define ID_NAME    0
#define ID_FILE    1
#define ID_CRC     2
#define ID_FS      3
#define ID_MODE    4
#define ID_UPDATE  5
#define ID_VERSION 6
#define ID_START   7
#define ID_SIZE    8
#define ID_RES_SIZE 9

static char *basedir;
flash *flash_load_conf(const char *conf, char *bin_path)
{
	FILE *fp = NULL;
	char str[1024], *key = NULL, *value = NULL;
	flash *entry = NULL;
	int i;
	unsigned int count = 0;
	unsigned int total_size = 0;
	char *conf_file = strdup(conf);

	if ((fp = fopen(conf, "r")) == NULL) {
		printf("error: can't open file \"%s\"!\n", conf);
		exit(1);
	}

	entry = (flash*)calloc(1, sizeof(flash));
	memset(entry, 0, sizeof(flash));

	entry->table_enable = 0;
	entry->flash_table.write_protect = 0;
	entry->flash_table.crc32_enable = 0;

	if (bin_path == NULL)
		basedir = dirname(conf_file);
	else
		basedir = bin_path;

	while (!feof(fp)) {
		memset(str, 0, 1024);
		fgets(str, 1023, fp);
		trim(str);
		if (strcmp(str, "") == 0 || str[0] == '#')
			continue;

		if (keycmp(str, "flash_size") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);
			entry->flash_size = str2int(value);
		}
		else if (keycmp(str, "block_size") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);
			entry->block_size = str2int(value);
		}
		else if (keycmp(str, "table_enable") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);

			entry->table_enable = str2bool(value);
		}
		else if (keycmp(str, "write_protect") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);

			entry->flash_table.write_protect = str2bool(value);
		}
		else if (keycmp(str, "crc32") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);

			entry->flash_table.crc32_enable = str2bool(value);
		}
		else if (keycmp(str, "zlibmode") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);

			entry->new_style = str2bool(value);
		}
		else if (keycmp(str, "table_version") == 0) {
			key = strtok(str, " \t");
			value = strtok(NULL, " \t");
			trim(value);

			entry->table_version = str2int(value);
			init_table_sect(entry->table_version);
		} else {
			int id = 0;
			struct partition_info *info = &entry->flash_table.tables[count];

			entry->list[count].info = info;
			memset(info, 0, sizeof(struct partition_info));
			id = 0;
			key = strtok(str, " \t");
			while (key) {
				char filename[256];
				int filename_len;
				void *buf;
				size_t size;
				const char *table_bin = "table.bin";
				char tmp_key[16];
				switch(id) {
					case ID_NAME:
						strncpy(info->name, key, 7);
						break;
					case ID_FILE:
						if (strcasecmp(info->name, "TABLE") == 0) {
							if (strcasecmp(key, "NULL") == 0) {
								strcpy(tmp_key, table_bin);
								key = tmp_key;
							}
						}
						if (strcasecmp(key, "NULL") != 0) {
							snprintf(filename, 255, "%s/%s", basedir, key);
							filename_len = strlen(filename);
							entry->list[count].filename = (char*)malloc(filename_len+1);
							memcpy(entry->list[count].filename, filename, filename_len);
							entry->list[count].filename[filename_len] = '\0';

							buf = x_fmmap(filename, &size);
							if (buf) {
								info->used_size = size;
								info->crc32 = 0;
								entry->list[count].buf = malloc(size);

								memcpy(entry->list[count].buf, buf, size);
								x_munmap(buf, size);
							}
						}
						break;
					case ID_CRC:
						info->crc32_enable = str2bool(key);
						break;
					case ID_FS:
						if (strcasecmp(key, "RAW") == 0)
							info->file_system_type = RAW;
						else if (strcasecmp(key, "CRAMFS") == 0)
							info->file_system_type = CRAMFS;
						else if (strcasecmp(key, "ROMFS") == 0)
							info->file_system_type = ROMFS;
						else if (strcasecmp(key, "JFFS2") == 0)
							info->file_system_type = JFFS2;
						else if (strcasecmp(key, "YAFFS2") == 0 || strcasecmp(key, "YAFFS") == 0)
							info->file_system_type = YAFFS2;
						else if (strcasecmp(key, "MINIFS") == 0)
							info->file_system_type = MINIFS;
						else if (strcasecmp(key, "RAMFS") == 0)
							info->file_system_type = RAMFS;
						else if (strcasecmp(key, "HIDE") == 0)
							info->file_system_type = HIDE;
						break;
					case ID_MODE:
						if (strcasecmp(key, "ro") == 0)
							info->mode = 0;
						else if (strcasecmp(key, "rw") == 0)
							info->mode = 1;
						break;
					case ID_UPDATE:
						info->update_tags = str2int(key);
						break;
					case ID_VERSION:
						info->version = str2int(key);
						break;
					case ID_START:
						if (strcasecmp(key, "auto") == 0)
							info->start_addr = -1;
						else
							info->start_addr = str2int(key);
						break;
					case ID_SIZE:
						if (strcasecmp(key, "auto") == 0) {
							info->total_size = -1;
							info->partition_size = -1;
						} else {
							info->total_size = str2int(key);
							info->partition_size = str2int(key);
						}
						break;
					case ID_RES_SIZE:
						if (table_sect.version == 2) {
							if (strcasecmp(key, "auto") == 0)
								info->reserved_size = -1;
							else
								info->reserved_size = str2int(key);
						} else {
							info->reserved_size = 0;
						}
						break;
				}

				id++;
				key = strtok(NULL, " \t");
				if (key)
					trim(key);
			}

			count++;

			if (count > table_sect.part_num) {
				printf("No more than %d the number of partitions.\n", table_sect.part_num);
				exit(1);
			}
		}
	}

	printf("\n");
	entry->count = count;
	fclose(fp);

	if (entry->list[0].info->start_addr == 0xffffffff)
		entry->list[0].info->start_addr = 0;
	if (entry->list[0].info->total_size == 0xffffffff)
		entry->list[0].info->total_size = entry->list[0].info->used_size;
	if (entry->list[0].info->reserved_size == 0xffffffff)
		entry->list[0].info->reserved_size = 0;

	for (i=1; i < entry->count; i++) {
		if (entry->list[i].info->total_size == 0xffffffff)
			entry->list[i].info->total_size = entry->list[i].info->used_size;
		if (entry->list[i].info->reserved_size == 0xffffffff)
			entry->list[i].info->reserved_size = 0;
		if (entry->list[i].info->start_addr == 0xffffffff) {
			entry->list[i].info->start_addr = entry->list[i-1].info->total_size + entry->list[i-1].info->start_addr;
			entry->list[i].info->start_addr = ((entry->list[i].info->start_addr + 0x10000 - 1) / 0x10000) * 0x10000;
		}
		if (strcasecmp(entry->list[i].info->name, "TABLE") == 0) {
			entry->table_enable = 1;
			entry->table_addr = entry->list[i].info->start_addr;
		}
		if (entry->list[i].info->file_system_type == MINIFS) {
			if (entry->list[i].info->start_addr % (64 * 1024) != 0) {
				printf("minifs start address must be aligned 64K\n");
				free(conf_file);
				flash_free(entry);

				return NULL;
			}
		}
	}

	for (i=0; i < entry->count; i++) {
		if (i == entry->count - 1)
			entry->list[i].info->total_size = entry->flash_size - entry->list[i].info->start_addr;
		else
			entry->list[i].info->total_size = entry->list[i + 1].info->start_addr - entry->list[i].info->start_addr;
		total_size += entry->list[i].info->total_size;
		if (total_size > entry->flash_size) {
			printf("Total partition size is %d, out flash size %zd, please check.\n", total_size, entry->flash_size);
			exit(1);
		}
		if (entry->list[i].info->reserved_size > entry->list[i].info->total_size) {
			printf("error: partition %s (RESERVED size = 0x%x) > (TOTAL size = 0x%x)\n",
					entry->list[i].info->name,
					entry->list[i].info->reserved_size,
					entry->list[i].info->total_size);
			printf("please check the config of %s\n\n", conf);
			exit(1);
		}
		entry->list[i].info->partition_size = entry->list[i].info->total_size - entry->list[i].info->reserved_size;
		entry->list[i].crc32_passed = 1;
	}

	for (i=0; i < entry->count; i++) {
		/* if used_size > partition_size, printf error info */
		if(entry->list[i].info->used_size > entry->list[i].info->partition_size){
			printf("error: partition %s (FILE size = 0x%x) > (PART size = 0x%x)\n",
					entry->list[i].info->name,
					entry->list[i].info->used_size,
					entry->list[i].info->partition_size);
			printf("please check the config of %s\n\n", conf);
			exit(1);
		}
	}

	entry->flash_table.magic = table_sect.magic;
	entry->flash_table.count = entry->count;

	flash_sort(entry);

	if(entry->new_style){
		for (i=0; i < entry->count; i++) {
			entry->list[i].info->id = i;
			if(entry->list[i].buf){
				entry->list[i].buf = realloc(entry->list[i].buf, entry->list[i].info->partition_size);
				memset((char*)entry->list[i].buf + entry->list[i].info->used_size, 0xFF, \
						entry->list[i].info->partition_size - entry->list[i].info->used_size);
			}
		}
	}else{
		for (i=0; i < entry->count; i++) {
			entry->list[i].info->id = i;
			entry->list[i].buf = realloc(entry->list[i].buf, entry->list[i].info->partition_size);
			memset((char*)entry->list[i].buf + entry->list[i].info->used_size, 0xFF, \
					entry->list[i].info->partition_size - entry->list[i].info->used_size);
		}
	}

	entry->flash_table.version = PARTITION_VERSION;

#ifndef WIN32
	flash_printf(entry);
#endif

	free(conf_file);
	return entry;
}

int flash_free(flash *entry)
{
	int i;

	for (i=0; i <entry->count; i++) {
		if (entry->list[i].buf != NULL){
			free(entry->list[i].buf);
		}
	}
	free(entry);

	return 0;
}

int image_split(const char *file_name, const char *dir, char *conf_file)
{
	int i;
	FILE *fp;
	char *buf;
	size_t file_size;
	struct merge_head_s merge_head;
	struct merge_sub_file_s *p_sub_file;
	char split_dir_name[FILE_NAME_MAX_LEN];
	char tmp_file_name[FILE_NAME_MAX_LEN];

	if ((buf = (char *)x_fmmap(file_name, &file_size)) == NULL) {
		printf("error: %s(%d) file(%s) x_fmmap failed\n", __func__, __LINE__, file_name);
		exit(1);
	}

	memcpy(&merge_head, buf, sizeof(merge_head));
	if (strncmp(merge_head.magic, MERGE_HEAD_MAGIC, MERGE_HEAD_MAGIC_SIZE) == 0) {
		if (dir == NULL)
			sprintf(split_dir_name, "%s.split", file_name);
		else
			strcpy(split_dir_name, dir);
#ifdef WIN32
		mkdir(split_dir_name);
#else
		mkdir(split_dir_name, S_IRWXU | S_IRWXG);
#endif
		p_sub_file = (struct merge_sub_file_s *)(buf + sizeof(merge_head));
		//printf("Split output file list:\n");
		for (i = 0; i < merge_head.sub_file_num; i++) {
			sprintf(tmp_file_name, "%s/%s", split_dir_name, p_sub_file[i].name);
			if ((fp = fopen(tmp_file_name, "wb")) == NULL) {
				printf("error: %s(%d) fopen error, errno(%s)\n", __func__, __LINE__, strerror(errno));
				return -1;
			}
			//printf("    %s\n", tmp_file_name);
			fwrite((buf + p_sub_file[i].start_addr), p_sub_file[i].len, 1, fp);
			fclose(fp);

			if (i == 0)
				strcpy(conf_file, tmp_file_name);
		}
	} else {
		printf("error: %s(%d) file(%s) is not merge file.\n", __func__, __LINE__, file_name);
		return -1;
	}

	return 0;
}

int rm_split_dir(const char *file_name, const char *dir)
{
	int i;
	char *buf;
	size_t file_size;
	struct merge_head_s merge_head;
	struct merge_sub_file_s *p_sub_file;
	char tmp_file_name[FILE_NAME_MAX_LEN];

	if ((buf = (char *)x_fmmap(file_name, &file_size)) == NULL) {
		printf("error: %s(%d) file(%s) x_fmmap failed\n", __func__, __LINE__, file_name);
		exit(1);
	}

	memcpy(&merge_head, buf, sizeof(merge_head));
	p_sub_file = (struct merge_sub_file_s *)(buf + sizeof(merge_head));
	for (i = 0; i < merge_head.sub_file_num; i++) {
		sprintf(tmp_file_name, "%s/%s", dir, p_sub_file[i].name);
		if (access(tmp_file_name, F_OK) == 0) {
			if (remove(tmp_file_name) < 0)
				printf("warning: remove file(%s) failed\n", tmp_file_name);
		}
	}

	if (remove(dir) < 0)
		printf("warning: remove dir(%s) failed\n", dir);

	return 0;

}


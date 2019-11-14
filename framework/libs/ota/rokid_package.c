#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "sha1.h"
#include "rokid_package.h"

struct bin_head *bin_head[36];
struct file_head *file_head = NULL;
char *upgrade_file = NULL;

int judge_line_islegal(char *line) {
    char *file_name_start = NULL;
    char *sub_type_start = NULL;
    char *start_position_start = NULL;
    char *flash_partsize_start = NULL;
    int start_offect = 0;
    int flash_size = 0;

    if (strncmp(line, "file_name", strlen("file_name")) != 0) {
        return -2;
    }

    file_name_start = strstr(line, "file_name=");

    sub_type_start = strstr(line, "sub_type=");

    start_position_start = strstr(line, "start_position=");

    flash_partsize_start = strstr(line, "flash_partsize=");

    if ((file_name_start == NULL) || (sub_type_start == NULL) ||
        (start_position_start == NULL) || (flash_partsize_start == NULL)) {
        return -1;
    }

    return 0;
}

int parse_line_to_bin(char *line, struct bin_head *bin) {
    char start_position[32] = {0};
    char flash_part_size[32] = {0};
    char *pos = NULL;
    char *end = NULL;
    int ret = 0;
    int i = 0;

    while (1) {
        pos = strstr(line, "\"");
        if (pos == NULL) {
            break;
        }
        end = strstr(pos + 1, "\"");
        if (end == NULL) {
            return -1;
        }

        if (strstr(line, "file_name")) {
            strncpy(bin->file_name, pos + 1, end - pos - 1);
        } else if (strstr(line, "sub_type")) {
            strncpy(bin->sub_type, pos + 1, end - pos - 1);
        } else if (strstr(line, "start_position")) {
            strncpy(start_position, pos + 1, end - pos - 1);

            bin->flash_start_position = strtol(start_position, NULL, 16);

        } else if (strstr(line, "flash_partsize")) {
            strncpy(flash_part_size, pos + 1, end - pos - 1);

            bin->flash_part_size = strtol(flash_part_size, NULL, 16);
        } else if (strstr(line, "diff_pack_dev")) {
            strncpy(bin->diff_pack_dev, pos + 1, end - pos - 1);
        } else if (strstr(line, "ubi_dev")) {
            strncpy(bin->ubi_dev, pos + 1, end - pos - 1);
        } else if (strstr(line, "raw_dev")) {
            strncpy(bin->raw_dev, pos + 1, end - pos - 1);
        }


        line = end + 1;
    }

    return 0;
}

int get_file_size(struct bin_head *bin) {
    int fd = 0;

    fd = open(bin->file_name, O_RDONLY);
    if(fd < 0)
    {
        return -1;
    }

    bin->bin_size = lseek(fd, 0, SEEK_END);

    close(fd);

    return 0;
}


int calc_file_sha1sum(struct bin_head *bin) {
    int fd = 0;
    char *buf = 0;
    int ret = 0;

    buf = malloc(bin->bin_size);
    if (buf == NULL) {
        return -1;
    }

    fd = open(bin->file_name, O_RDONLY);
    if(fd < 0) {
        return -2;
    }

    ret = read(fd, buf, bin->bin_size);
    if (ret != bin->bin_size) {
        return -3;
    }

    close(fd);

    calc_sha1sum(buf, bin->bin_size, bin->bin_sha1sum);

    return 0;
}


int make_upgrade_bin(struct file_head *file, struct bin_head **bin) {
    int i = 0;
    uint64_t bin_offset = 0;
    uint8_t *buf = NULL;
    uint8_t *offset = NULL;
    int fd = 0;
    uint64_t len = 0;

    file->start_position = sizeof(struct file_head);
    bin_offset = sizeof(struct file_head) + file->sum * sizeof(struct bin_head);

    buf = calloc(file->sum, sizeof(struct bin_head));
    if (buf == NULL) {
        printf("calloc error :: %d\n", errno);
        return -1;
    }
    offset = buf;

    for (i = 0; i < file->sum; i++) {
        bin[i]->start_position = bin_offset;

        bin_offset += bin[i]->bin_size;

        memcpy(offset, bin[i], sizeof(struct bin_head));
        offset += sizeof(struct bin_head);
    }

    file->file_size = bin_offset - sizeof(struct file_head);

    buf = realloc(buf, file->file_size);
    if (buf == NULL) {
        printf("realloc error :: %d\n", errno);
        return -1;
    }


    offset = buf + sizeof(struct bin_head) * file->sum;

    for (i = 0; i < file->sum; i++) {

        fd = open(bin[i]->file_name, O_RDONLY);
        if (fd < 0) {
            printf("open file name %s error %d\n", bin[i]->file_name, errno);
            return -2;
        }
        lseek(fd, 0, SEEK_SET);

        len = read(fd, offset, bin[i]->bin_size);
        if (len != bin[i]->bin_size) {
            printf("file not equal\n");
            return -3;
        }

        close(fd);

        offset += len;
    }

    unlink(upgrade_file);

    fd = open(upgrade_file, O_RDWR|O_CREAT, 0666);
    if (fd < 0) {
        printf("open file name %s error %d\n", bin[i]->file_name, errno);
        return -4;
    }

    len = write(fd, file, sizeof(struct file_head));
    if (len != sizeof(struct file_head)) {
        printf("file not equal\n");
        return -5;
    }

    len = write(fd, buf, file->file_size);
    if (len != file->file_size) {
        printf("file not equal\n");
        return -6;
    }

    close(fd);

    free(buf);

    return 0;
}

void free_alloc_buf(struct file_head *file, struct bin_head **bin) {
    int i = 0;

    if (file != NULL) {
        for (i = 0; i < file->sum; i++) {
            free(bin[i]);
        }

        free(file);
    }
}


int main(int argc, char **argv)
{
    int i = 0;
    int opt = 0;
    char *upgrade_config = NULL;
    char *line = NULL;

    FILE * fp;
    ssize_t read = 0;
    size_t len = 0;

    int ret = 0;
    struct bin_head *temp = NULL;

    while ((opt = getopt(argc, argv,"c:x:o:")) != -1) {
        switch (opt) {
        case 'c': {
            upgrade_config = malloc(strlen(optarg) + 1);
            if (!upgrade_config) {
                printf("malloc error\n");
                return -1;
            }
            memset(upgrade_config, 0, strlen(optarg) + 1);
            strncpy(upgrade_config, optarg, strlen(optarg));
            break;
        }
        case 'o': {
            upgrade_file = malloc(strlen(optarg) + 1);
            if (!upgrade_file) {
                printf("malloc error\n");
                return -1;
            }
            memset(upgrade_file, 0, strlen(optarg) + 1);
            strncpy(upgrade_file, optarg, strlen(optarg));
            break;
        }
        default:
            break;
        }
    }

    if (upgrade_config == NULL) {
        upgrade_config = malloc(strlen(ROKID_UPGRADE_CONFIGURE) + 1);
        if (!upgrade_config) {
            printf("malloc error\n");
            return -1;
        }
        //memset(upgrade_config, 0, strlen(optarg) + 1);
        memset(upgrade_config, 0, strlen(ROKID_UPGRADE_CONFIGURE) + 1);
        strncpy(upgrade_config, ROKID_UPGRADE_CONFIGURE, strlen(ROKID_UPGRADE_CONFIGURE));
    }

    if (upgrade_file == NULL) {
        upgrade_file = malloc(strlen(ROKID_DEFAULT_IMG) + 1);
        if (!upgrade_file) {
            printf("malloc error\n");
            return -1;
        }
        //memset(upgrade_file, 0, strlen(optarg) + 1);
        memset(upgrade_file, 0,strlen(ROKID_DEFAULT_IMG) + 1);
        strncpy(upgrade_file, ROKID_DEFAULT_IMG, strlen(ROKID_DEFAULT_IMG));
    }
    
    fp = fopen(upgrade_config, "r");
    if (fp == NULL) {
        printf("open error %d %s \n", errno, strerror(errno));
        goto free_param;
    }

    while ((read = getline(&line, &len, fp)) != -1) {

        // head must file_name
       ret = judge_line_islegal(line);
       if (ret < 0) {
           continue;
       }

       temp = calloc(1, sizeof(struct bin_head));
       if (!temp) {
           printf("malloc bin head error :: %d\n", errno);
           goto free_configure;
       }

       ret = parse_line_to_bin(line, temp);
       if (ret < 0) {
       }

       ret = get_file_size(temp);
       if (ret < 0) {

       }

       ret = calc_file_sha1sum(temp);
       if (ret < 0) {

       }

       temp->id = i;

       bin_head[i] = calloc(1, sizeof(struct bin_head));
       if (bin_head[i] == NULL) {
           goto free_configure;
       }

       memcpy(bin_head[i], temp, sizeof(struct bin_head));

       free(temp);
       i++;
    }

    file_head = calloc(1, sizeof(struct file_head));
    if (file_head == NULL) {
        goto free_file;
    }

    file_head->sum = i;


    ret = make_upgrade_bin(file_head, bin_head);
    if (ret < 0) {
        goto free_file;
    }

free_file:
    free_alloc_buf(file_head, bin_head);
free_configure:
    fclose(fp);
free_param:
    free(upgrade_config);

    return 0;
}

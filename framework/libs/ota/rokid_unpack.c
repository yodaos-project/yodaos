#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "boot.hpp"
#include "sha1.h"
#include "rokid_package.h"

struct bin_head *bin_head[36];
struct file_head *file_head = NULL;

static int keep_data = 0;

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

    printf("num :%d  position : 0x%llx size 0x%llx\n", head->sum, head->start_position, head->file_size);

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

        printf("name :: %s bin_size :: %lld flash_start :: 0x%llx flash size :: 0x%llx sub type :: %s \n", bin[i]->file_name, bin[i]->bin_size, bin[i]->flash_start_position, bin[i]->flash_part_size, bin[i]->sub_type);
    }

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

void brush_to_flash(char *flash_cmd) {
    Boot boot;

    flash_cmd[strlen(flash_cmd) - 1] = '\0';
    boot.SetTransferMode(USBSLAVE);

    boot.BootCommand(NULL, "leo", flash_cmd);
}

int get_cmd_flash_line(struct bin_head *bin, char *temppath, char *cmdline) {
    int ret = 0;

    sprintf(cmdline, "flash erase 0x%0x 0x%0x; download 0x%0x ./.tmp.%s; ", bin->flash_start_position, bin->flash_part_size, bin->flash_start_position, bin->file_name);
    return strlen(cmdline);
}

void unlink_temp_file(struct file_head *file, struct bin_head **bin) {
    int ret = 0;
    char file_path[128] = {0};
    int i = 0;

    for (i = 0; i < file->sum; i++) {
        sprintf(file_path, "./.tmp.%s", bin[i]->file_name);
        unlink(file_path);
    }
}

int parse_update_bin(int fd, struct file_head *file, struct bin_head **bin) {
    int i = 0;
    int fd_dump = 0;
    char temppath[512] = {0};
    uint8_t *buf = NULL;
    char sha1sum[48] = {0};
    int len = 0;
    int cmdlen = 0;
    int ret = 0;
    char cmdline[1024] = {0};

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

        if (strncmp(sha1sum, bin[i]->bin_sha1sum, sizeof(sha1sum)) == 0) {
#if 1
            memset(temppath, 0, sizeof(temppath));

            sprintf(temppath, ".tmp.%s", bin[i]->file_name);
            fd_dump = open(temppath, O_RDWR|O_CREAT, 0666);
            if (fd_dump < 0) {
                return -1;
            }

            write(fd_dump, buf, len);

            close(fd_dump);
#endif
            len = get_cmd_flash_line(bin[i], temppath, cmdline + cmdlen);
            if (len <= 0) {
                return -1;
            }

            cmdlen += len;
        }


        free(buf);
    }

    brush_to_flash(cmdline);

    unlink_temp_file(file, bin);

    return 0;
}


void usage(){
    printf("how to usage \n");
    printf("x<upgrade.img>\tassign the upgrade.img\n");
    printf("k \t\tkeep the data part\n");
}

int main(int argc, char **argv)
{
    int fd = 0;
    int opt = 0;
    int ret = 0;
    char *file_name = NULL;
    int i = 0;

    if (argc == 1) {
        usage();
        return 0;
    }
    while ((opt = getopt(argc, argv,"x:p:k")) != -1) {
        switch (opt) {
        case 'x': {
            file_name = (char *)malloc(strlen(optarg) + 1);
            if (!file_name) {
                printf("malloc error\n");
                return -1;
            }
            memset(file_name, 0, strlen(optarg) + 1);
            strncpy(file_name, optarg, strlen(optarg));

            break;
            }
        case 'p': {

            break;
            }
        case 'k': {
            keep_data = 1;
            break;
        }
        default:
            break;
        }
    }

    if (file_name == NULL) {
        file_name = (char *)malloc(strlen(ROKID_DEFAULT_IMG) + 1);
        if (!file_name) {
            printf("malloc error\n");
            return -1;
        }

        strncpy(file_name, ROKID_DEFAULT_IMG, strlen(ROKID_DEFAULT_IMG));
    }

    fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        printf("open error %d %s \n", errno, strerror(errno));
        goto free_param;
    }

    file_head = (struct file_head*)calloc(1, sizeof(struct file_head));
    if (file_head == NULL) {
        return -1;
    }

    ret = get_file_head(fd, file_head);
    if (ret < 0) {
        goto free_file;
    }

    for (i = 0; i < file_head->sum; i++) {
        bin_head[i] = (struct bin_head*)calloc(1, sizeof(struct bin_head));
        if (bin_head[i] == NULL) {
            goto free_head;
        }
    }

    ret = get_bin_head(fd, file_head, bin_head);
    if (ret < 0) {
        goto free_head;
    }

    ret = parse_update_bin(fd, file_head, bin_head);
    if (ret < 0) {
        goto free_head;
    }

free_head:
    free_alloc_buf(file_head, bin_head);

free_file:
    close(fd);
free_param:
    free(file_name);
    return 0;
}

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BT_MAC_STR "androidboot.btmac="
#define BT_MAC_LEN 17

int bd_strtoba(uint8_t addr[6], const char *address)
{
    int i;
    int len = strlen(address);
    char *dest = (char *)addr;
    char *begin = (char *)address;
    char *end_ptr;

    if (!address || !addr || len != 17) {
        printf("faile to addr:%s, len:%d\n", address, len);
        return -1;
    }
    for (i = 5; i >= 0; i--) {
        dest[i] = (char)strtoul(begin, &end_ptr, 16);
        if (!end_ptr) break;
        if (*end_ptr == '\0') break;
        if (*end_ptr != ':') {
            printf("faile to addr:%s, len:%d, end_ptr: %c, %s\n", address, len, *end_ptr, end_ptr);
            return -1;
        }
        begin = end_ptr +1;
        end_ptr = NULL;
    }
    if (i != 0) {
        printf("faile to addr:%s, len:%d", address, len);
        return -1;
    }
    printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
                        dest[5], dest[4], dest[3], dest[2], dest[1], dest[0]);
    return 0;
}

int read_btmac(uint8_t addr[6])
{
    char cmdline[4096] = {0};
    char *ptr;
    int fd;

    fd = open("/proc/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd >= 0) {
        int n = read(fd, cmdline, sizeof(cmdline) - 1);
        if (n < 0) n = 0;

        /* get rid of trailing newline, it happens */
        if (n > 0 && cmdline[n-1] == '\n') n--;

        cmdline[n] = 0;
        close(fd);
    } else {
        cmdline[0] = 0;
    }

    ptr = cmdline;
    char *x = strstr(ptr, BT_MAC_STR);
    if (x == 0) {
         printf("no find bt mac in cmdline\n");
        return -1;
    }
    printf("%s\n", x);
    if (strlen(x) > (strlen(BT_MAC_STR) + BT_MAC_LEN)) {
        x += strlen(BT_MAC_STR);
        if (x[0] == ' ') {
            printf("no bt mac data in cmdline\n");
            return -1;
        }
        if ((x[2] != ':') || (x[5] != ':') || (x[8] != ':') | (x[11] != ':') || (x[14] != ':')) {
            printf("mac format error\n");
            return -1;
        } else {
            x[BT_MAC_LEN] = 0;
            return bd_strtoba(addr, x);
        }
    }
    printf("no bt mac data in cmdline\n");
    return -1;
}
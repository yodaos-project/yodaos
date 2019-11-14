/**
 * 实时打印rklua应用的日志
 *
 * rklua会读取ROKID_LUA_PORT环境变量，然后将日志输出到对应的端口
 *
 * Usage: rklogger [options] port
 *
 * 支持一个选项 -w ，rklogger永远等待port端口
 * 
 * 接收一个参数，就是端口号(默认15003)，然后监听这个端口，读取日志并打印
 *
 * @module rklogger
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/route.h>

#define LOCAL_IP "127.0.0.1"

int main(int argc, char* argv[])
{
    bool wait = false;
    int opt, port;
    while ((opt = getopt(argc, argv, "w")) != -1) {
        switch (opt) {
            case 'w':
                wait = true;
                break;
            default:
                break;
        }
    }
    if (argc > optind) {
        port = atoi(argv[optind]);
        if (port <= 0) {
            printf("unsupported port %d\n", port);
            return -1;
        }
    } else {
        port = 15003;
    }

    printf("日志监听端口: %d\n", port);

    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
    addr.sin_port = htons(port);
    
    char buffer[1024*10];
    memset(buffer, 0x00, sizeof(buffer));
    size_t len;
    int fd = -1, ret;

recreate:
    if (fd > 0) {
        close(fd);
    }
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1) {
        perror("socket");
        return -1;
    }
    
reconnect:
    ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret == -1 && errno != EISCONN) {
        if (wait) {
            usleep(100);
            goto reconnect;
        }
        perror("connect");
        return -1;
    }

    printf("\n\n");

    while (true) {
        len = recv(fd, buffer, sizeof(buffer) - 16, 0);
        if (len == -1) {
            if (wait) {
                goto reconnect;
            } else {
                perror("recv");
                return -1;
            }
        } else if (len > 0) {
            buffer[len] = '\0';
            printf("%s", buffer);
            fflush(stdout);
        } else if (len == 0) {
            printf("\nDisconnected.\n\n");
            if (wait) {
                goto recreate;
            } else {
                break;
            }
        }
    }

    close(fd);
    return 0;
}


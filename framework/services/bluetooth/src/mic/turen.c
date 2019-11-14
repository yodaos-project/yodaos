#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <hardware/bt_log.h>

#define TUREN_PORT 30017
static int socketfd = -1;

int turen_close(void)
{
    if(socketfd >= 0) {
        shutdown(socketfd, SHUT_RDWR);
        close(socketfd);
        socketfd = -1;
    }
    return 0;
}

int turen_open(void)
{
    char buffer[1024] = {0};
    struct sockaddr_in addr;
    int ret = 0;

    if (socketfd >= 0 ) return 0;
    memset(&addr,0x00,sizeof(struct sockaddr_in));
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd < 0) {
    	BT_LOGE("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        ret = socketfd;
        goto err_exit;
    }

    int flags = fcntl(socketfd, F_GETFL, 0);
    if (flags < 0) {
    	BT_LOGE("fcntl F_GETFL socket error: %s(errno: %d)\n", strerror(errno),errno);
        ret = flags;
        goto err_exit;
    }

    ret = fcntl(socketfd, F_SETFL, flags & ~O_NONBLOCK);
    if (ret < 0) {
    	BT_LOGE("fcntl F_SETFL socket error: %s(errno: %d)\n", strerror(errno),errno);
        goto err_exit;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(TUREN_PORT);
    ret = connect(socketfd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret) {
    	BT_LOGE("connect socket error: %s(errno: %d)\n", strerror(errno),errno);
        goto err_exit;
    }

    sprintf(buffer, "save");
    ret = send(socketfd, buffer, 1024 , 0);
    if(ret < 0) {
    	BT_LOGE("send socket error: %s(errno: %d)\n", strerror(errno),errno);
        goto err_exit;
    }

    return 0;

err_exit:

    if(socketfd >= 0) {
        close(socketfd);
        socketfd = -1;
    }

    return ret;
}

int turen_recv_data(char *buf, int size)
{
    int read_size = 0;
    int count = 0;

    if (socketfd < 0) return 0;

    while(count < size) {
        read_size = recv(socketfd, &buf[count], size - count, 0);
        if (read_size == 0) {
            BT_LOGE("recv socket count: %d, read_size %d, socket maybe disconnect!!", count, read_size);
            return 0;
        }
        if(read_size < 0) {
            BT_LOGE("recv socket count: %d, read_size %d, %s(errno: %d)", count, read_size, strerror(errno),errno);
            return -1;
        } else
            count += read_size;
    }
    return count;
}
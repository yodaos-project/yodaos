/**
 * 接收turen日志并上传到阿里云
 *
 * @module cloudlog
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <pthread.h>
#include <aliyunlog-lite/aloglite.h>
#include <cutils/properties.h>

//#define LOCAL_IP "127.0.0.1"
#define MAX_THREAD_NUM 20
#define SHARED_MEM_STS_TOKEN 7395
#define MAX_FILE_NAME_LEN 20
#define STS_TOKEN_SIZE 10 * 1024
#define STS_KEY_SIZE 256
#define HTTP_AUTH_SIZE 512
struct sts_token
{
    int flag; //0 no one access;1 someone is accessing,could not access
    char keyid[STS_KEY_SIZE];
    char keysec[STS_KEY_SIZE];
    char token[STS_TOKEN_SIZE];
    char exptime[32];
    char httpauth[HTTP_AUTH_SIZE];
};

typedef struct my_socket_info
{
    int port;
    rokid_cloudloglite_client *rkclient;
    const char *deviceId;
}socket_info_t;

static struct sts_token *p_sts_token = NULL;


static void *fun_log2cloud(void *socketInfo)
{
    socket_info_t socket_info = *((socket_info_t *)socketInfo);
    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(LOCAL_IP);
    addr.sin_port = htons(socket_info.port);

    char buffer[1024 * 10];
    memset(buffer, 0x00, sizeof(buffer));
    size_t len;
    int fd = -1, ret;
    long timeout_time = 0;

recreate:
    if (fd > 0)
    {
        close(fd);
    }

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == fd)
    {
        perror("socket");
        return NULL;
    }

reconnect:
    ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (-1 == ret && errno != EISCONN)
    {
        sleep(1); //防止无效socket号导致一直尝试连接占用cpu
        goto reconnect;
    }

    while (true)
    {
        len = recv(fd, buffer, sizeof(buffer) - 16, 0);
        if (-1 == len)
        {
            usleep(1000);
            goto reconnect;
        }
        else if (len > 0)
        {
            if (buffer[len - 1] == '\n')
            { //有换行符，去掉换成0
                buffer[len - 1] = '\0';
            }
            else
            {
                buffer[len] = '\0'; //无换行符，结尾加0
            }
            if (1 == len && buffer[0] == '\0')
            { //turen会发送大量换行符，丢弃只有一个换行符的无用log
                continue;
            }
            if (NULL == p_sts_token)
            {
                int shm_id_token;
                shm_id_token = shmget(SHARED_MEM_STS_TOKEN, 0, 0); //shared mem should be created by activation when set_cloudlog_on
                if (-1 == shm_id_token)
                {
                    printf("shmget error");
                    continue;
                }
                p_sts_token = (struct sts_token *)shmat(shm_id_token, 0, 0);
                printf("link shared mem and set flag =0\n");
            }

            time_t curr_time;
            time(&curr_time);
            if (timeout_time - curr_time < 60) //exp time <1min,try to update sts token
            {
                printf("reset sts token,current time:%ld,timeout:%ld\n", curr_time, timeout_time);
                if (0 == p_sts_token->flag)
                {
                    p_sts_token->flag = 1;
                    struct tm tminfo;
                    time_t timeinfo = 0;
                    if (strptime(p_sts_token->exptime, "%Y-%m-%dT%H:%M:%SZ", &tminfo) != NULL)
                    {
                        timeinfo = mktime(&tminfo) + 28800;
                        printf("time of exptime is %ld\n", timeinfo);
                    }
                    else
                    {
                        printf("reset sts get time error\n");
                        p_sts_token->flag = 0;
                        continue;
                    }
                    if (timeinfo != timeout_time) //someone had updated the token
                    {
                        printf("reset sts token:timeinfo:%ld\n", timeinfo);
                        rokid_reset_sts_token(socket_info.rkclient, p_sts_token->keyid, p_sts_token->keysec, p_sts_token->token);
                        timeout_time = timeinfo;
                    }
                    p_sts_token->flag = 0;
                }
            }

            rkloglite_cloud(socket_info.rkclient, 4, "_SN_", socket_info.deviceId, "_zcontent_", buffer);
            //printf("len:%d,%s",len, buffer);
        }
        else if (0 == len)
        {
            printf("\nDisconnected. try to recreate %d\n\n", socket_info.port);
            usleep(1000);
            goto recreate;
        }
    }
    close(fd);
    return 0;
}
int main(int argc, char *argv[])
{
    const char *name_str = "turenproc";
    char logstore[PROP_VALUE_MAX];
    char device_id[PROP_VALUE_MAX];
    property_get("ro.boot.serialno", device_id, "unknown");
    property_get("ro.rokid.device", logstore, "rokid_me");
    rokid_log_configs conf = {
        .project = "rokid-linux",
        .logstore = logstore,
        .name = name_str,
        .endpoint = "https://cn-hangzhou.log.aliyuncs.com",
        .log_bytes_per_package = 100 * 1024,
        .log_count_per_package = 4096,
        .package_timeout_ms = 3000,
        .max_buffer_bytes = 500 * 1024,
    };

    rokid_cloudloglite_client *cloud_client = create_client(&conf); //创建client
    if (NULL == cloud_client)
    {
        return -1;
    }
    //参数值为端口号，获取端口号，建立线程，从端口接收log，发送上云，eg：cloudlog 15006 15003 30100
    int i, port;
    pthread_t thr_accept[MAX_THREAD_NUM]; //max threads
    socket_info_t socketInfo[MAX_THREAD_NUM];
    for (i = 0; i < argc - 1 && i < MAX_THREAD_NUM; i++)
    {
        port = atoi(argv[i + 1]);
        printf("日志监听端口: %d\n", port);
        socketInfo[i].port = port;
        socketInfo[i].rkclient = cloud_client;
        socketInfo[i].deviceId = device_id;
        pthread_create(&thr_accept[i], NULL, fun_log2cloud, &socketInfo[i]);
    }

    for (i = 0; i < argc - 1 && i < MAX_THREAD_NUM; i++)
    {
        pthread_join(thr_accept[i], NULL);
    }

    destory_client(cloud_client);
    cloud_client = NULL;
    shmdt(p_sts_token);
    p_sts_token = NULL;
    return 0;
}
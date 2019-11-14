/**
 * 动态控制log打印级别
 *
 * Usage: set_log_level -[level]
 *
 * 支持5个级别 -v -d -i -w -e
 * 
 * 设置一个级别后，低于此级别的log将不再打印。
 *
 * @module set_log_level
 */

#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHARED_MEM_RKLOG 8975//this macro should be the same as in RKLog.cpp

void printusage()
{
    printf("Usage: set_log_level -[level]\n");
    printf("support 5 level: v:verbos\n");
    printf("                 d:debug\n");
    printf("                 i:info\n");
    printf("                 w:warn\n");
    printf("                 e:error\n");
    printf("eg: set_log_level -i   then log level below i(v and d) will not print\n");
}

void set_log_level(int loglevel)
{
    int shm_id;
    shm_id = shmget(SHARED_MEM_RKLOG,sizeof(unsigned int),IPC_CREAT | 0666);
    if(shm_id == -1){
        printf("shmget error");
    }
    unsigned int *share = (unsigned int *)shmat(shm_id,0,0);
    *share = loglevel;
    shmdt(share);
    share = NULL;
}

int main(int argc, char* argv[])
{
    int opt;
    int loglevel = 0;
    if(argc == 1 || argc >=3){
        printusage();
        return -1;
    }
    while ((opt = getopt(argc, argv, "vdiwe")) != -1) {
        switch (opt) {
            case 'v':
                loglevel = 1;
                break;
            case 'd':
                loglevel = 2;
                break;
            case 'i':
                loglevel = 3;
                break;
            case 'w':
                loglevel = 4;
                break;
            case 'e':
                loglevel = 5;
                break;
            default:
                printf("unknown log level\n");
                printusage();
                return -1;
        }
    }
    set_log_level(loglevel);
}

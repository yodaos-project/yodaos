#ifndef OS_H
#define OS_H

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define closesocket(fd) close(fd)
#endif

typedef struct {
	long threadid;
	void *handle;
	void *args;
	int (*fn)(void *);
}Thread;

Thread *_CreateThread(int(*fn)(void*), void *args);
void _WaitThreadJoin(Thread *thread);
void save_local_ip(const char *ipaddr);
char *get_local_ip(char *ipaddr);
void save_stb_ip(const char *ipaddr);
char *get_stb_ip(char *ipaddr);
int OSInit(void);

unsigned long long bb_strtoull(const char *arg, char **endp, int base);
unsigned int bb_strtou(const char *arg, char **endp, int base);
char* xmalloc_readlink(const char *path);
size_t open_read_close(const char *filename, void *buf, size_t size);

#endif


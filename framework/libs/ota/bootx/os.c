#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>

#include "os.h"

static unsigned long long ret_ERANGE(void)
{
	errno = ERANGE; /* this ain't as small as it looks (on glibc) */
	return ULLONG_MAX;
}

static unsigned long long handle_errors(unsigned long long v, char **endp)
{
	char next_ch = **endp;

	/* errno is already set to ERANGE by strtoXXX if value overflowed */
	if (next_ch) {
		/* "1234abcg" or out-of-range? */
		if (isalnum(next_ch) || errno)
			return ret_ERANGE();
		/* good number, just suspicious terminator */
		errno = EINVAL;
	}
	return v;
}

#if defined(__APPLE__)
unsigned long long bb_strtoull(const char *arg, char **endp, int base)
{
	unsigned long long v;
	char *endptr;

	if (!endp) endp = &endptr;
	*endp = (char*) arg;

	/* strtoul("  -4200000000") returns 94967296, errno 0 (!) */
	/* I don't think that this is right. Preventing this... */
	if (!isalnum(arg[0])) return ret_ERANGE();

	/* not 100% correct for lib func, but convenient for the caller */
	errno = 0;
	v = strtoull(arg, endp, base);
	return handle_errors(v, endp);
}
#endif

unsigned int bb_strtou(const char *arg, char **endp, int base)
{
	unsigned long v;
	char *endptr;

	if (!endp) endp = &endptr;
	*endp = (char*) arg;

	if (!isalnum(arg[0])) return ret_ERANGE();
	errno = 0;
	v = strtoul(arg, endp, base);
	if (v > UINT_MAX) return ret_ERANGE();
	return handle_errors(v, endp);
}

size_t safe_read(FILE *fp, void *buf, size_t count)
{
	size_t n;

	do {
		n = fread(buf, 1, count, fp);
	} while ((n == (size_t)NULL) && (ferror(fp) != 0));
	return n;
}

size_t full_read(FILE *fp, void *buf, size_t len)
{
	size_t cc;
	size_t total;

	total = 0;

	while (len) {
		cc = safe_read(fp, buf, len);

		if (cc == (size_t)NULL) {
			if (total) {
				/* we already have some! */
				/* user can do another read to know the error code */
				return total;
			}
			return cc; /* read() returns -1 on failure. */
		}
		buf = ((char *)buf) + cc;
		total += cc;
		len -= cc;
	}

	return total;
}

size_t open_read_close(const char *filename, void *buf, size_t size)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
		return (int)NULL;

	size = full_read(fp, buf, size);
	/*e = errno;*/
	fclose(fp);
	/*errno = e;*/
	return size;
}

static char local_ip[32]="";
void save_local_ip(const char *ipaddr)
{
	strcpy(local_ip, ipaddr);
}

static char stb_ip[32]="";
void save_stb_ip(const char *ipaddr)
{
	strcpy(stb_ip, ipaddr);
}

char *get_stb_ip(char *ipaddr)
{
	if(strcmp(stb_ip, "") == 0){
		return NULL;
	}else{
		strcpy(ipaddr, stb_ip);
		return ipaddr;
	}
}

#ifdef WIN32
#include <windows.h>

int OSInit(void)
{
	WORD verPref = MAKEWORD(2,2);
	WSADATA wsaData;

	if (WSAStartup(verPref, &wsaData) != 0) {
		printf("Winsock 2.2 initialization failed");
		return -1;
	}
	return 0;
}

static DWORD WINAPI RunThreadViaCreateThread(LPVOID data)
{
	Thread *thread = (Thread*)data;

	if (data != NULL && thread->fn != NULL)
		return (DWORD)thread->fn(thread->args);

	return 0;
}

Thread *_CreateThread(int(*fn)(void*), void *args)
{
	Thread *thread = (Thread*)malloc(sizeof(Thread));
	thread->fn = fn;
	thread->args = args;

	thread->threadid = 0;
	thread->handle = (void *)CreateThread(NULL, 0, RunThreadViaCreateThread, thread, 0, (DWORD*)&thread->threadid);

	return thread;
}

void _WaitThreadJoin(Thread *thread)
{
	WaitForSingleObject((HANDLE)thread->handle, 0);
}

char *get_local_ip(char *ipaddr)
{
	strcpy(ipaddr, local_ip);
	return ipaddr;
}
#endif


#if defined(linux) || defined(__APPLE__)

#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/stat.h>

int OSInit(void)
{
	return 0;
}

static void *RunThreadViaCreateThread(void *data)
{
	Thread *thread = (Thread*)data;

	if (data != NULL && thread->fn != NULL)
		thread->fn(thread->args);

	pthread_exit(EXIT_SUCCESS);
}

Thread *_CreateThread(int(*fn)(void*), void *args)
{
	Thread *thread = (Thread*)malloc(sizeof(Thread));
	thread->fn = fn;
	thread->args = args;

	thread->threadid = 0;
	pthread_create((pthread_t*)&thread->handle, NULL, RunThreadViaCreateThread, thread);

	return thread;
}

void _WaitThreadJoin(Thread *thread)
{
	pthread_join((pthread_t)thread->handle, NULL);
}

char *get_local_ip(char *ipaddr)
{
	int i;
	char eth_name[10];
	int sock_get_ip;
	struct sockaddr_in *sin;
	struct ifreq ifr_ip;

	if(strcmp(local_ip, "") == 0){
		if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("socket create false...GetLocalIp!/n");
			return (char*)"";
		}

#define MAX_ETH_NUM	5
		for (i = 0; i < MAX_ETH_NUM; i++) {
			memset(&ifr_ip, 0, sizeof(ifr_ip));
			memset(eth_name, 0, sizeof(eth_name));
			sprintf(eth_name, "eth%d", i);
			strncpy(ifr_ip.ifr_name, eth_name, sizeof(ifr_ip.ifr_name) - 1);

			if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 ) {
				if(i == (MAX_ETH_NUM - 1)){
					closesocket(sock_get_ip);
					return (char*)"";
				}
			} else
				break;
		}

		sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;
		strcpy(ipaddr, inet_ntoa(sin->sin_addr));

		closesocket(sock_get_ip);
	}else{
		strcpy(ipaddr, local_ip);
	}

	return ipaddr;
}

char* xmalloc_readlink(const char *path)
{
	enum { GROWBY = 80 }; /* how large we will grow strings by */

	char *buf = NULL;
	int bufsize = 0, readsize = 0;

	do {
		bufsize += GROWBY;
		buf = (char *)realloc(buf, bufsize);
		readsize = readlink(path, buf, bufsize);
		if (readsize == -1) {
			free(buf);
			return NULL;
		}
	} while (bufsize < readsize + 1);

	buf[readsize] = '\0';

	return buf;
}

#endif


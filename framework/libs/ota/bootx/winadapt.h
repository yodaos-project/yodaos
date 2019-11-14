#ifdef WIN32

#ifndef __MINGW
typedef signed int ssize_t;
#endif

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;

#define strcasecmp	stricmp
#define usleep(_us) Sleep((_us)/1000)

#ifndef PATH_MAX
#define PATH_MAX 512
#endif

#endif

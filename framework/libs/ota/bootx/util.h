#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>

#define return_val_if_fail(a,b) if (a) return b

#ifndef MIN
#	define MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef S_ISLNK
#	define S_IFLNK    0120000 /* Symbolic link */
#	define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)
#endif

#ifndef __USE_XOPEN2K8
char *strndup(const char *s, size_t n);
#endif

int scandir(const char *dirname,
		struct dirent ***ret_namelist,
		int (*select)(const struct dirent *),
		int (*compar)(const struct dirent **, const struct dirent **));

char **strsplit(const char *string, const char *delimiter, int max_tokens);
void strfreev(char **str_array);
int strv_length(char **str_array);
char *strconcat(const char *first, ...);
char *trim(char *str);

char *get_extension(const char *path);
char *catpath(const char *dir, const char *file);

//int mkstemp(char *temp);
void *x_fmmap(const char *fname, size_t *size);
int x_munmap(void *addr, size_t length);

int lstat(const char *file_name, struct stat *buf);
uint64_t str2int(char *str);

#endif

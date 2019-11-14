#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <ctype.h>

#ifdef WIN32
//#include <wtsapi32.h>
#include <windef.h>
#include <windows.h>
#include <conio.h>
#include <io.h>
#else
#include <sys/mman.h>
#define HAVE_SCANDIR
#define HAVE_MKSTEMP
#endif

#include "util.h"

#ifndef WIN32
#define O_BINARY 0
#endif

#ifdef WIN32
/*
 * This fixes linking with precompiled libusb-1.0.18-win and
 * libusb-1.0.19-rc1-win: "undefined reference to __ms_vsnprintf". See:
 * http://sourceforge.net/p/mingw-w64/mailman/mingw-w64-public/thread/4F8CA26A.70103@users.sourceforge.net/
 */

#include <stdio.h>
#include <stdarg.h>
#ifdef __cplusplus
extern "C"
{
#endif

int __cdecl __ms_vsnprintf(char * __restrict__ d,size_t n,const char * __restrict__ format,va_list arg)
{
    return vsnprintf(d, n, format, arg);
}

#ifdef __cplusplus
}
#endif
#endif

char *trim(char *str)
{
	char *cp = str;

	while (*cp != 0 && isspace(*cp))
		++cp;

	if (cp == str)
		cp += strlen(str) - 1;
	else {
		char *cpdes;
		for (cpdes = str; *cp != 0; )
			*cpdes++ = *cp++;
		cp = cpdes;
		*cp-- = 0;
	}

	while (cp > str && isspace(*cp))
		*cp-- = 0;

	return str;
}

static unsigned int _htoi(const char *str)
{
	const char *cp;
	unsigned int data, bdata;

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			bdata = *cp - '0';
		else if (*cp >= 'A' && *cp <= 'F')
			bdata = *cp - 'A' + 10;
		else if (*cp >= 'a' && *cp <= 'f')
			bdata = *cp - 'a' + 10;
		else
			break;
		data = (data << 4) | bdata;
	}

	return data;
}

uint64_t str2int(char *str)
{
	const char *cp;
	uint64_t data;

	if (str == NULL)
		return 0;

	if (str[0] == '0' && (str[1] == 'X' || str[1] == 'x'))
		return _htoi(str + 2);

	for (cp = str, data = 0; *cp != 0; ++cp) {
		if (*cp >= '0' && *cp <= '9')
			data = data * 10 + *cp - '0';
		else if (*cp == 'K' || (*cp == 'k'))
			data = data * 1024;
		else if (*cp == 'M' || (*cp == 'm'))
			data = data * 1024 * 1024;
		else if (*cp == 'G' || (*cp == 'g'))
			data = data * 1024 * 1024 * 1024;
		else if (*cp == 'T' || (*cp == 't'))
			data = data * 1024 * 1024 * 1024 * 1024;
		else
			break;
	}

	return data;
}

#ifndef HAVE_MKSTEMP
/*  cheap and nasty mkstemp replacement */
int mkstemp(char *temp)
{
	mktemp(temp);
	return open(temp, O_RDWR | O_CREAT | O_EXCL | O_BINARY, 0600);
}
#endif

#ifndef __USE_XOPEN2K8
char *strndup (__const char *s, size_t n)
{
	  char *clone;

	  clone = (char *)malloc (n + 1);
	  strncpy (clone, s, n);
	  clone[n] = '\0';
	  return clone;
}
#endif

static void add_to_vector(char ***vector, int size, char *token)
{
	*vector = *vector == NULL ?
		(char **)malloc(2 * sizeof(*vector)) :
		(char **)realloc(*vector, (size + 1) * sizeof(*vector));

	(*vector)[size - 1] = token;
}

char **strsplit(const char *string, const char *delimiter, int max_tokens)
{
	const char *c;
	char *token, **vector;
	int size = 1;

	return_val_if_fail (string != NULL, NULL);
	return_val_if_fail (delimiter != NULL, NULL);
	return_val_if_fail (delimiter[0] != 0, NULL);

	if (strncmp (string, delimiter, strlen (delimiter)) == 0) {
		vector = (char **)malloc (2 * sizeof(vector));
		vector[0] = strdup ("");
		size++;
		string += strlen (delimiter);
	} else {
		vector = NULL;
	}

	while (*string && !(max_tokens > 0 && size >= max_tokens)) {
		c = string;
		if (strncmp (string, delimiter, strlen (delimiter)) == 0) {
			token = strdup ("");
			string += strlen (delimiter);
		} else {
			while (*string && strncmp (string, delimiter, strlen (delimiter)) != 0) {
				string++;
			}

			if (*string) {
				int toklen = (string - c);
				token = strndup(c, toklen);

				/* Need to leave a trailing empty
				 * token if the delimiter is the last
				 * part of the string
				 */
				if (strcmp (string, delimiter) != 0) {
					string += strlen (delimiter);
				}
			} else {
				token = strdup (c);
			}
		}

		add_to_vector (&vector, size, token);
		size++;
	}

	if (*string) {
		/* Add the rest of the string as the last element */
		add_to_vector (&vector, size, strdup (string));
		size++;
	}

	if (vector == NULL) {
		vector = (char **) malloc (2 * sizeof (vector));
		vector [0] = NULL;
	} else if (size > 0) {
		vector[size - 1] = NULL;
	}

	return vector;
}

void strfreev(char **str_array)
{
	char **orig = str_array;
	if (str_array == NULL)
		return;
	while (*str_array != NULL){
		free (*str_array);
		str_array++;
	}
	free (orig);
}

int strv_length(char **str_array)
{
	int length = 0;
	if (str_array == NULL)
		return 0;
	for(length = 0; str_array[length] != NULL; length++);
	return length;
}

char *strconcat(const char *first, ...)
{
	va_list args;
	size_t total = 0;
	char *s, *ret;
	return_val_if_fail (first != NULL, NULL);

	total += strlen (first);
	va_start (args, first);
	for (s = va_arg (args, char *); s != NULL; s = va_arg(args, char *)){
		total += strlen (s);
	}
	va_end (args);

	ret = (char*)malloc (total + 1);
	if (ret == NULL)
		return NULL;

	ret [total] = 0;
	strcpy (ret, first);
	va_start (args, first);
	for (s = va_arg (args, char *); s != NULL; s = va_arg(args, char *)){
		strcat (ret, s);
	}
	va_end (args);

	return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef HAVE_SCANDIR

#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/*
 * convenience helper function for scandir's |compar()| function:
 * sort directory entries using strcoll(3)
 */
int alphasort(const void *_a, const void *_b)
{
	struct dirent **a = (struct dirent **)_a;
	struct dirent **b = (struct dirent **)_b;
	return strcoll((*a)->d_name, (*b)->d_name);
}


#define strverscmp(a,b) strcoll(a,b) /* for now */

/*
 * convenience helper function for scandir's |compar()| function:
 * sort directory entries using GNU |strverscmp()|
 */
int versionsort(const void *_a, const void *_b)
{
	struct dirent **a = (struct dirent **)_a;
	struct dirent **b = (struct dirent **)_b;
	return strverscmp((*a)->d_name, (*b)->d_name);
}

/*
 * The scandir() function reads the directory dirname and builds an
 * array of pointers to directory entries using malloc(3).  It returns
 * the number of entries in the array.  A pointer to the array of
 * directory entries is stored in the location referenced by namelist.
 *
 * The select parameter is a pointer to a user supplied subroutine
 * which is called by scandir() to select which entries are to be
 * included in the array.  The select routine is passed a pointer to
 * a directory entry and should return a non-zero value if the
 * directory entry is to be included in the array.  If select is null,
 * then all the directory entries will be included.
 *
 * The compar parameter is a pointer to a user supplied subroutine
 * which is passed to qsort(3) to sort the completed array.  If this
 * pointer is null, the array is not sorted.
 */
int scandir(const char *dirname,
		struct dirent ***ret_namelist,
		int (*select)(const struct dirent *),
		int (*compar)(const struct dirent **, const struct dirent **))
{
	int i, len;
	int used, allocated;
	DIR *dir;
	struct dirent *ent, *ent2;
	struct dirent **namelist = NULL;

	if ((dir = opendir(dirname)) == NULL)
		return -1;

	used = 0;
	allocated = 2;
	namelist = (struct dirent **)malloc(allocated * sizeof(struct dirent *));
	if (!namelist)
		goto error;

	while ((ent = readdir(dir)) != NULL) {

		if (select != NULL && !select(ent))
			continue;

		/* duplicate struct direct for this entry */
		len = offsetof(struct dirent, d_name) + strlen(ent->d_name) + 1;
		if ((ent2 = (struct dirent *)malloc(len)) == NULL)
			return -1;

		if (used >= allocated) {
			allocated *= 2;
			namelist = (struct dirent **)realloc(namelist, allocated * sizeof(struct dirent *));
			if (!namelist)
				goto error;
		}
		memcpy(ent2, ent, len);
		namelist[used++] = ent2;
	}
	closedir(dir);

	if (compar)
		qsort(namelist, used, sizeof(struct dirent *),
				(int (*)(const void *, const void *)) compar);

	*ret_namelist = namelist;
	return used;


error:
	if (namelist) {
		for (i = 0; i < used; i++)
			free(namelist[i]);
		free(namelist);
	}
	return -1;
}
#endif

#ifdef WIN32
/*
 * The unit of FILETIME is 100-nanoseconds since January 1, 1601, UTC.
 * Returns the 100-nanoseconds ("hekto nanoseconds") since the epoch.
 */
static inline long long filetime_to_hnsec(const FILETIME *ft)
{
	long long winTime = ((long long)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
	/* Windows to Unix Epoch conversion */
	return winTime - 116444736000000000LL;
}

static inline time_t filetime_to_time_t(const FILETIME *ft)
{
	return (time_t)(filetime_to_hnsec(ft) / 10000000);
}

static inline int file_attr_to_st_mode (DWORD attr)
{
	int fMode = S_IREAD;
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		fMode |= S_IFDIR;
	else
		fMode |= S_IFREG;
	if (!(attr & FILE_ATTRIBUTE_READONLY))
		fMode |= S_IWRITE;
	return fMode;
}

static inline int get_file_attr(const char *fname, WIN32_FILE_ATTRIBUTE_DATA *fdata)
{
	if (GetFileAttributesExA(fname, GetFileExInfoStandard, fdata))
		return 0;

	switch (GetLastError()) {
		case ERROR_ACCESS_DENIED:
		case ERROR_SHARING_VIOLATION:
		case ERROR_LOCK_VIOLATION:
		case ERROR_SHARING_BUFFER_EXCEEDED:
			return EACCES;
		case ERROR_BUFFER_OVERFLOW:
			return ENAMETOOLONG;
		case ERROR_NOT_ENOUGH_MEMORY:
			return ENOMEM;
		default:
			return ENOENT;
	}
}

int lstat(const char *file_name, struct stat *buf)
{
	WIN32_FILE_ATTRIBUTE_DATA fdata;

	if (!(errno = get_file_attr(file_name, &fdata))) {
		buf->st_ino = 0;
		buf->st_gid = 0;
		buf->st_uid = 0;
		buf->st_nlink = 1;
		buf->st_mode = file_attr_to_st_mode(fdata.dwFileAttributes);
		buf->st_size = fdata.nFileSizeLow;
		buf->st_dev = buf->st_rdev = 0; /* not used by Git */
		buf->st_atime = filetime_to_time_t(&(fdata.ftLastAccessTime));
		buf->st_mtime = filetime_to_time_t(&(fdata.ftLastWriteTime));
		buf->st_ctime = filetime_to_time_t(&(fdata.ftCreationTime));
		return 0;
	}

	return -1;
}

#endif

#if 0
/* return the base name of a file - caller frees */
char *basename(const char *s)
{
	char *p;
	p = strrchr(s, '/');
	if (p) s = p + 1;
#ifdef _WIN32
	p = strrchr(s, '\\');
	if (p) s = p + 1;
#endif

	return strdup(s);
}
#endif

/*
 * Return the file extension (including the dot) of a path as a pointer into
 * path. If path has no file extension, the empty string and the end of path is
 * returned.
 */
char *get_extension(const char *path)
{
	size_t len = strlen(path);
	const char *p;

	for (p = &path[len - 1]; p >= path; --p) {
		if (*p == '.') {
			return (char *)p;
		}
		if (*p == '/') {
			break;
		}
	}
	return (char *)(&path[len]);
}

/*
 * Map file into memory. Return a pointer to the mapped area if successful or
 * -1 if any error occurred. The file size is also returned,
 */
void *x_fmmap(const char *fname, size_t *size)
{
	struct stat st;
	void *data = NULL;
	int fd = -1;
#ifdef WIN32
	HANDLE section;
	HANDLE file;
	file = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0, NULL);
	if (file == INVALID_HANDLE_VALUE) {
		printf("%s: %s\n", strerror(errno), fname);
		goto error;
	}
	fd = _open_osfhandle((intptr_t) file, O_RDONLY | O_BINARY);
	if (fd == -1) {
		printf("%s: %s\n", strerror(errno), fname);
		CloseHandle(file);
		goto error;
	}
	if (fstat(fd, &st) == -1) {
		printf("%s: %s\n", strerror(errno), fname);
		CloseHandle(file);
		goto error;
	}
	section = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	CloseHandle(file);
	if (!section) {
		printf("Failed to mmap %s %s\n", strerror(errno), fname);
		goto error;
	}
	data = MapViewOfFile(section, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(section);
#else
	fd = open(fname, O_RDONLY);
	if (fd == -1) {
		printf("%s: %s\n", strerror(errno), fname);
		goto error;
	}
	if (fstat(fd, &st) == -1) {
		printf("%s: %s\n", strerror(errno), fname);
		close(fd);
		goto error;
	}

	if (st.st_size > 0) {
		data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (data == (void *) -1)
			printf("Failed to mmap %s %s\n", strerror(errno), fname);
	}

#endif
	if (size)
		*size = st.st_size;
error:
	return data;
}

/*
 * Unmap file from memory.
 */
int x_munmap(void *addr, size_t length)
{
#ifdef WIN32
	return UnmapViewOfFile(addr) ? 0 : -1;
#else
	return munmap(addr, length);
#endif
}

char* catpath(const char *dir, const char *file)
{
	int dir_len = strlen(dir);
	char *path;

	if (dir == NULL || file == NULL)
		return NULL;

	path = (char *)malloc(dir_len + strlen(file) + 2);

	if (!path)
		return NULL;

	strcpy(path, dir);

	if ((!path[0] || path[dir_len - 1] != '/') && file[0] != '/')
		strcat ((char *) path, "/");

	strcat(path, file);

	return path;
}

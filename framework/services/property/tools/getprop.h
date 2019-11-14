
#ifndef __GETPROP_H__
#define __GETPROP_H__
#include <sys/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static void error_exit (char *msg)
{
    if (msg)
        printf("%s failed\n", msg);
    exit(-1);
}
// The qsort man page says you can use alphasort, the posix committee
// disagreed, and doubled down: http://austingroupbugs.net/view.php?id=142
// So just do our own. (The const is entirely to humor the stupid compiler.)
int qstrcmp(const void *a, const void *b)
{
    return strcmp(*(char **)a, *(char **)b);
}

// Die unless we can change the size of an existing allocation, possibly
// moving it.  (Notice different arguments from libc function.)
void *xrealloc(void *ptr, size_t size)
{
    ptr = realloc(ptr, size);
    if (!ptr) error_exit("xrealloc");

    return ptr;
}

size_t  strnlen(const char*  str, size_t  maxlen)
{
    char*  p = memchr(str, 0, maxlen);

    if (p == NULL)
        return maxlen;
    else
        return (p - str);
}

#ifndef __x86_64__
char *strndup(const char *str, size_t maxlen)
{
    char *copy;
    size_t len;

    len = strnlen(str, maxlen);
    copy = malloc(len + 1);
    if (copy != NULL) {
        (void)memcpy(copy, str, len);
        copy[len] = '\0';
    }

    return copy;
}
#endif


// Die unless we can allocate a copy of this many bytes of string.
char *xstrndup(char *s, size_t n)
{
    char *ret = strndup(s, ++n);

    if (!ret) error_exit("xstrndup");
    ret[--n] = 0;

    return ret;
}

// Die unless we can allocate a copy of this string.
char *xstrdup(char *s)
{
    return xstrndup(s, strlen(s));
}

#endif

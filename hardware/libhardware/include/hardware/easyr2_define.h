/*
 * easyr2_define.h
 *
 *  Created on: 2014-8-31
 *      Author: root
 */

#ifndef EASYR2_DEFINE_H_
#define EASYR2_DEFINE_H_

#ifdef __cplusplus
# define EASYR2_CPP_START extern "C" {
# define EASYR2_CPP_END }
#else
# define EASYR2_CPP_START
# define EASYR2_CPP_END
#endif


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <inttypes.h>
#include <unistd.h>

EASYR2_CPP_START

#define easyr2_malloc(size)           malloc(size)
#define easyr2_realloc(ptr, size)     realloc(ptr, size)
#define easyr2_free(ptr)              if(ptr) free(ptr)

#define easyr2_memcpy(dst, src, n)    (((char *) memcpy(dst, src, (n))) + (n))
#define easyr2_const_strcpy(b, s)     easy_memcpy(b, s, sizeof(s)-1)

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define easyr2_align_ptr(p, a)        (uint8_t*)(((uintptr_t)(p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))
#define easyr2_align(d, a)            (((d) + (a - 1)) & ~(a - 1))
#define easyr2_max(a,b)               (a > b ? a : b)
#define easyr2_min(a,b)               (a < b ? a : b)
#define easyr2_div(a,b)               ((b) ? ((a)/(b)) : 0)

#define EASYR2_OK                     0
#define EASYR2_ERROR                  (-1)
#define EASYR2_ABORT                  (-2)
#define EASYR2_ASYNC                  (-3)
#define EASYR2_BREAK                  (-4)
#define EASYR2_AGAIN                  (-EAGAIN)

EASYR2_CPP_END

#endif /* EASYR2_DEFINE_H_ */

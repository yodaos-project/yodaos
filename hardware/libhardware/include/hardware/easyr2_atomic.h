#if defined(__arm__) || defined(__TARGET_ARCH_ARM)
#include "easyr2_atomic32.h"
#elif defined(__aarch64__)
#include "easyr2_atomic64.h"
#else
#error: please define __aarch64__ or __arm__ in your makefile
#endif

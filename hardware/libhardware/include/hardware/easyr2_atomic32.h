/*
 * easyr2_atomic.h
 *
 *  Created on: 2014-8-31
 *      Author: root
 */

#ifndef EASYR2_ATOMIC_H_
#define EASYR2_ATOMIC_H_

#include <stdint.h>
#include "easyr2_define.h"

EASYR2_CPP_START

#define easyr2_atomic_set(v,i)        ((v) = (i))
typedef volatile int32_t            easyr2_atomic32_t;


#define sev()	__asm__ __volatile__ ("sev" : : : "memory")
#define wfe()	__asm__ __volatile__ ("wfe" : : : "memory")
#define wfi()	__asm__ __volatile__ ("wfi" : : : "memory")

#define isb() __asm__ __volatile__ ("isb" : : : "memory")
#define dsb() __asm__ __volatile__ ("dsb" : : : "memory")
#define dmb() __asm__ __volatile__ ("dmb" : : : "memory")
#define smp_mb()	dmb()
#define smp_wmb()	dmb()

#define ALT_SMP(smp, up)					\
	"9998:	" smp "\n"					\
	"	.pushsection \".alt.smp.init\", \"a\"\n"	\
	"	.long	9998b\n"				\
	"	" up "\n"					\
	"	.popsection\n"

#define SEV		ALT_SMP("sev", "nop")
#define WFE(cond)	ALT_SMP("wfe" cond, "nop")


static inline void dsb_sev(void)
{
	__asm__ __volatile__ (
		"dsb\n"
		SEV
	);
}


static inline void easyr2_atomic32_add(easyr2_atomic32_t *v, int i)
{
	unsigned long tmp;
	int result;

	__asm__ __volatile__("@ atomic_add\n"
"1:	ldrex	%0, [%3]\n"
"	add	%0, %0, %4\n"
"	strex	%1, %0, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)
	: "r" (v), "Ir" (i)
	: "cc");
}

static inline int easyr2_atomic32_add_return(easyr2_atomic32_t *v, int i)
{
	unsigned long tmp;
	int result;

	smp_mb();

	__asm__ __volatile__("@ atomic_add_return\n"
"1:	ldrex	%0, [%3]\n"
"	add	%0, %0, %4\n"
"	strex	%1, %0, [%3]\n"
"	teq	%1, #0\n"
"	bne	1b"
	: "=&r" (result), "=&r" (tmp), "+Qo" (*v)
	: "r" (v), "Ir" (i)
	: "cc");

	smp_mb();
	
	return result;
}

#define easyr2_atomic32_inc(v)		easyr2_atomic_add(v, 1)
#define easyr2_atomic32_dec(v)		easyr2_atomic_add(v, -1)

typedef volatile int32_t easyr2_atomic_t;
#define easyr2_atomic_add(v,i) easyr2_atomic32_add(v,i)
#define easyr2_atomic_add_return(v,diff) easyr2_atomic32_add_return(v,diff)
#define easyr2_atomic_inc(v) easyr2_atomic32_inc(v)
#define easyr2_atomic_dec(v) easyr2_atomic32_dec(v)


static inline int easyr2_atomic_cmpxchg(easyr2_atomic_t *ptr, int old, int neww)
{
	unsigned long oldval, res;

	smp_mb();

	do {
		__asm__ __volatile__("@ atomic_cmpxchg\n"
		"ldrex	%1, [%3]\n"
		"mov	%0, #0\n"
		"teq	%1, %4\n"
		"strexeq %0, %5, [%3]\n"
		    : "=&r" (res), "=&r" (oldval), "+Qo" (*ptr)
		    : "r" (ptr), "Ir" (old), "r" (neww)
		    : "cc");
	} while (res);

	smp_mb();

	return oldval;
}


/*
 * This function is different from the kernel ones
 * */
static inline int easyr2_atomic_cmp_set(easyr2_atomic_t *ptr, int old, int neww)
{
	int oldval, res;
	
	if (*ptr == old && old == neww)
		return 1;

	smp_mb();
	do {
	    __asm__ __volatile__("@ atomic_cmpxchg\n"
	    "ldrex	%1, [%3]\n"
	    "mov	%0, #0\n"
	    "teq	%1, %4\n"
	    "strexeq	%0, %5, [%3]\n"
		: "=&r" (res), "=&r" (oldval), "+Qo" (*ptr)
		: "r" (ptr), "Ir" (old), "r" (neww)
		: "cc");
	} while (res);

	smp_mb();
	
	return (*ptr != oldval);
}


// #define easyr2_trylock(lock)  (*(lock) == 0 && easyr2_atomic_cmp_set(lock, 0, 1))
#define easyr2_unlock(lock)   {__asm__ ("" ::: "memory"); *(lock) = 0;}
#define easyr2_spin_unlock easyr2_unlock


static inline int easyr2_trylock(easyr2_atomic_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
"	strexeq	%0, %2, [%1]"
	: "=&r" (tmp)
	: "r" (lock), "r" (1)
	: "cc");

	if (tmp == 0) {
		smp_mb();
		return 1;
	} else {
		return 0;
	}
}


static inline void easyr2_spin_lock(easyr2_atomic_t *lock)
{
	unsigned long tmp;

	__asm__ __volatile__(
"1:	ldrex	%0, [%1]\n"
"	teq	%0, #0\n"
	WFE("ne")
"	strexeq	%0, %2, [%1]\n"
"	teqeq	%0, #0\n"
"	bne	1b"
	: "=&r" (tmp)
	: "r" (lock), "r" (1)
	: "cc");

	smp_mb();
}


static __inline__ void easyr2_clear_bit(unsigned long nr, volatile void *addr)
{
    int8_t                  *m = ((int8_t *) addr) + (nr >> 3);
    *m &= (int8_t)(~(1 << (nr & 7)));
}

static __inline__ void easyr2_set_bit(unsigned long nr, volatile void *addr)
{
    int8_t                  *m = ((int8_t *) addr) + (nr >> 3);
    *m |= (int8_t)(1 << (nr & 7));
}

typedef struct easyr2_spinrwlock_t {
    easyr2_atomic_t           lock;
} easyr2_spinrwlock_t;


#define EASYR2_SPINRWLOCK_INITIALIZER {0}

static inline int easyr2_spinrwlock_rdlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;
	unsigned long tmp, tmp2;

	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%2]\n"
	"	adds	%0, %0, #1\n"
	"	strexpl	%1, %0, [%2]\n"
		WFE("mi")
	"	rsbpls	%0, %1, #0\n"
	"	bmi	1b"
		: "=&r" (tmp), "=&r" (tmp2)
		: "r" (&rw->lock)
		: "cc");

		smp_mb();
	}

	return ret;
}

static inline int easyr2_spinrwlock_wrlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;
	unsigned long tmp;

	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%1]\n"
	"	teq	%0, #0\n"
		WFE("ne")
	"	strexeq	%0, %2, [%1]\n"
	"	teq	%0, #0\n"
	"	bne	1b"
		: "=&r" (tmp)
		: "r" (&rw->lock), "r" (0x80000000)
		: "cc");

		smp_mb();
	}

	return ret;
}

static inline int easyr2_spinrwlock_try_rdlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;
	unsigned long tmp, tmp2 = 1;

	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%2]\n"
	"	adds	%0, %0, #1\n"
	"	strexpl	%1, %0, [%2]\n"
		: "=&r" (tmp), "+r" (tmp2)
		: "r" (&rw->lock)
		: "cc");

		smp_mb();
	}
	/*
	 * when conflict happens , tmp2 should be 1
	 */

	if (tmp2 == 0) {
		return ret;
	} else
		ret = EASYR2_AGAIN;

	return ret;
}


static inline int easyr2_spinrwlock_try_wrlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;
	unsigned long tmp = 0;

	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		__asm__ __volatile__(
	"1:	ldrex	%0, [%1]\n"
	"	teq	%0, #0\n"
	"	strexeq	%0, %2, [%1]"
		: "=&r" (tmp)
		: "r" (&rw->lock), "r" (0x80000000)
		: "cc");

		if (tmp == 0 ) {
			smp_mb();
			return ret;
		} else
			ret = EASYR2_AGAIN;
	}
	return ret;
}

static inline int easyr2_spinrlock_unlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;
	unsigned long tmp, tmp2;


	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		smp_mb();
		__asm__ __volatile__(
	"1:	ldrex	%0, [%2]\n"
	"	sub	%0, %0, #1\n"
	"	strex	%1, %0, [%2]\n"
	"	teq	%1, #0\n"
	"	bne	1b"
		: "=&r" (tmp), "=&r" (tmp2)
		: "r" (&rw->lock)
		: "cc");
	}

	if (tmp == 0)
		dsb_sev();

	return ret;
}


static inline int easyr2_spinwlock_unlock(easyr2_spinrwlock_t *rw)
{
	int ret = EASYR2_OK;

	if (rw == NULL) {
		ret = EASYR2_ERROR;
	} else {
		smp_mb();

		__asm__ __volatile__(
		"str	%1, [%0]\n"
		:
		: "r" (&rw->lock), "r" (0)
		: "cc");

		dsb_sev();
	}
	return ret;
}

EASYR2_CPP_END

#endif /* EASYR2_ATOMIC_H_ */

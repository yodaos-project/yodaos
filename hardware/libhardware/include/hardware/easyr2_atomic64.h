/*
 * easyr2_atomic64.h
 *
 *  Created on: 2016-7-14
 *      Author: medea@rokid.com
 */

#ifndef EASYR2_ATOMIC64_H_
#define EASYR2_ATOMIC64_H_

#include <stdint.h>
#include "easyr2_define.h"

EASYR2_CPP_START

#define TICKET_SHIFT	16
#define smp_mb()	asm volatile("dmb ish" : : : "memory")
#define smp_wmb()	smp_mb()

typedef volatile int32_t            easyr2_atomic32_t;
#define easyr2_atomic_read(v)	(*(volatile int *)&v)
#define easyr2_atomic_set(v,i)	((v) = (i))

/*
 * AArch64 UP and SMP safe atomic ops.  We use load exclusive and
 * store exclusive to ensure that these are atomic.  We may loop
 * to ensure that the update happens.
 */
static inline void easyr2_atomic32_add(easyr2_atomic32_t *v, int i)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add\n"
"1:	ldxr	%w0, %2\n"
"	add	%w0, %w0, %w3\n"
"	stxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (*v)
	: "Ir" (i));
}

static inline int easyr2_atomic32_add_return(easyr2_atomic32_t *v, int i)
{
	unsigned long tmp;
	int result;

	asm volatile("// atomic_add_return\n"
"1:	ldxr	%w0, %2\n"
"	add	%w0, %w0, %w3\n"
"	stlxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b"
	: "=&r" (result), "=&r" (tmp), "+Q" (*v)
	: "Ir" (i)
	: "memory");

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
	unsigned long tmp;
	int oldval;

	smp_mb();

	asm volatile("// atomic_cmpxchg\n"
"1:	ldxr	%w1, %2\n"
"	cmp	%w1, %w3\n"
"	b.ne	2f\n"
"	stxr	%w0, %w4, %2\n"
"	cbnz	%w0, 1b\n"
"2:"
	: "=&r" (tmp), "=&r" (oldval), "+Q" (*ptr)
	: "Ir" (old), "r" (neww)
	: "cc");

	smp_mb();
	return oldval;
}


/*
 * This function is different from the kernel ones
 * */
static inline int easyr2_atomic_cmp_set(easyr2_atomic_t *ptr, int old, int neww)
{
	unsigned long tmp;
	int oldval;

	if(*ptr == old && old == neww)
		return -1;

	smp_mb();

	asm volatile("// atomic_cmpxchg\n"
"1:	ldxr	%w1, %2\n"
"	cmp	%w1, %w3\n"
"	b.ne	2f\n"
"	stxr	%w0, %w4, %2\n"
"	cbnz	%w0, 1b\n"
"2:"
	: "=&r" (tmp), "=&r" (oldval), "+Q" (*ptr)
	: "Ir" (old), "r" (neww)
	: "cc");

	smp_mb();
	return oldval;
}


static inline void easyr2_spin_unlock(easyr2_atomic_t *lock)
{
	asm volatile(
"	stlrh	%w1, %0\n"
	: "=Q" (*lock)
	: "r" (*lock + 1)
	: "memory");
}

static inline int easyr2_trylock(easyr2_atomic_t *lock)
{
	unsigned int tmp;
	easyr2_atomic_t lockval;

	asm volatile(
"	prfm	pstl1strm, %2\n"
"1:	ldaxr	%w0, %2\n"
"	eor	%w1, %w0, %w0, ror #16\n"
"	cbnz	%w1, 2f\n"
"	add	%w0, %w0, %3\n"
"	stxr	%w1, %w0, %2\n"
"	cbnz	%w1, 1b\n"
"2:"
	: "=&r" (lockval), "=&r" (tmp), "+Q" (*lock)
	: "I" (1 << TICKET_SHIFT)
	: "memory");

	return !tmp;
}

static inline void easyr2_spin_lock(easyr2_atomic_t *lock)
{
	unsigned int tmp;
	easyr2_atomic_t lockval, newval;

	asm volatile(
	/* Atomically increment the next ticket. */
"	prfm	pstl1strm, %3\n"
"1:	ldaxr	%w0, %3\n"
"	add	%w1, %w0, %w5\n"
"	stxr	%w2, %w1, %3\n"
"	cbnz	%w2, 1b\n"
	/* Did we get the lock? */
"	eor	%w1, %w0, %w0, ror #16\n"
"	cbz	%w1, 3f\n"
	/*
	 * No: spin on the owner. Send a local event to avoid missing an
	 * unlock before the exclusive load.
	 */
"	sevl\n"
"2:	wfe\n"
"	ldaxrh	%w2, %4\n"
"	eor	%w1, %w2, %w0, lsr #16\n"
"	cbnz	%w1, 2b\n"
	/* We got the lock. Critical section starts here. */
"3:"
	: "=&r" (lockval), "=&r" (newval), "=&r" (tmp), "+Q" (*lock)
	: "Q" (*lock), "I" (1 << TICKET_SHIFT)
	: "memory");
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

/*  comment by medea, no component use these rw spin lock 
 *  so unuse these functions at first in 64 platform 
*/

/*
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
*/
	/*
	 * when conflict happens , tmp2 should be 1
	 */
/*
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
*/

EASYR2_CPP_END

#endif /* EASYR2_ATOMIC_H_ */

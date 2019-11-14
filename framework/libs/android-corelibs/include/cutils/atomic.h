/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_CUTILS_ATOMIC_H
#define ANDROID_CUTILS_ATOMIC_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ANDROID_ATOMIC_INLINE
//#define ANDROID_ATOMIC_INLINE static inline
#define ANDROID_ATOMIC_INLINE
#endif

ANDROID_ATOMIC_INLINE
int32_t android_atomic_inc(volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_dec(volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_add(int32_t value, volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_and(int32_t value, volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_or(int32_t value, volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_acquire_load(volatile const int32_t* addr);

ANDROID_ATOMIC_INLINE
int32_t android_atomic_release_load(volatile const int32_t* addr);

/*
 * Perform an atomic store with "acquire" or "release" ordering.
 *
 * Note that the notion of an "acquire" ordering for a store does not
 * really fit into the C11 or C++11 memory model.  The extra ordering
 * is normally observable only by code using memory_order_relaxed
 * atomics, or data races.  In the rare cases in which such ordering
 * is called for, use memory_order_relaxed atomics and a trailing
 * atomic_thread_fence (typically with memory_order_release,
 * not memory_order_acquire!) instead.
 */
ANDROID_ATOMIC_INLINE
void android_atomic_acquire_store(int32_t value, volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
void android_atomic_release_store(int32_t value, volatile int32_t* addr);

/*
 * Compare-and-set operation with "acquire" or "release" ordering.
 *
 * This returns zero if the new value was successfully stored, which will
 * only happen when *addr == oldvalue.
 *
 * (The return value is inverted from implementations on other platforms,
 * but matches the ARM ldrex/strex result.)
 *
 * Implementations that use the release CAS in a loop may be less efficient
 * than possible, because we re-issue the memory barrier on each iteration.
 */
ANDROID_ATOMIC_INLINE
int android_atomic_acquire_cas(int32_t oldvalue, int32_t newvalue,
                           volatile int32_t* addr);

ANDROID_ATOMIC_INLINE
int android_atomic_release_cas(int32_t oldvalue, int32_t newvalue,
                               volatile int32_t* addr);

/*
 * Fence primitives.
 */
ANDROID_ATOMIC_INLINE
void android_compiler_barrier(void);

ANDROID_ATOMIC_INLINE
void android_memory_barrier(void);

/*
 * Aliases for code using an older version of this header.  These are now
 * deprecated and should not be used.  The definitions will be removed
 * in a future release.
 */
#define android_atomic_write android_atomic_release_store
#define android_atomic_cmpxchg android_atomic_release_cas
#ifdef __cplusplus
}
#endif

#endif // ANDROID_CUTILS_ATOMIC_H

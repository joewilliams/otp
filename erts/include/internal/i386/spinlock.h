/*
 * %CopyrightBegin%
 *
 * Copyright Ericsson AB 2005-2010. All Rights Reserved.
 *
 * The contents of this file are subject to the Erlang Public License,
 * Version 1.1, (the "License"); you may not use this file except in
 * compliance with the License. You should have received a copy of the
 * Erlang Public License along with this software. If not, it can be
 * retrieved online at http://www.erlang.org/.
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * %CopyrightEnd%
 */

/*
 * Native ethread spinlocks on x86/x86-64.
 * Author: Mikael Pettersson.
 */
#ifndef ETHREAD_I386_SPINLOCK_H
#define ETHREAD_I386_SPINLOCK_H

/* A spinlock is the low byte of an aligned 32-bit integer.
 * A non-zero value means that the lock is locked.
 */
typedef struct {
    volatile unsigned int lock;
} ethr_native_spinlock_t;

#if defined(ETHR_TRY_INLINE_FUNCS) || defined(ETHR_AUX_IMPL__)

static ETHR_INLINE void
ethr_native_spinlock_init(ethr_native_spinlock_t *lock)
{
    lock->lock = 0;
}

static ETHR_INLINE void
ethr_native_spin_unlock(ethr_native_spinlock_t *lock)
{
    /* To unlock we move 0 to the lock.
     * On i386 this needs to be a locked operation
     * to avoid Pentium Pro errata 66 and 92.
     */
#if defined(__x86_64__) || !defined(ETHR_PRE_PENTIUM4_COMPAT)
    __asm__ __volatile__("" : : : "memory");
    *(unsigned char*)&lock->lock = 0;
#else
    char tmp = 0;
    __asm__ __volatile__(
	"xchgb %b0, %1"
	: "=q"(tmp), "=m"(lock->lock)
	: "0"(tmp) : "memory");
#endif
}

static ETHR_INLINE int
ethr_native_spin_trylock(ethr_native_spinlock_t *lock)
{
    char tmp = 1;
    __asm__ __volatile__(
	"xchgb %b0, %1"
	: "=q"(tmp), "=m"(lock->lock)
	: "0"(tmp) : "memory");
    return tmp == 0;
}

static ETHR_INLINE int
ethr_native_spin_is_locked(ethr_native_spinlock_t *lock)
{
    return *(volatile unsigned char*)&lock->lock != 0;
}

static ETHR_INLINE void
ethr_native_spin_lock(ethr_native_spinlock_t *lock)
{
    for(;;) {
	if (__builtin_expect(ethr_native_spin_trylock(lock) != 0, 1))
	    break;
	do {
	    __asm__ __volatile__("rep;nop" : "=m"(lock->lock) : : "memory");
	} while (ethr_native_spin_is_locked(lock));
    }
}

#endif /* ETHR_TRY_INLINE_FUNCS */

#endif /* ETHREAD_I386_SPINLOCK_H */

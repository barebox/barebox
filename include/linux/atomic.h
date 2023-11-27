/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef LINUX_ATOMIC_H_
#define LINUX_ATOMIC_H_

#include <asm-generic/atomic.h>
#include <linux/compiler.h>
#include <asm-generic/cmpxchg.h>

#define raw_cmpxchg_relaxed cmpxchg

/**
 * raw_atomic_cmpxchg_relaxed() - atomic compare and exchange with relaxed ordering
 * @v: pointer to atomic_t
 * @old: int value to compare with
 * @new: int value to assign
 *
 * If (@v == @old), atomically updates @v to @new with relaxed ordering.
 *
 * Safe to use in noinstr code; prefer atomic_cmpxchg_relaxed() elsewhere.
 *
 * Return: The original value of @v.
 */
static __always_inline int
raw_atomic_cmpxchg_relaxed(atomic_t *v, int old, int new)
{
	return raw_cmpxchg_relaxed(&v->counter, old, new);
}

/**
 * atomic_try_cmpxchg_relaxed() - atomic compare and exchange with relaxed ordering
 * @v: pointer to atomic_t
 * @old: pointer to int value to compare with
 * @new: int value to assign
 *
 * If (@v == @old), atomically updates @v to @new with relaxed ordering.
 * Otherwise, updates @old to the current value of @v.
 *
 * Return: @true if the exchange occured, @false otherwise.
 */
static __always_inline bool
atomic_try_cmpxchg_relaxed(atomic_t *v, int *old, int new)
{
	int r, o = *old;
	r = raw_atomic_cmpxchg_relaxed(v, o, new);
	if (unlikely(r != o))
		*old = r;
	return likely(r == o);
}

/**
 * atomic_fetch_add() - atomic add
 * @i: int value to add
 * @v: pointer to atomic_t
 *
 * Atomically updates @v to (@v + @i).
 *
 * Return: The original value of @v.
 */
static __always_inline int
atomic_fetch_add(int i, atomic_t *v)
{
	int old = v->counter;
	v->counter += i;
	return old;
}
#define atomic_fetch_add_relaxed atomic_fetch_add
#define atomic_fetch_sub(i, v) atomic_fetch_add(-i, v)
#define atomic_fetch_sub_release atomic_fetch_sub

#endif

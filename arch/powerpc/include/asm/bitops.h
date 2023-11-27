/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * bitops.h: Bit string operations on the ppc
 */

#ifndef _PPC_BITOPS_H
#define _PPC_BITOPS_H

#include <asm/byteorder.h>
#include <asm-generic/bitops/ops.h>

/*
 * Arguably these bit operations don't imply any memory barrier or
 * SMP ordering, but in fact a lot of drivers expect them to imply
 * both, since they do on x86 cpus.
 */
#ifdef CONFIG_SMP
#define SMP_WMB		"eieio\n"
#define SMP_MB		"\nsync"
#else
#define SMP_WMB
#define SMP_MB
#endif /* CONFIG_SMP */

#define __INLINE_BITOPS	1

#if __INLINE_BITOPS
/*
 * These used to be if'd out here because using : "cc" as a constraint
 * resulted in errors from egcs.  Things may be OK with gcc-2.95.
 */
static inline void set_bit(int nr, volatile void * addr)
{
	unsigned long old;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%3\n\
	or	%0,%0,%2\n\
	stwcx.	%0,0,%3\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc" );
}

static inline void clear_bit(int nr, volatile void *addr)
{
	unsigned long old;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%3\n\
	andc	%0,%0,%2\n\
	stwcx.	%0,0,%3\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");
}

static inline void change_bit(int nr, volatile void *addr)
{
	unsigned long old;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%3\n\
	xor	%0,%0,%2\n\
	stwcx.	%0,0,%3\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");
}

static inline int test_and_set_bit(int nr, volatile void *addr)
{
	unsigned int old, t;
	unsigned int mask = 1 << (nr & 0x1f);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%4\n\
	or	%1,%0,%3\n\
	stwcx.	%1,0,%4\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=&r" (t), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");

	return (old & mask) != 0;
}

static inline int test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned int old, t;
	unsigned int mask = 1 << (nr & 0x1f);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%4\n\
	andc	%1,%0,%3\n\
	stwcx.	%1,0,%4\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=&r" (t), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");

	return (old & mask) != 0;
}

static inline int test_and_change_bit(int nr, volatile void *addr)
{
	unsigned int old, t;
	unsigned int mask = 1 << (nr & 0x1f);
	volatile unsigned int *p = ((volatile unsigned int *)addr) + (nr >> 5);

	__asm__ __volatile__(SMP_WMB "\
1:	lwarx	%0,0,%4\n\
	xor	%1,%0,%3\n\
	stwcx.	%1,0,%4\n\
	bne	1b"
	SMP_MB
	: "=&r" (old), "=&r" (t), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");

	return (old & mask) != 0;
}
#endif /* __INLINE_BITOPS */

/* Return the bit position of the most significant 1 bit in a word */
static inline int __ilog2(unsigned int x)
{
	int lz;

	asm ("cntlzw %0,%1" : "=r" (lz) : "r" (x));
	return 31 - lz;
}

static inline int ffz(unsigned int x)
{
	if ((x = ~x) == 0)
		return 32;
	return __ilog2(x & -x);
}

static inline int __ffs(unsigned long x)
{
	return __ilog2(x & -x);
}

/*
 * fls: find last (most-significant) bit set.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int fls(unsigned int x)
{
	int lz;

	asm ("cntlzw %0,%1" : "=r" (lz) : "r" (x));
	return 32 - lz;
}

#ifdef __KERNEL__

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int ffs(int x)
{
	return __ilog2(x & -x) + 1;
}

#include <asm-generic/bitops/__fls.h>
#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/hweight.h>

#endif /* __KERNEL__ */

#include <asm-generic/bitops/find.h>

#endif /* _PPC_BITOPS_H */

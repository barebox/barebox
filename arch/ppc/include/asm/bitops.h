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
extern __inline__ void set_bit(int nr, volatile void * addr)
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

extern __inline__ void clear_bit(int nr, volatile void *addr)
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

extern __inline__ void change_bit(int nr, volatile void *addr)
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

extern __inline__ int test_and_set_bit(int nr, volatile void *addr)
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

extern __inline__ int test_and_clear_bit(int nr, volatile void *addr)
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

extern __inline__ int test_and_change_bit(int nr, volatile void *addr)
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
extern __inline__ int __ilog2(unsigned int x)
{
	int lz;

	asm ("cntlzw %0,%1" : "=r" (lz) : "r" (x));
	return 31 - lz;
}

extern __inline__ int ffz(unsigned int x)
{
	if ((x = ~x) == 0)
		return 32;
	return __ilog2(x & -x);
}

static __inline__ int __ffs(unsigned long x)
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
extern __inline__ int ffs(int x)
{
	return __ilog2(x & -x) + 1;
}

#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/hweight.h>

#endif /* __KERNEL__ */

#include <asm-generic/bitops/find.h>

#define _EXT2_HAVE_ASM_BITOPS_

#ifdef __KERNEL__
/*
 * test_and_{set,clear}_bit guarantee atomicity without
 * disabling interrupts.
 */
#define ext2_set_bit(nr, addr)		test_and_set_bit((nr) ^ 0x18, addr)
#define ext2_clear_bit(nr, addr)	test_and_clear_bit((nr) ^ 0x18, addr)

#else
extern __inline__ int ext2_set_bit(int nr, void * addr)
{
	int		mask;
	unsigned char	*ADDR = (unsigned char *) addr;
	int oldbit;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	oldbit = (*ADDR & mask) ? 1 : 0;
	*ADDR |= mask;
	return oldbit;
}

extern __inline__ int ext2_clear_bit(int nr, void * addr)
{
	int		mask;
	unsigned char	*ADDR = (unsigned char *) addr;
	int oldbit;

	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);
	oldbit = (*ADDR & mask) ? 1 : 0;
	*ADDR = *ADDR & ~mask;
	return oldbit;
}
#endif	/* __KERNEL__ */

extern __inline__ int ext2_test_bit(int nr, __const__ void * addr)
{
	__const__ unsigned char	*ADDR = (__const__ unsigned char *) addr;

	return (ADDR[nr >> 3] >> (nr & 7)) & 1;
}

/*
 * This implementation of ext2_find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h and modified for a big-endian machine.
 */

#define ext2_find_first_zero_bit(addr, size) \
	ext2_find_next_zero_bit((addr), (size), 0)

#define ext2_find_next_zero_bit(addr, size, off) \
	generic_find_next_zero_le_bit((unsigned long*)addr, size, off)

/* Bitmap functions for the minix filesystem.  */
#define minix_test_and_set_bit(nr,addr) ext2_set_bit(nr,addr)
#define minix_set_bit(nr,addr) ((void)ext2_set_bit(nr,addr))
#define minix_test_and_clear_bit(nr,addr) ext2_clear_bit(nr,addr)
#define minix_test_bit(nr,addr) ext2_test_bit(nr,addr)
#define minix_find_first_zero_bit(addr,size) ext2_find_first_zero_bit(addr,size)

#endif /* _PPC_BITOPS_H */

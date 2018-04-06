/*
 * Copyright 1995, Russell King.
 * Various bits and pieces copyrights include:
 *  Linus Torvalds (test_bit).
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 *
 * Please note that the code in this file should never be included
 * from user space.  Many of these are not implemented in assembler
 * since they would be too costly.  Also, they require priviledged
 * instructions (which are not available from user mode) to ensure
 * that they are atomic.
 */

#ifndef __ASM_ARM_BITOPS_H
#define __ASM_ARM_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

/*
 * Functions equivalent of ops.h
 */
static inline void __set_bit(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] |= (1U << (nr & 7));
}

static inline void __clear_bit(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] &= ~(1U << (nr & 7));
}

static inline void __change_bit(int nr, volatile void *addr)
{
	((unsigned char *) addr)[nr >> 3] ^= (1U << (nr & 7));
}

static inline int __test_and_set_bit(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval | mask;
	return oldval & mask;
}

static inline int __test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval & ~mask;
	return oldval & mask;
}

static inline int __test_and_change_bit(int nr, volatile void *addr)
{
	unsigned int mask = 1 << (nr & 7);
	unsigned int oldval;

	oldval = ((unsigned char *) addr)[nr >> 3];
	((unsigned char *) addr)[nr >> 3] = oldval ^ mask;
	return oldval & mask;
}

/*
 * This routine doesn't need to be atomic.
 */
static inline int test_bit(int nr, const void * addr)
{
    return ((unsigned char *) addr)[nr >> 3] & (1U << (nr & 7));
}

#define set_bit(x, y)			__set_bit(x, y)
#define clear_bit(x, y)			__clear_bit(x, y)
#define change_bit(x, y)		__change_bit(x, y)
#define test_and_set_bit(x, y)		__test_and_set_bit(x, y)
#define test_and_clear_bit(x, y)	__test_and_clear_bit(x, y)
#define test_and_change_bit(x, y)	__test_and_change_bit(x, y)

#ifndef __ARMEB__
/*
 * These are the little endian definitions.
 */
extern int _find_first_zero_bit_le(const void *p, unsigned size);
extern int _find_next_zero_bit_le(const void *p, int size, int offset);
extern int _find_first_bit_le(const unsigned long *p, unsigned size);
extern int _find_next_bit_le(const unsigned long *p, int size, int offset);
#define find_first_zero_bit(p, sz)	_find_first_zero_bit_le(p, sz)
#define find_next_zero_bit(p, sz, off)	_find_next_zero_bit_le(p, sz, off)
#define find_first_bit(p, sz)		_find_first_bit_le(p, sz)
#define find_next_bit(p, sz, off)	_find_next_bit_le(p, sz, off)

#define WORD_BITOFF_TO_LE(x)		((x))

#else		/* ! __ARMEB__ */

/*
 * These are the big endian definitions.
 */
extern int _find_first_zero_bit_be(const void *p, unsigned size);
extern int _find_next_zero_bit_be(const void *p, int size, int offset);
extern int _find_first_bit_be(const unsigned long *p, unsigned size);
extern int _find_next_bit_be(const unsigned long *p, int size, int offset);
#define find_first_zero_bit(p, sz)	_find_first_zero_bit_be(p, sz)
#define find_next_zero_bit(p, sz, off)	_find_next_zero_bit_be(p, sz, off)
#define find_first_bit(p, sz)		_find_first_bit_be(p, sz)
#define find_next_bit(p, sz, off)	_find_next_bit_be(p, sz, off)

#define WORD_BITOFF_TO_LE(x)		((x) ^ 0x18)

#endif		/* __ARMEB__ */

#if defined (CONFIG_CPU_32) && defined(__LINUX_ARM_ARCH__) && (__LINUX_ARM_ARCH__ >= 5)
static inline int constant_fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

/*
 * On ARMv5 and above those functions can be implemented around
 * the clz instruction for much better code efficiency.
 */
#define fls(x) \
	(__builtin_constant_p(x) ? constant_fls(x) : \
	({ int __r; asm("clz\t%0, %1" : "=r"(__r) : "r"(x) : "cc"); 32-__r; }))
#define ffs(x) ({ unsigned long __t = (x); fls(__t &-__t); })
#define __ffs(x) (ffs(x) - 1)
#define ffz(x) __ffs(~(x))
#else		/* ! __ARM__USE_GENERIC_FF */
#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/__ffs.h>
#include <asm-generic/bitops/ffs.h>
#include <asm-generic/bitops/fls.h>
#endif		/* __ARM__USE_GENERIC_FF */

#include <asm-generic/bitops/__fls.h>

#include <asm-generic/bitops/fls64.h>

#include <asm-generic/bitops/hweight.h>

#endif /* _ARM_BITOPS_H */

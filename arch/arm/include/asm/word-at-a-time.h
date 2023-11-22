/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 ARM Ltd.
 */
#ifndef __ASM_WORD_AT_A_TIME_H
#define __ASM_WORD_AT_A_TIME_H

#if !defined(__AARCH64EB__) && !defined(__ARMEB__)

#include <linux/kernel.h>
#include <linux/bitops.h>

struct word_at_a_time {
	const unsigned long one_bits, high_bits;
};

#define WORD_AT_A_TIME_CONSTANTS { REPEAT_BYTE(0x01), REPEAT_BYTE(0x80) }

static inline unsigned long has_zero(unsigned long a, unsigned long *bits,
				     const struct word_at_a_time *c)
{
	unsigned long mask = ((a - c->one_bits) & ~a) & c->high_bits;
	*bits = mask;
	return mask;
}

#define prep_zero_mask(a, bits, c) (bits)

static inline unsigned long create_zero_mask(unsigned long bits)
{
	bits = (bits - 1) & ~bits;
	return bits >> 7;
}

static inline unsigned long find_zero(unsigned long mask)
{
	unsigned long ret;

#if __LINUX_ARM_ARCH__ >= 8
	ret = fls64(mask) >> 3;
#elif __LINUX_ARM_ARCH__ >= 5
	/* We have clz available. */
	ret = fls(mask) >> 3;
#else
	/* (000000 0000ff 00ffff ffffff) -> ( 1 1 2 3 ) */
	ret = (0x0ff0001 + mask) >> 23;
	/* Fix the 1 for 00 case */
	ret &= mask;
#endif

	return ret;
}

#define zero_bytemask(mask) (mask)

#else	/* __AARCH64EB__ || __ARMEB__ */
#include <asm-generic/word-at-a-time.h>
#endif

#endif /* __ASM_WORD_AT_A_TIME_H */

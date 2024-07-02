/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __DEBUG_LL_SEMIHOSTING_H
#define __DEBUG_LL_SEMIHOSTING_H

#include <asm/semihosting.h>
#include <linux/stddef.h>

#ifdef CONFIG_DEBUG_SEMIHOSTING
static inline void PUTC_LL(char c)
{
	semihosting_putc(NULL, c);
}
#endif

#endif /* __DEBUG_LL_ARM_SEMIHOSTING_H */

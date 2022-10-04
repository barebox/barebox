/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_DEBUG_LL__
#define __ASM_DEBUG_LL__

#undef putchar

static inline void PUTC_LL(char ch)
{
	int putchar(int c);
	putchar(ch);
}

#define putchar barebox_putchar

#endif /* __ASM_DEBUG_LL__ */

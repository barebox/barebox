/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * debug_ll.h
 *
 *  written by Marc Singer
 *  12 Feb 2005
 *
 *  Copyright (C) 2005 Marc Singer
 */

#ifndef __INCLUDE_DEBUG_LL_H__
#define   __INCLUDE_DEBUG_LL_H__

#ifdef CONFIG_HAS_DEBUG_LL
/*
 * asm/debug_ll.h should implement PUTC_LL. This can be a macro or a static
 * inline function. Note that several SoCs expect the UART to be initialized
 * by a debugger or a first stage bootloader. You won't see anything without
 * this initialization. Depending on the PUTC_LL implementation the board might
 * also hang in PUTC_LL without proper initialization.
 */
#include <asm/debug_ll.h>
#endif

#if defined (CONFIG_DEBUG_LL)

static inline void putc_ll(char value)
{
	PUTC_LL(value);
}

static inline void puthexc_ll(unsigned char value)
{
	int i; unsigned char ch;

	for (i = 2; i--; ) {
		ch = ((value >> (i * 4)) & 0xf);
		ch += (ch >= 10) ? 'a' - 10 : '0';
		putc_ll(ch);
	}
}

static inline void puthex_ll(unsigned long value)
{
	int i;

	for (i = sizeof(unsigned long); i--; )
		puthexc_ll(value >> (i * 8));
}

/*
 * Be careful with puts_ll, it only works if the binary is running at the
 * link address which often is not the case during early startup. If in doubt
 * don't use it.
 */
static inline void puts_ll(const char * str)
{
	while (*str) {
		if (*str == '\n')
			putc_ll('\r');

		putc_ll(*str);
		str++;
	}
}

static inline void putws_ll(const unsigned short * wstr)
{
	while (*wstr) {
		if (*wstr == L'\n')
			putc_ll('\r');

		putc_ll(*wstr);
		wstr++;
	}
}

#else

static inline void putc_ll(char value)
{
}

static inline void putws_ll(const unsigned short * wstr)
{
}

static inline void puthexc_ll(unsigned char value)
{
}

static inline void puthex_ll(unsigned long value)
{
}

/*
 * Be careful with puts_ll, it only works if the binary is running at the
 * link address which often is not the case during early startup. If in doubt
 * don't use it.
 */
static inline void puts_ll(const char * str)
{
}

#endif

#endif  /* __INCLUDE_DEBUG_LL_H__ */

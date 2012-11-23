/*
 * debug_ll.h
 *
 *  written by Marc Singer
 *  12 Feb 2005
 *
 *  Copyright (C) 2005 Marc Singer
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __INCLUDE_DEBUG_LL_H__
#define   __INCLUDE_DEBUG_LL_H__

#if defined (CONFIG_DEBUG_LL)

/*
 * mach/debug_ll.h should implement PUTC_LL. This can be a macro or a static
 * inline function. Note that several SoCs expect the UART to be initialized
 * by a debugger or a first stage bootloader. You won't see anything without
 * this initialization. Depending on the PUTC_LL implementation the board might
 * also hang in PUTC_LL without proper initialization.
 */
# include <mach/debug_ll.h>

# define PUTHEX_LL(value)  ({ unsigned long v = (unsigned long) (value); \
			     int i; unsigned char ch; \
			     for (i = 8; i--; ) {\
			     ch = ((v >> (i*4)) & 0xf);\
			     ch += (ch >= 10) ? 'a' - 10 : '0';\
			     PUTC_LL (ch); }})

/*
 * Be careful with PUTS_LL, it only works if the binary is running at the
 * link address which often is not the case during early startup. If in doubt
 * don't use it.
 */
static __inline__ void PUTS_LL(const char * str)
{
	while (*str) {
		if (*str == '\n') {
			PUTC_LL('\r');
		}
		PUTC_LL(*str);
		str++;
	}
}

#else
# define PUTC_LL(c) do {} while (0)
# define PUTHEX_LL(v) do {} while (0)
# define PUTS_LL(c) do {} while (0)

#endif

#endif  /* __INCLUDE_DEBUG_LL_H__ */

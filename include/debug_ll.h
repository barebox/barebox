/* debug_ll.h
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 */

#ifndef __INCLUDE_DEBUG_LL_H__
#define   __INCLUDE_DEBUG_LL_H__

#if defined (CONFIG_DEBUG_LL)
# include <mach/debug_ll.h>

#define PUTC_LL(x) putc(x)
# define PUTHEX_LL(value)  ({ unsigned long v = (unsigned long) (value); \
			     int i; unsigned char ch; \
			     for (i = 8; i--; ) {\
			     ch = ((v >> (i*4)) & 0xf);\
			     ch += (ch >= 10) ? 'a' - 10 : '0';\
			     PUTC_LL (ch); }})
#else
# define PUTC_LL(c) do {} while (0)
# define PUTHEX_LL(v) do {} while (0)

#endif

#endif  /* __INCLUDE_DEBUG_LL_H__ */

/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of U-Boot V2.
 *
 * U-Boot V2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * U-Boot V2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with U-Boot V2.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Arch dependant configuration of std libc string and memory functions.
 */
#ifndef __ASM_M68K_STRING_H
#define __ASM_M68K_STRING_H

/*
 * We don't do inline string functions, since the
 * optimised inline asm versions are not small.
 */

#undef __HAVE_ARCH_STRRCHR
extern char * strrchr(const char * s, int c);

#undef __HAVE_ARCH_STRCHR
extern char * strchr(const char * s, int c);

#undef __HAVE_ARCH_MEMCPY
extern void * memcpy(void *, const void *, __kernel_size_t);

#undef __HAVE_ARCH_MEMMOVE
extern void * memmove(void *, const void *, __kernel_size_t);

#undef __HAVE_ARCH_MEMCHR
extern void * memchr(const void *, int, __kernel_size_t);

#undef __HAVE_ARCH_MEMZERO
#undef __HAVE_ARCH_MEMSET
extern void * memset(void *, int, __kernel_size_t);
extern void memzero(void *ptr, __kernel_size_t n);

#endif

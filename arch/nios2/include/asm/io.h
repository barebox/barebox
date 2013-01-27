/*
 * (C) Copyright 2004, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_NIOS2_IO_H_
#define __ASM_NIOS2_IO_H_

#include <asm/byteorder.h>

#define __raw_writeb(v, a)       (*(volatile unsigned char  *)(a) = (v))
#define __raw_writew(v, a)       (*(volatile unsigned short *)(a) = (v))
#define __raw_writel(v, a)       (*(volatile unsigned int   *)(a) = (v))

#define __raw_readb(a)          (*(volatile unsigned char  *)(a))
#define __raw_readw(a)          (*(volatile unsigned short *)(a))
#define __raw_readl(a)          (*(volatile unsigned int   *)(a))

#define readb(addr)\
	({unsigned char val;\
	 asm volatile("ldbio %0, 0(%1)" : "=r"(val) : "r" (addr)); val; })
#define readw(addr)\
	({unsigned short val;\
	 asm volatile("ldhio %0, 0(%1)" : "=r"(val) : "r" (addr)); val; })
#define readl(addr)\
	({unsigned int val;\
	 asm volatile("ldwio %0, 0(%1)" : "=r"(val) : "r" (addr)); val; })

#define writeb(val, addr)\
	asm volatile("stbio %0, 0(%1)" : : "r" (val), "r" (addr))
#define writew(val, addr)\
	asm volatile ("sthio %0, 0(%1)" : : "r" (val), "r" (addr))
#define writel(val, addr)\
	asm volatile("stwio %0, 0(%1)" : : "r" (val), "r" (addr))

#endif /* __ASM_NIOS2_IO_H_ */

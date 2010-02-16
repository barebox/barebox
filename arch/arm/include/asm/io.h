/*
 *  linux/include/asm-arm/io.h
 *
 *  Copyright (C) 1996-2000 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modifications:
 *  16-Sep-1996	RMK	Inlined the inx/outx functions & optimised for both
 *			constant addresses and variable addresses.
 *  04-Dec-1997	RMK	Moved a lot of this stuff to the new architecture
 *			specific IO header files.
 *  27-Mar-1999	PJB	Second parameter of memcpy_toio is const..
 *  04-Apr-1999	PJB	Added check_signature.
 *  12-Dec-1999	RMK	More cleanups
 *  18-Jun-2000 RMK	Removed virt_to_* and friends definitions
 */

/**
 * @file
 * @brief ARM IO access functions
 */

#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))

#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))

#define writeb(v,a) __raw_writeb(v,a)
#define writew(v,a) __raw_writew(v,a)
#define writel(v,a) __raw_writel(v,a)

#define readb(a) __raw_readb(a)
#define readw(a) __raw_readw(a)
#define readl(a) __raw_readl(a)

/* for the ARM architecture the string functions are library based */
extern void writesb(void __iomem*, const void*, int);
extern void writesw(void __iomem*, const void*, int);
extern void writesl(void __iomem*, const void*, int);
extern void readsb(const void __iomem*, void*, int);
extern void readsw(const void __iomem*, void*, int);
extern void readsl(const void __iomem*, void*, int);

#endif	/* __ASM_ARM_IO_H */

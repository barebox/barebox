/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/** @file
 *  This file contains declaration for early output support
 */
#ifndef __INCLUDE_ARCH_DEBUG_LL_NS16550_H__
#define __INCLUDE_ARCH_DEBUG_LL_NS16550_H__

#include <asm/io.h>

#define rbr		(0 << DEBUG_LL_UART_SHIFT)
#define lsr		(5 << DEBUG_LL_UART_SHIFT)

#define LSR_THRE	0x20	/* Xmit holding register empty */

static __inline__ void putc(char ch)
{
	while (!(__raw_readb((u8 *)DEBUG_LL_UART_ADDR + lsr) & LSR_THRE));
	__raw_writeb(ch, (u8 *)DEBUG_LL_UART_ADDR + rbr);
}

#endif  /* __INCLUDE_ARCH_DEBUG_LL_NS16550_H__ */

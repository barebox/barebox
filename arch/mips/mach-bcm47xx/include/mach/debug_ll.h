/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
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
 *  This File contains declaration for early output support
 */
#ifndef __INCLUDE_ARCH_DEBUG_LL_H__
#define   __INCLUDE_ARCH_DEBUG_LL_H__

#include <io.h>
#include <mach/hardware.h>

#define rbr		0
#define lsr		5
#define LSR_THRE	0x20	/* Xmit holding register empty */

static __inline__ void PUTC_LL(char ch)
{
	void *base = (void *)DEBUG_LL_UART_ADDR;

	while (!(__raw_readb(base + lsr) & LSR_THRE));
	__raw_writeb(ch, base + rbr);
}

#endif  /* __INCLUDE_ARCH_DEBUG_LL_H__ */

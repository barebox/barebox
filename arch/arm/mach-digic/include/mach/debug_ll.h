/*
 * Copyright (C) 2013, 2014 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_DEBUG_LL_H__
#define __MACH_DEBUG_LL_H__

#include <io.h>
#include <mach/digic4.h>
#include <mach/uart.h>

#define DEBUG_LL_UART         DIGIC4_UART

/* Serial interface registers */
#define DEBUG_LL_UART_TX         (DEBUG_LL_UART + DIGIC_UART_TX)
#define DEBUG_LL_UART_ST         (DEBUG_LL_UART + DIGIC_UART_ST)

static inline void PUTC_LL(char ch)
{
	while (!(readl(DEBUG_LL_UART_ST) & DIGIC_UART_ST_TX_RDY))
		; /* noop */

	writel(0x06, DEBUG_LL_UART_ST);
	writel(ch, DEBUG_LL_UART_TX);
}

#endif /* __MACH_DEBUG_LL_H__ */

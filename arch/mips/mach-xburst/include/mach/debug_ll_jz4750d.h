/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __INCLUDE_DEBUG_LL_JZ4750D_H__
#define __INCLUDE_DEBUG_LL_JZ4750D_H__

#include <mach/jz4750d_regs.h>

#ifdef CONFIG_JZ4750D_DEBUG_LL_UART0
#define DEBUG_LL_UART_ADDR	UART0_BASE
#endif

#ifdef CONFIG_JZ4750D_DEBUG_LL_UART1
#define DEBUG_LL_UART_ADDR	UART1_BASE
#endif

#ifdef CONFIG_JZ4750D_DEBUG_LL_UART2
#define DEBUG_LL_UART_ADDR	UART2_BASE
#endif

#define DEBUG_LL_UART_SHIFT	2

#define DEBUG_LL_UART_CLK	(12000000 / 16)
#define DEBUG_LL_UART_BPS	CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR	(DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#endif /* __INCLUDE_DEBUG_LL_JZ4750D_H__ */

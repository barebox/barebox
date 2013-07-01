/*
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
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
#ifndef __NETGEAR_WG102_DEBUG_LL_H__
#define __NETGEAR_WG102_DEBUG_LL_H__

#include <mach/ar2312_regs.h>

#define DEBUG_LL_UART_ADDR	KSEG1ADDR(AR2312_UART0)
#define DEBUG_LL_UART_SHIFT	AR2312_UART_SHIFT

#define DEBUG_LL_UART_CLK   (45000000 / 16)
#define DEBUG_LL_UART_BPS   CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#endif /* __NETGEAR_WG102_DEBUG_LL_H__ */

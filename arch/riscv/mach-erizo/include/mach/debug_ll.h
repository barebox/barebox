/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MACH_ERIZO_DEBUG_LL__
#define __MACH_ERIZO_DEBUG_LL__

/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>

#define DEBUG_LL_UART_ADDR	0x90000000
#define DEBUG_LL_UART_SHIFT	2
#define DEBUG_LL_UART_IOSIZE32

#define DEBUG_LL_UART_CLK       (24000000 / 16)
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#endif /* __MACH_ERIZO_DEBUG_LL__ */

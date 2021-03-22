/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2017 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __MACH_VIRT_DEBUG_LL__
#define __MACH_VIRT_DEBUG_LL__

/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>

#define DEBUG_LL_UART_ADDR	0x10000000
#define DEBUG_LL_UART_SHIFT	0
#define DEBUG_LL_UART_IOSIZE8

#define DEBUG_LL_UART_CLK       0x00384000
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#endif /* __MACH_VIRT_DEBUG_LL__ */

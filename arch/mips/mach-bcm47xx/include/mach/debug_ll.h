/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __INCLUDE_ARCH_DEBUG_LL_H__
#define   __INCLUDE_ARCH_DEBUG_LL_H__

#include <mach/hardware.h>

#define DEBUG_LL_UART_SHIFT	0

#define DEBUG_LL_UART_CLK   (25804800 / 16)
#define DEBUG_LL_UART_BPS   CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#include <asm/debug_ll_ns16550.h>

#endif  /* __INCLUDE_ARCH_DEBUG_LL_H__ */

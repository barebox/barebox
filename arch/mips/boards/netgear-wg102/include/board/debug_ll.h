/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
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

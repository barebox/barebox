/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__
#define __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__

#include <mach/hardware.h>

#define DEBUG_LL_UART_ADDR	MALTA_PIIX4_UART0
#define DEBUG_LL_UART_SHIFT	0

#define DEBUG_LL_UART_CLK       1843200
#define DEBUG_LL_UART_BPS       CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#endif /* __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__ */

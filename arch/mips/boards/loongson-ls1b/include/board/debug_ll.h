/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __LOONGSON_TECH_LS1B_DEBUG_LL_H__
#define __LOONGSON_TECH_LS1B_DEBUG_LL_H__

#include <asm/addrspace.h>
#include <mach/loongson1.h>

#define DEBUG_LL_UART_ADDR	KSEG1ADDR(LS1X_UART2_BASE)
#define DEBUG_LL_UART_SHIFT	0

#define DEBUG_LL_UART_CLK   (83000000 / 16)
#define DEBUG_LL_UART_BPS   CONFIG_BAUDRATE
#define DEBUG_LL_UART_DIVISOR   (DEBUG_LL_UART_CLK / DEBUG_LL_UART_BPS)

#endif /* __LOONGSON_TECH_LS1B_DEBUG_LL_H__ */

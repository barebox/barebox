/*
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __MACH_BCM2835_DEBUG_LL_H__
#define __MACH_BCM2835_DEBUG_LL_H__

#include <mach/platform.h>

#ifndef CONFIG_MACH_RPI_DEBUG_UART_BASE
#define CONFIG_MACH_RPI_DEBUG_UART_BASE 0
#endif

#define DEBUG_LL_UART_ADDR CONFIG_MACH_RPI_DEBUG_UART_BASE

#include <asm/debug_ll_pl011.h>

#endif /* __MACH_BCM2835_DEBUG_LL_H__ */

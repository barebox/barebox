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

#ifdef CONFIG_DEBUG_RPI1_UART

#define DEBUG_LL_UART_ADDR BCM2835_PL011_BASE
#include <debug_ll/pl011.h>

#elif defined CONFIG_DEBUG_RPI2_3_UART

#define DEBUG_LL_UART_ADDR BCM2836_PL011_BASE
#include <debug_ll/pl011.h>

#endif

#endif /* __MACH_BCM2835_DEBUG_LL_H__ */

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

#ifndef __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__
#define __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__

#include <mach/hardware.h>

#define DEBUG_LL_UART_ADDR	MALTA_PIIX4_UART0
#define DEBUG_LL_UART_SHIFT	0
#define DEBUG_LL_UART_DIVISOR 1843200 /* no matter for emulated port */

#endif /* __INCLUDE_BOARD_DEBUG_LL_QEMU_MALTA_H__ */

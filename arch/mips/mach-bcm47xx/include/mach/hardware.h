/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
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

#ifndef __INCLUDE_ARCH_HARDWARE_H__
#define   __INCLUDE_ARCH_HARDWARE_H__

#define DEBUG_LL_UART_ADDR	0xb8000300

/*
 * Reset register.
 */
#define SOFTRES_REG	0xbf000500
#define GORESET		0x42

#endif  /* __INCLUDE_ARCH_HARDWARE_H__ */

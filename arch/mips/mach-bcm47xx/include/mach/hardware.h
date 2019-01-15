/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
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

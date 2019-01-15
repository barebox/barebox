/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2011 Antony Pavlov <antonynpavlov@gmail.com>
 */

#ifndef __INCLUDE_ARCH_HARDWARE_H__
#define   __INCLUDE_ARCH_HARDWARE_H__

#define MALTA_PIIX4_UART0	0xb80003f8

#define MALTA_CBUS_UART	0xbf000900
#define MALTA_CBUS_UART_SHIFT	3

/*
 * Reset register.
 */
#define SOFTRES_REG	0xbf000500
#define GORESET		0x42

#endif  /* __INCLUDE_ARCH_HARDWARE_H__ */

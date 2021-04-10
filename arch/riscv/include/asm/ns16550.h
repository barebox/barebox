/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016, 2017 Antony Pavlov <antonynpavlov@gmail.com>
 */

/** @file
 *  This file contains declaration for early output support
 */
#ifndef __ASM_NS16550_H__
#define __ASM_NS16550_H__

#include <linux/kconfig.h>

#define UART_THR(shift)		(0x0 << shift)
#define UART_RBR(shift)		(0x0 << shift)
#define UART_DLL(shift)		(0x0 << shift)
#define UART_DLM(shift)		(0x1 << shift)
#define UART_LCR(shift)		(0x3 << shift)
#define UART_LSR(shift)		(0x5 << shift)

#define UART_LCR_W	0x07	/* Set UART to 8,N,2 & DLAB = 0 */
#define UART_LCR_DLAB	0x87	/* Set UART to 8,N,2 & DLAB = 1 */

#define UART_LSR_DR	0x01    /* UART received data present */
#define UART_LSR_THRE	0x20	/* Xmit holding register empty */

#ifndef __ASSEMBLY__

#include <asm/io.h>

#define early_ns16550_putc(ch, base, shift, readx, writex) \
	do { \
		while (!(readx((u8 *)base + UART_LSR(shift)) & UART_LSR_THRE)) \
			; \
		writex(ch, (u8 *)base + UART_THR(shift)); \
	} while (0)

#define early_ns16550_init(base, divisor, shift, writex) \
	do { \
		writex(UART_LCR_DLAB, (u8 *)base + UART_LCR(shift)); \
		writex(divisor & 0xff, (u8 *)base + UART_DLL(shift)); \
		writex((divisor >> 8) & 0xff, (u8 *)base + UART_DLM(shift)); \
		writex(UART_LCR_W, (u8 *)base + UART_LCR(shift)); \
	} while (0)

#endif

#endif

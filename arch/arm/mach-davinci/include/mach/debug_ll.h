/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2014 Antony Pavlov <antonynpavlov@gmail.com> */

/** @file
 *  This File contains declaration for early output support
 */
#ifndef __INCLUDE_ARCH_DEBUG_LL_H__
#define __INCLUDE_ARCH_DEBUG_LL_H__

#include <asm/io.h>
#include <mach/serial.h>

#define DEBUG_LL_UART_ADDR	DAVINCI_UART0_BASE
#define DEBUG_LL_UART_RSHFT	2

#define rbr		(0 << DEBUG_LL_UART_RSHFT)
#define lsr		(5 << DEBUG_LL_UART_RSHFT)
#define LSR_THRE	0x20	/* Xmit holding register empty */

static inline void PUTC_LL(char ch)
{
	while (!(__raw_readb(DEBUG_LL_UART_ADDR + lsr) & LSR_THRE))
		;

	__raw_writeb(ch, DEBUG_LL_UART_ADDR + rbr);
}

#endif /* __INCLUDE_ARCH_DEBUG_LL_H__ */

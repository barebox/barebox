/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2019 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __ASM_DEBUG_LL_LITEX__
#define __ASM_DEBUG_LL_LITEX__

/** @file
 *  This File contains declaration for early output support
 */

#include <linux/kconfig.h>

#define DEBUG_LL_UART_ADDR	0xf0001000
#define UART_RXTX	0x00
#define UART_TXFULL	0x04
#define UART_RXEMPTY	0x08
#define UART_EV_PENDING	0x10
#define  UART_EV_RX	(1 << 1)


#ifndef __ASSEMBLY__

/*
 * C macros
 */

#include <asm/io.h>

static inline void PUTC_LL(char ch)
{
#ifdef CONFIG_DEBUG_LL
	/* wait for space */
	while (__raw_readb((u8 *)DEBUG_LL_UART_ADDR + UART_TXFULL))
		;

	__raw_writeb(ch, (u8 *)DEBUG_LL_UART_ADDR + UART_RXTX);
#endif /* CONFIG_DEBUG_LL */
}
#else /* __ASSEMBLY__ */
/*
 * Macros for use in assembly language code
 */

/*
 * output a character in a0
 */
.macro	debug_ll_outc_a0
#ifdef CONFIG_DEBUG_LL

	li	t0, DEBUG_LL_UART_ADDR

201:
	lbu	t1, UART_TXFULL(t0)	/* uart tx full ? */
	andi	t1, t1, 0xff
	bnez	t1, 201b		/* try again */

	sb	a0, UART_RXTX(t0)	/* write the character */

#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * output a character
 */
.macro	debug_ll_outc chr
#ifdef CONFIG_DEBUG_LL
	li	a0, \chr
	debug_ll_outc_a0
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * check character in input buffer
 * return value:
 *  v0 = 0   no character in input buffer
 *  v0 != 0  character in input buffer
 */
.macro	debug_ll_tstc
#ifdef CONFIG_DEBUG_LL
	li	t0, DEBUG_LL_UART_ADDR

	/* get line status and check for data present */
	lbu	s1, UART_RXEMPTY(t0)
	bnez    s1, 243f
	li	s1, 1
	j	244f
243:	li	s1, 0
244:	nop
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * get character to v0
 */
.macro	debug_ll_getc
#ifdef CONFIG_DEBUG_LL

204:
	debug_ll_tstc

	/* try again */
	beqz	s1, 204b

	/* read a character */
	lb	s1, UART_RXTX(t0)
	li      t1, UART_EV_RX
	sb	t1, UART_EV_PENDING(t0)

#endif /* CONFIG_DEBUG_LL */
.endm
#endif /* __ASSEMBLY__ */

#endif /* __ASM_DEBUG_LL_LITEX__ */

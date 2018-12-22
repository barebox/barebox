/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012, 2013 Antony Pavlov <antonynpavlov@gmail.com>
 */

/** @file
 *  This file contains declaration for early output support
 */
#ifndef __INCLUDE_MIPS_ASM_DEBUG_LL_NS16550_H__
#define __INCLUDE_MIPS_ASM_DEBUG_LL_NS16550_H__

#ifdef CONFIG_DEBUG_LL

#ifndef DEBUG_LL_UART_ADDR
#error DEBUG_LL_UART_ADDR is undefined!
#endif

#ifndef DEBUG_LL_UART_SHIFT
#error DEBUG_LL_UART_SHIFT is undefined!
#endif

#ifndef DEBUG_LL_UART_DIVISOR
#error DEBUG_LL_UART_DIVISOR is undefined!
#endif

#endif /* CONFIG_DEBUG_LL */

#define UART_THR	(0x0 << DEBUG_LL_UART_SHIFT)
#define UART_RBR	(0x0 << DEBUG_LL_UART_SHIFT)
#define UART_DLL	(0x0 << DEBUG_LL_UART_SHIFT)
#define UART_DLM	(0x1 << DEBUG_LL_UART_SHIFT)
#define UART_LCR	(0x3 << DEBUG_LL_UART_SHIFT)
#define UART_LSR	(0x5 << DEBUG_LL_UART_SHIFT)

#define UART_LCR_W	0x07		/* Set UART to 8,N,2 & DLAB = 0 */
#define UART_LCR_DLAB	0x87	/* Set UART to 8,N,2 & DLAB = 1 */

#define UART_LSR_DR     0x01    /* UART received data present */
#define UART_LSR_THRE	0x20	/* Xmit holding register empty */

#ifndef __ASSEMBLY__
/*
 * C macros
 */

#include <asm/io.h>

static inline void PUTC_LL(char ch)
{
#ifdef CONFIG_DEBUG_LL
	while (!(__raw_readb((u8 *)DEBUG_LL_UART_ADDR + UART_LSR) & UART_LSR_THRE))
		;
	__raw_writeb(ch, (u8 *)DEBUG_LL_UART_ADDR + UART_THR);
#endif /* CONFIG_DEBUG_LL */
}
#else /* __ASSEMBLY__ */
/*
 * Macros for use in assembly language code
 */

.macro	debug_ll_ns16550_init
#ifdef CONFIG_DEBUG_LL
	la	t0, DEBUG_LL_UART_ADDR

	li	t1, UART_LCR_DLAB		/* DLAB on */
	sb	t1, UART_LCR(t0)		/* Write it out */

	li	t1, DEBUG_LL_UART_DIVISOR
	sb	t1, UART_DLL(t0)		/* write low order byte */
	srl	t1, t1, 8
	sb	t1, UART_DLM(t0)		/* write high order byte */

	li	t1, UART_LCR_W			/* DLAB off */
	sb	t1, UART_LCR(t0)		/* Write it out */
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * output a character in a0
 */
.macro	debug_ll_outc_a0
#ifdef CONFIG_DEBUG_LL
	.set	push
	.set	reorder

	la	t0, DEBUG_LL_UART_ADDR

201:	lbu	t1, UART_LSR(t0)	/* get line status */
	andi	t1, t1, UART_LSR_THRE	/* check for transmitter empty */
	beqz	t1, 201b			/* try again */

	sb	a0, UART_THR(t0)	/* write the character */

	.set	pop
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
 * output CR + NL
 */
.macro	debug_ll_ns16550_outnl
#ifdef CONFIG_DEBUG_LL
	debug_ll_outc '\r'
	debug_ll_outc '\n'
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
	.set	push
	.set	reorder

	la      t0, DEBUG_LL_UART_ADDR

	/* get line status and check for data present */
	lbu	t1, UART_LSR(t0)
	andi	v0, t1, UART_LSR_DR

	.set	pop
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * get character to v0
 */
.macro	debug_ll_getc
#ifdef CONFIG_DEBUG_LL
	.set	push
	.set	reorder

204:
	debug_ll_tstc

	/* try again */
	beqz	v0, 204b

	/* read a character */
	lbu	v0, UART_RBR(t0)

	.set	pop
#endif /* CONFIG_DEBUG_LL */
.endm
#endif /* __ASSEMBLY__ */

#endif /* __INCLUDE_MIPS_ASM_DEBUG_LL_NS16550_H__ */

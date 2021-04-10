/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016, 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/** @file
 *  This file contains declaration for early output support
 */
#ifndef __INCLUDE_RISCV_ASM_DEBUG_LL_NS16550_H__
#define __INCLUDE_RISCV_ASM_DEBUG_LL_NS16550_H__

#include <linux/kconfig.h>

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

#if defined(DEBUG_LL_UART_IOSIZE32)
#define UART_REG_L	lw
#define UART_REG_S	sw
#define __uart_read	readl
#define __uart_write	writel
#elif defined(DEBUG_LL_UART_IOSIZE8)
#define UART_REG_L	lbu
#define UART_REG_S	sb
#define __uart_read	readb
#define __uart_write	writeb
#else
#error "Please define DEBUG_LL_UART_IOSIZE{8,32}"
#endif

#include <asm/ns16550.h>

#ifndef __ASSEMBLY__
/*
 * C macros
 */

static inline void PUTC_LL(char ch)
{
#ifdef CONFIG_DEBUG_LL
	early_ns16550_putc(ch, DEBUG_LL_UART_ADDR, DEBUG_LL_UART_SHIFT,
			   __uart_read, __uart_write);
#endif
}

static inline void debug_ll_ns16550_init(void)
{
#ifdef CONFIG_DEBUG_LL
	early_ns16550_init(DEBUG_LL_UART_ADDR, DEBUG_LL_UART_DIVISOR,
			   DEBUG_LL_UART_SHIFT, __uart_write);
#endif
}

#else /* __ASSEMBLY__ */
/*
 * Macros for use in assembly language code
 */

.macro	debug_ll_ns16550_init divisor=DEBUG_LL_UART_DIVISOR
#ifdef CONFIG_DEBUG_LL
	li	t0, DEBUG_LL_UART_ADDR

	li	t1, UART_LCR_DLAB		/* DLAB on */
	UART_REG_S	t1, UART_LCR(DEBUG_LL_UART_SHIFT)(t0)	/* Write it out */

	li	t1, \divisor
	UART_REG_S	t1, UART_DLL(DEBUG_LL_UART_SHIFT)(t0)	/* write low order byte */
	srl	t1, t1, 8
	UART_REG_S	t1, UART_DLM(DEBUG_LL_UART_SHIFT)(t0)	/* write high order byte */

	li	t1, UART_LCR_W			/* DLAB off */
	UART_REG_S	t1, UART_LCR(DEBUG_LL_UART_SHIFT)(t0)	/* Write it out */
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * output a character in a0
 */
.macro	debug_ll_outc_a0
#ifdef CONFIG_DEBUG_LL

	li	t0, DEBUG_LL_UART_ADDR

201:
	UART_REG_L	t1, UART_LSR(DEBUG_LL_UART_SHIFT)(t0)	/* get line status */
	andi	t1, t1, UART_LSR_THRE	/* check for transmitter empty */
	beqz	t1, 201b			/* try again */

	UART_REG_S	a0, UART_THR(DEBUG_LL_UART_SHIFT)(t0)	/* write the character */

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
	li      t0, DEBUG_LL_UART_ADDR

	/* get line status and check for data present */
	UART_REG_L	s0, UART_LSR(DEBUG_LL_UART_SHIFT)(t0)
	andi	s0, s0, UART_LSR_DR

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
	beqz	s0, 204b

	/* read a character */
	UART_REG_L	s0, UART_RBR(DEBUG_LL_UART_SHIFT)(t0)

#endif /* CONFIG_DEBUG_LL */
.endm
#endif /* __ASSEMBLY__ */

#endif /* __INCLUDE_RISCV_ASM_DEBUG_LL_NS16550_H__ */

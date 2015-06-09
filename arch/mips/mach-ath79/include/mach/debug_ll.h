/*
 * based on linux.git/drivers/tty/serial/ar933x_uart.c
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

#ifndef __AR933X_DEBUG_LL__
#define __AR933X_DEBUG_LL__

#include <asm/addrspace.h>

/* Alas! <mach/ar71xx_regs.h> isn't assembly-tolerant */
#define AR71XX_APB_BASE     0x18000000
#define AR933X_UART_BASE    (AR71XX_APB_BASE + 0x00020000)

#define DEBUG_LL_UART_ADDR	KSEG1ADDR(AR933X_UART_BASE)

#define AR933X_UART_DATA_REG            0x00
#define AR933X_UART_DATA_TX_RX_MASK     0xff
#define AR933X_UART_DATA_TX_CSR		0x200
#define AR933X_UART_DATA_RX_CSR		0x100

#ifndef __ASSEMBLY__

#include <io.h>

/*
 * C macros
 */

static inline void ar933x_debug_ll_writel(u32 b, int offset)
{
	__raw_writel(b, (u8 *)DEBUG_LL_UART_ADDR + offset);
}

static inline u32 ar933x_debug_ll_readl(int offset)
{
	return __raw_readl((u8 *)DEBUG_LL_UART_ADDR + offset);
}

static inline void PUTC_LL(int ch)
{
	u32 data;

	/* wait transmitter ready */
	data = ar933x_debug_ll_readl(AR933X_UART_DATA_REG);
	while (!(data & AR933X_UART_DATA_TX_CSR))
		data = ar933x_debug_ll_readl(AR933X_UART_DATA_REG);

	data = (ch & AR933X_UART_DATA_TX_RX_MASK) | AR933X_UART_DATA_TX_CSR;
	ar933x_debug_ll_writel(data, AR933X_UART_DATA_REG);
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
	.set	push
	.set	reorder

	la	t0, DEBUG_LL_UART_ADDR
201:
	lw	t1, AR933X_UART_DATA_REG(t0)	/* get line status */
	andi	t1, t1, AR933X_UART_DATA_TX_CSR	/* check for transmitter empty */
	beqz	t1, 201b	/* try again */
	andi	a0, a0, AR933X_UART_DATA_TX_RX_MASK
	ori	a0, a0, AR933X_UART_DATA_TX_CSR
	sw	a0, 0(t0)	/* write the character */
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
 * output a 32-bit value in hex
 */
.macro debug_ll_outhexw
#ifdef CONFIG_DEBUG_LL
	.set	push
	.set	reorder

	move	t6, a0
	li		t5, 32

202:
	addi	t5, t5, -4
	srlv	a0, t6, t5

	/* output one hex digit */
	andi	a0, a0, 15
	blt	a0, 10, 203f

	addi	a0, a0, ('a' - '9' - 1)

203:
	addi	a0, a0, '0'

	debug_ll_outc_a0

	bgtz	t5, 202b

	.set	pop
#endif /* CONFIG_DEBUG_LL */
.endm

/*
 * check character in input buffer
 * return value:
 *  v0 = 0   no character in input buffer
 *  v0 != 0  character in input buffer
 */
/* FIXME: use tstc */
.macro	debug_ll_tstc
#ifdef CONFIG_DEBUG_LL
	.set	push
	.set	reorder

	la	t0, DEBUG_LL_UART_ADDR

	/* get line status and check for data present */
	lw	v0, AR933X_UART_DATA_REG(t0)
	andi	v0, v0, AR933X_UART_DATA_RX_CSR

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

	la	t0, DEBUG_LL_UART_ADDR
204:
	lw	v0, AR933X_UART_DATA_REG(t0)
	andi	v0, v0, AR933X_UART_DATA_RX_CSR

	/* try again */
	beqz	v0, 204b

	/* read a character */
	lw	v0, AR933X_UART_DATA_REG(t0)
	andi	v0, v0, AR933X_UART_DATA_TX_RX_MASK

	/* remove the character from the FIFO */
	li	t1, AR933X_UART_DATA_RX_CSR
	sw  t1, AR933X_UART_DATA_REG(t0)

	.set	pop
#endif /* CONFIG_DEBUG_LL */
.endm
#endif /* __ASSEMBLY__ */

#endif /* __AR933X_DEBUG_LL__ */

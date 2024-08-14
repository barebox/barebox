/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DEBUG_LL_NS16550_H
#define __DEBUG_LL_NS16550_H

/*
 * Early debugging functions for the NS16550
 * This file needs register access functions declared as:
 *
 * uint8_t debug_ll_read_reg(void __iomem *base, int reg);
 * void debug_ll_write_reg(void __iomem *base, int reg, uint8_t val);
 */
#define NS16550_THR	0x0
#define NS16550_RBR	0x0
#define NS16550_DLL	0x0
#define NS16550_IER	0x1
#define NS16550_DLM	0x1
#define NS16550_FCR	0x2
#define NS16550_LCR	0x3
#define NS16550_MCR	0x4
#define NS16550_LSR	0x5

#define NS16550_LCR_VAL		0x3 /* 8 data, 1 stop, no parity */
#define NS16550_MCR_VAL		0x3 /* RTS/DTR */
#define NS16550_FCR_VAL		0x7 /* Clear & enable FIFOs */

#define NS16550_LSR_DR		0x01 /* UART received data present */
#define NS16550_LSR_THRE	0x20 /* Xmit holding register empty */

#define NS16550_LCR_BKSE	0x80 /* Bank select enable */

static inline void debug_ll_ns16550_putc(void __iomem *base, char ch)
{
        while (!(debug_ll_read_reg(base, NS16550_LSR) & NS16550_LSR_THRE))
                ;

        debug_ll_write_reg(base, NS16550_THR, ch);
}

static inline uint16_t debug_ll_ns16550_calc_divisor(unsigned long clk)
{
	return clk / (CONFIG_BAUDRATE * 16);
}

static inline void debug_ll_ns16550_init(void __iomem *base, uint16_t divisor)
{
	debug_ll_write_reg(base, NS16550_LCR, 0x0); /* select ier reg */
	debug_ll_write_reg(base, NS16550_IER, 0x0); /* disable interrupts */

	debug_ll_write_reg(base, NS16550_LCR, NS16550_LCR_BKSE);
	debug_ll_write_reg(base, NS16550_DLL, divisor & 0xff);
	debug_ll_write_reg(base, NS16550_DLM, (divisor >> 8) & 0xff);
	debug_ll_write_reg(base, NS16550_LCR, NS16550_LCR_VAL);
	debug_ll_write_reg(base, NS16550_MCR, NS16550_MCR_VAL);
	debug_ll_write_reg(base, NS16550_FCR, NS16550_FCR_VAL);
}

#endif

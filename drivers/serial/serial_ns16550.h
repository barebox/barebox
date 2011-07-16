/**
 * @file
 * @brief Serial NS16550 header
 *
 * FileName: drivers/serial/serial_ns16550.h
 *
 * @code struct NS16550 @endcode
 * Register definitions for NS16550 device
 */
/*
 * This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 * NS16550 Serial Port
 * originally from linux source (arch/ppc/boot/ns16550.h)
 * modified slightly to
 * have addresses as offsets from CFG_ISA_BASE
 * added a few more definitions
 * added prototypes for ns16550.c
 * reduced no of com ports to 2
 * modifications (c) Rob Taylor, Flying Pig Systems. 2000.
 *
 * added support for port on 64-bit bus
 * by Richard Danter (richard.danter@windriver.com), (C) 2005 Wind River Systems
 */

#ifndef __SERIAL_NS16550__H
#define  __SERIAL_NS16550__H

/** Register offset definitions
 * platform implementation needs to translate this
 */
#define rbr		0
#define ier		1
#define fcr		2
#define lcr		3
#define mcr		4
#define lsr		5
#define msr		6
#define scr		7
#ifdef CONFIG_DRIVER_SERIAL_NS16550_OMAP_EXTENSIONS
#define mdr1		8
#define osc_12m_sel	9
#endif

#define thr		rbr
#define iir		fcr
#define dll		rbr
#define dlm		ier

#define FCR_FIFO_EN     0x01	/* Fifo enable */
#define FCR_RXSR        0x02	/* Receiver soft reset */
#define FCR_TXSR        0x04	/* Transmitter soft reset */

#define MCR_DTR         0x01
#define MCR_RTS         0x02
#define MCR_DMA_EN      0x04
#define MCR_TX_DFR      0x08

#define LCR_WLS_MSK	0x03	/* character length select mask */
#define LCR_WLS_5	0x00	/* 5 bit character length */
#define LCR_WLS_6	0x01	/* 6 bit character length */
#define LCR_WLS_7	0x02	/* 7 bit character length */
#define LCR_WLS_8	0x03	/* 8 bit character length */
/* Number of stop Bits, off = 1, on = 1.5 or 2) */
#define LCR_STB		0x04
#define LCR_PEN		0x08	/* Parity eneble */
#define LCR_EPS		0x10	/* Even Parity Select */
#define LCR_STKP	0x20	/* Stick Parity */
#define LCR_SBRK	0x40	/* Set Break */
#define LCR_BKSE	0x80	/* Bank select enable */

#define LSR_DR		0x01	/* Data ready */
#define LSR_OE		0x02	/* Overrun */
#define LSR_PE		0x04	/* Parity error */
#define LSR_FE		0x08	/* Framing error */
#define LSR_BI		0x10	/* Break */
#define LSR_THRE	0x20	/* Xmit holding register empty */
#define LSR_TEMT	0x40	/* Xmitter empty */
#define LSR_ERR		0x80	/* Error */

/* useful defaults for LCR */
#define LCR_8N1		0x03

#define LCRVAL		LCR_8N1	/* 8 data, 1 stop, no parity */
#define MCRVAL		(MCR_DTR | MCR_RTS) /* RTS/DTR */
/* Clear & enable FIFOs */
#define FCRVAL		(FCR_FIFO_EN | FCR_RXSR | FCR_TXSR)

#define MODE_X_DIV	16

#endif				/*  __SERIAL_NS16550__H */

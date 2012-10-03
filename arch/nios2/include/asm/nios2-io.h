/*
 * (C) Copyright 2004, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*************************************************************************
 * Altera Nios2 Standard Peripherals
 ************************************************************************/

#ifndef __NIOS2IO_H__
#define __NIOS2IO_H__

/*------------------------------------------------------------------------
 * UART (http://www.altera.com/literature/ds/ds_nios_uart.pdf)
 *----------------------------------------------------------------------*/
struct nios_uart {
	unsigned	rxdata;		/* Rx data reg */
	unsigned	txdata;		/* Tx data reg */
	unsigned	status;		/* Status reg */
	unsigned	control;	/* Control reg */
	unsigned	divisor;	/* Baud rate divisor reg */
	unsigned	endofpacket;	/* End-of-packet reg */
};

/* status register */
#define NIOS_UART_PE		(1 << 0)	/* parity error */
#define NIOS_UART_FE		(1 << 1)	/* frame error */
#define NIOS_UART_BRK		(1 << 2)	/* break detect */
#define NIOS_UART_ROE		(1 << 3)	/* rx overrun */
#define NIOS_UART_TOE		(1 << 4)	/* tx overrun */
#define NIOS_UART_TMT		(1 << 5)	/* tx empty */
#define NIOS_UART_TRDY		(1 << 6)	/* tx ready */
#define NIOS_UART_RRDY		(1 << 7)	/* rx ready */
#define NIOS_UART_E		(1 << 8)	/* exception */
#define NIOS_UART_DCTS		(1 << 10)	/* cts change */
#define NIOS_UART_CTS		(1 << 11)	/* cts */
#define NIOS_UART_EOP		(1 << 12)	/* eop detected */

/* control register */
#define NIOS_UART_IPE		(1 << 0)	/* parity error int ena*/
#define NIOS_UART_IFE		(1 << 1)	/* frame error int ena */
#define NIOS_UART_IBRK		(1 << 2)	/* break detect int ena */
#define NIOS_UART_IROE		(1 << 3)	/* rx overrun int ena */
#define NIOS_UART_ITOE		(1 << 4)	/* tx overrun int ena */
#define NIOS_UART_ITMT		(1 << 5)	/* tx empty int ena */
#define NIOS_UART_ITRDY		(1 << 6)	/* tx ready int ena */
#define NIOS_UART_IRRDY		(1 << 7)	/* rx ready int ena */
#define NIOS_UART_IE		(1 << 8)	/* exception int ena */
#define NIOS_UART_TBRK		(1 << 9)	/* transmit break */
#define NIOS_UART_IDCTS		(1 << 10)	/* cts change int ena */
#define NIOS_UART_RTS		(1 << 11)	/* rts */
#define NIOS_UART_IEOP		(1 << 12)	/* eop detected int ena */


/*------------------------------------------------------------------------
 * TIMER (http://www.altera.com/literature/ds/ds_nios_timer.pdf)
 *----------------------------------------------------------------------*/
struct nios_timer {
	unsigned	status;			/* Timer status reg */
	unsigned	control;		/* Timer control reg */
	unsigned	periodl;		/* Timeout period low */
	unsigned	periodh;		/* Timeout period high */
	unsigned	snapl;			/* Snapshot low */
	unsigned	snaph;			/* Snapshot high */
};

struct nios_timer_64 {
	unsigned	status;			/* Timer status reg */
	unsigned	control;		/* Timer control reg */
	unsigned	period0;		/* Timeout period low */
	unsigned	period1;		/* Timeout period high */
	unsigned	period2;		/* Timeout period low */
	unsigned	period3;		/* Timeout period high */
	unsigned	snap0;			/* Snapshot low */
	unsigned	snap1;			/* Snapshot high */
	unsigned	snap2;			/* Snapshot low */
	unsigned	snap3;			/* Snapshot high */
};

/* status register */
#define NIOS_TIMER_TO		(1 << 0)	/* Timeout */
#define NIOS_TIMER_RUN		(1 << 1)	/* Timer running */

/* control register */
#define NIOS_TIMER_ITO		(1 << 0)	/* Timeout int ena */
#define NIOS_TIMER_CONT		(1 << 1)	/* Continuous mode */
#define NIOS_TIMER_START	(1 << 2)	/* Start timer */
#define NIOS_TIMER_STOP		(1 << 3)	/* Stop timer */


/*------------------------------------------------------------------------
 * PIO (http://www.altera.com/literature/ds/ds_nios_pio.pdf)
 *----------------------------------------------------------------------*/
struct nios_pio {
	unsigned int	data;		/* Data value at each PIO in/out */
	unsigned int	direction;	/* Data direct. for each PIO bit */
	unsigned int	interruptmask;	/* Per-bit IRQ enable/disable */
	unsigned int	edgecapture;	/* Per-bit sync. edge detect & hold */
};

/* direction register */
#define NIOS_PIO_OUT		(1)		/* PIO bit is output */
#define NIOS_PIO_IN		(0)		/* PIO bit is input */


/*------------------------------------------------------------------------
 * SPI (http://www.altera.com/literature/ds/ds_nios_spi.pdf)
 *----------------------------------------------------------------------*/
struct nios_spi {
	unsigned	rxdata;		/* Rx data reg */
	unsigned	txdata;		/* Tx data reg */
	unsigned	status;		/* Status reg */
	unsigned	control;	/* Control reg */
	unsigned	reserved;	/* (master only) */
	unsigned	slaveselect;	/* SPI slave select mask (master only) */
};

/* status register */
#define NIOS_SPI_ROE		(1 << 3)	/* rx overrun */
#define NIOS_SPI_TOE		(1 << 4)	/* tx overrun */
#define NIOS_SPI_TMT		(1 << 5)	/* tx empty */
#define NIOS_SPI_TRDY		(1 << 6)	/* tx ready */
#define NIOS_SPI_RRDY		(1 << 7)	/* rx ready */
#define NIOS_SPI_E		(1 << 8)	/* exception */

/* control register */
#define NIOS_SPI_IROE		(1 << 3)	/* rx overrun int ena */
#define NIOS_SPI_ITOE		(1 << 4)	/* tx overrun int ena */
#define NIOS_SPI_ITRDY		(1 << 6)	/* tx ready int ena */
#define NIOS_SPI_IRRDY		(1 << 7)	/* rx ready int ena */
#define NIOS_SPI_IE		(1 << 8)	/* exception int ena */
#define NIOS_SPI_SSO		(1 << 10)	/* override SS_n output */

/*------------------------------------------------------------------------
 * JTAG UART
 *----------------------------------------------------------------------*/
struct nios_jtag {
	unsigned	data;			/* Data register */
	unsigned	control;		/* Control register */
};

/* data register */
#define NIOS_JTAG_RVALID	(1<<15)		/* Read valid */
#define NIOS_JTAG_DATA(d)	((d)&0x0ff)	/* Read data */
#define NIOS_JTAG_RAVAIL(d)	((d)>>16)	/* Read space avail */

/* control register */
#define NIOS_JTAG_RE		(1 << 0)	/* read intr enable */
#define NIOS_JTAG_WE		(1 << 1)	/* write intr enable */
#define NIOS_JTAG_RI		(1 << 8)	/* read intr pending */
#define NIOS_JTAG_WI		(1 << 9)	/* write intr pending*/
#define NIOS_JTAG_AC		(1 << 10)	/* activity indicator */
#define NIOS_JTAG_RRDY		(1 << 12)	/* read available */
#define NIOS_JTAG_WSPACE(d)	((d)>>16)	/* Write space avail */

/*------------------------------------------------------------------------
 * SYSTEM ID
 *----------------------------------------------------------------------*/
struct nios_sysid {
	unsigned	id;			/* The system build id*/
	unsigned	timestamp;		/* Timestamp */
};

#endif /* __NIOS2IO_H__ */

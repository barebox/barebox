/*
 * Copyright (c) 2016 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 *
 * Based on code found in Linux kernel and U-Boot.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#ifndef __LPUART_H__
#define __LPUART_H__


/* All registers are 8-bit width */
#define UARTBDH			0x00
#define UARTBDL			0x01
#define UARTCR1			0x02
#define UARTCR2			0x03
#define UARTSR1			0x04
#define UARTCR3			0x06
#define UARTDR			0x07
#define UARTCR4			0x0a
#define UARTCR5			0x0b
#define UARTMODEM		0x0d
#define UARTPFIFO		0x10
#define UARTCFIFO		0x11
#define UARTSFIFO		0x12
#define UARTTWFIFO		0x13
#define UARTTCFIFO		0x14
#define UARTRWFIFO		0x15
#define UARTRCFIFO		0x16

#define UARTBDH_LBKDIE		0x80
#define UARTBDH_RXEDGIE		0x40
#define UARTBDH_SBR_MASK	0x1f

#define UARTCR1_LOOPS		0x80
#define UARTCR1_RSRC		0x20
#define UARTCR1_M		0x10
#define UARTCR1_WAKE		0x08
#define UARTCR1_ILT		0x04
#define UARTCR1_PE		0x02
#define UARTCR1_PT		0x01

#define UARTCR2_TIE		0x80
#define UARTCR2_TCIE		0x40
#define UARTCR2_RIE		0x20
#define UARTCR2_ILIE		0x10
#define UARTCR2_TE		0x08
#define UARTCR2_RE		0x04
#define UARTCR2_RWU		0x02
#define UARTCR2_SBK		0x01

#define UARTSR1_TDRE		0x80
#define UARTSR1_TC		0x40
#define UARTSR1_RDRF		0x20
#define UARTSR1_IDLE		0x10
#define UARTSR1_OR		0x08
#define UARTSR1_NF		0x04
#define UARTSR1_FE		0x02
#define UARTSR1_PE		0x01

#define UARTCR3_R8		0x80
#define UARTCR3_T8		0x40
#define UARTCR3_TXDIR		0x20
#define UARTCR3_TXINV		0x10
#define UARTCR3_ORIE		0x08
#define UARTCR3_NEIE		0x04
#define UARTCR3_FEIE		0x02
#define UARTCR3_PEIE		0x01

#define UARTCR4_MAEN1		0x80
#define UARTCR4_MAEN2		0x40
#define UARTCR4_M10		0x20
#define UARTCR4_BRFA_MASK	0x1f
#define UARTCR4_BRFA_OFF	0

#define UARTCR5_TDMAS		0x80
#define UARTCR5_RDMAS		0x20

#define UARTMODEM_RXRTSE	0x08
#define UARTMODEM_TXRTSPOL	0x04
#define UARTMODEM_TXRTSE	0x02
#define UARTMODEM_TXCTSE	0x01

#define UARTPFIFO_TXFE		0x80
#define UARTPFIFO_FIFOSIZE_MASK	0x7
#define UARTPFIFO_TXSIZE_OFF	4
#define UARTPFIFO_RXFE		0x08
#define UARTPFIFO_RXSIZE_OFF	0

#define UARTCFIFO_TXFLUSH	0x80
#define UARTCFIFO_RXFLUSH	0x40
#define UARTCFIFO_RXOFE		0x04
#define UARTCFIFO_TXOFE		0x02
#define UARTCFIFO_RXUFE		0x01

#define UARTSFIFO_TXEMPT	0x80
#define UARTSFIFO_RXEMPT	0x40
#define UARTSFIFO_RXOF		0x04
#define UARTSFIFO_TXOF		0x02
#define UARTSFIFO_RXUF		0x01

/* 32-bit register defination */
#define UARTBAUD		0x00
#define UARTSTAT		0x04
#define UARTCTRL		0x08
#define UARTDATA		0x0C
#define UARTMATCH		0x10
#define UARTMODIR		0x14
#define UARTFIFO		0x18
#define UARTWATER		0x1c

#define UARTBAUD_MAEN1		0x80000000
#define UARTBAUD_MAEN2		0x40000000
#define UARTBAUD_M10		0x20000000
#define UARTBAUD_TDMAE		0x00800000
#define UARTBAUD_RDMAE		0x00200000
#define UARTBAUD_MATCFG		0x00400000
#define UARTBAUD_BOTHEDGE	0x00020000
#define UARTBAUD_RESYNCDIS	0x00010000
#define UARTBAUD_LBKDIE		0x00008000
#define UARTBAUD_RXEDGIE	0x00004000
#define UARTBAUD_SBNS		0x00002000
#define UARTBAUD_SBR		0x00000000
#define UARTBAUD_SBR_MASK	0x1fff

#define UARTSTAT_LBKDIF		0x80000000
#define UARTSTAT_RXEDGIF	0x40000000
#define UARTSTAT_MSBF		0x20000000
#define UARTSTAT_RXINV		0x10000000
#define UARTSTAT_RWUID		0x08000000
#define UARTSTAT_BRK13		0x04000000
#define UARTSTAT_LBKDE		0x02000000
#define UARTSTAT_RAF		0x01000000
#define UARTSTAT_TDRE		0x00800000
#define UARTSTAT_TC		0x00400000
#define UARTSTAT_RDRF		0x00200000
#define UARTSTAT_IDLE		0x00100000
#define UARTSTAT_OR		0x00080000
#define UARTSTAT_NF		0x00040000
#define UARTSTAT_FE		0x00020000
#define UARTSTAT_PE		0x00010000
#define UARTSTAT_MA1F		0x00008000
#define UARTSTAT_M21F		0x00004000

#define UARTCTRL_R8T9		0x80000000
#define UARTCTRL_R9T8		0x40000000
#define UARTCTRL_TXDIR		0x20000000
#define UARTCTRL_TXINV		0x10000000
#define UARTCTRL_ORIE		0x08000000
#define UARTCTRL_NEIE		0x04000000
#define UARTCTRL_FEIE		0x02000000
#define UARTCTRL_PEIE		0x01000000
#define UARTCTRL_TIE		0x00800000
#define UARTCTRL_TCIE		0x00400000
#define UARTCTRL_RIE		0x00200000
#define UARTCTRL_ILIE		0x00100000
#define UARTCTRL_TE		0x00080000
#define UARTCTRL_RE		0x00040000
#define UARTCTRL_RWU		0x00020000
#define UARTCTRL_SBK		0x00010000
#define UARTCTRL_MA1IE		0x00008000
#define UARTCTRL_MA2IE		0x00004000
#define UARTCTRL_IDLECFG	0x00000100
#define UARTCTRL_LOOPS		0x00000080
#define UARTCTRL_DOZEEN		0x00000040
#define UARTCTRL_RSRC		0x00000020
#define UARTCTRL_M		0x00000010
#define UARTCTRL_WAKE		0x00000008
#define UARTCTRL_ILT		0x00000004
#define UARTCTRL_PE		0x00000002
#define UARTCTRL_PT		0x00000001

#define UARTDATA_NOISY		0x00008000
#define UARTDATA_PARITYE	0x00004000
#define UARTDATA_FRETSC		0x00002000
#define UARTDATA_RXEMPT		0x00001000
#define UARTDATA_IDLINE		0x00000800
#define UARTDATA_MASK		0x3ff

#define UARTMODIR_IREN		0x00020000
#define UARTMODIR_TXCTSSRC	0x00000020
#define UARTMODIR_TXCTSC	0x00000010
#define UARTMODIR_RXRTSE	0x00000008
#define UARTMODIR_TXRTSPOL	0x00000004
#define UARTMODIR_TXRTSE	0x00000002
#define UARTMODIR_TXCTSE	0x00000001

#define UARTFIFO_TXEMPT		0x00800000
#define UARTFIFO_RXEMPT		0x00400000
#define UARTFIFO_TXOF		0x00020000
#define UARTFIFO_RXUF		0x00010000
#define UARTFIFO_TXFLUSH	0x00008000
#define UARTFIFO_RXFLUSH	0x00004000
#define UARTFIFO_TXOFE		0x00000200
#define UARTFIFO_RXUFE		0x00000100
#define UARTFIFO_TXFE		0x00000080
#define UARTFIFO_FIFOSIZE_MASK	0x7
#define UARTFIFO_TXSIZE_OFF	4
#define UARTFIFO_RXFE		0x00000008
#define UARTFIFO_RXSIZE_OFF	0

#define UARTWATER_COUNT_MASK	0xff
#define UARTWATER_TXCNT_OFF	8
#define UARTWATER_RXCNT_OFF	24
#define UARTWATER_WATER_MASK	0xff
#define UARTWATER_TXWATER_OFF	0
#define UARTWATER_RXWATER_OFF	16

#define FSL_UART_RX_DMA_BUFFER_SIZE	64

static inline void lpuart_setbrg(void __iomem *base,
				 unsigned int refclock,
				 unsigned int baudrate)
{
	unsigned int bfra;
	u16 sbr;

	BUG_ON(!baudrate);

	sbr = (u16) (refclock / (16 * baudrate));

	writeb(sbr >> 8,   base + UARTBDH);
	writeb(sbr & 0xff, base + UARTBDL);

	bfra  = DIV_ROUND_UP(2 * refclock, baudrate) - 32 * sbr;
	bfra &= UARTCR4_BRFA_MASK;
	writeb(bfra, base + UARTCR4);
}

static inline void lpuart_setup(void __iomem *base,
				unsigned int refclock)
{
	/* Disable UART */
	writeb(0, base + UARTCR2);
	writeb(0, base + UARTMODEM);
	writeb(0, base + UARTCR1);

	writeb(0, base + UARTPFIFO);
	writeb(0, base + UARTTWFIFO);

	writeb(1, base + UARTRWFIFO);
	writeb(UARTCFIFO_RXFLUSH | UARTCFIFO_TXFLUSH, base + UARTCFIFO);

	lpuart_setbrg(base, refclock, CONFIG_BAUDRATE);

	writeb(UARTCR2_TE | UARTCR2_RE, base + UARTCR2);
}

static inline void lpuart_putc(void __iomem *base, int c)
{
	if (!(readb(base + UARTCR2) & UARTCR2_TE))
		return;

	while (!(readb(base + UARTSR1) & UARTSR1_TDRE));

	writeb(c, base + UARTDR);
}

#endif

/*
 * (c) 2009 Sascha Hauer <s.hauer@pengutronix.de>
 *     2010 by Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>

#include <mach/clock.h>
#include <asm/io.h>

#define RBR		0x00	/* Receive Buffer Register (read only) */
#define THR		0x00	/* Transmit Holding Register (write only) */
#define IER		0x04	/* Interrupt Enable Register (read/write) */
#define IIR		0x08	/* Interrupt ID Register (read only) */
#define FCR		0x08	/* FIFO Control Register (write only) */
#define LCR		0x0c	/* Line Control Register (read/write) */
#define MCR		0x10	/* Modem Control Register (read/write) */
#define LSR		0x14	/* Line Status Register (read only) */
#define MSR		0x18	/* Modem Status Register (read only) */
#define SPR		0x1c	/* Scratch Pad Register (read/write) */
#define ISR		0x20	/* Infrared Selection Register (read/write) */
#define DLL		0x00	/* Divisor Latch Low Register (DLAB = 1) (read/write) */
#define DLH		0x04	/* Divisor Latch High Register (DLAB = 1) (read/write) */

#define IER_DMAE	(1 << 7)	/* DMA Requests Enable */
#define IER_UUE		(1 << 6)	/* UART Unit Enable */
#define IER_NRZE	(1 << 5)	/* NRZ coding Enable */
#define IER_RTIOE	(1 << 4)	/* Receiver Time Out Interrupt Enable */
#define IER_MIE		(1 << 3)	/* Modem Interrupt Enable */
#define IER_RLSE	(1 << 2)	/* Receiver Line Status Interrupt Enable */
#define IER_TIE		(1 << 1)	/* Transmit Data request Interrupt Enable */
#define IER_RAVIE	(1 << 0)	/* Receiver Data Available Interrupt Enable */

#define IIR_FIFOES1	(1 << 7)	/* FIFO Mode Enable Status */
#define IIR_FIFOES0	(1 << 6)	/* FIFO Mode Enable Status */
#define IIR_TOD		(1 << 3)	/* Time Out Detected */
#define IIR_IID2	(1 << 2)	/* Interrupt Source Encoded */
#define IIR_IID1	(1 << 1)	/* Interrupt Source Encoded */
#define IIR_IP		(1 << 0)	/* Interrupt Pending (active low) */

#define FCR_ITL2	(1 << 7)	/* Interrupt Trigger Level */
#define FCR_ITL1	(1 << 6)	/* Interrupt Trigger Level */
#define FCR_RESETTF	(1 << 2)	/* Reset Transmitter FIFO */
#define FCR_RESETRF	(1 << 1)	/* Reset Receiver FIFO */
#define FCR_TRFIFOE	(1 << 0)	/* Transmit and Receive FIFO Enable */
#define FCR_ITL_1	(0)
#define FCR_ITL_8	(FCR_ITL1)
#define FCR_ITL_16	(FCR_ITL2)
#define FCR_ITL_32	(FCR_ITL2 | F CR_ITL1)

#define LCR_DLAB	(1 << 7)	/* Divisor Latch Access Bit */
#define LCR_SB		(1 << 6)	/* Set Break */
#define LCR_STKYP	(1 << 5)	/* Sticky Parity */
#define LCR_EPS		(1 << 4)	/* Even Parity Select */
#define LCR_PEN		(1 << 3)	/* Parity Enable */
#define LCR_STB		(1 << 2)	/* Stop Bit */
#define LCR_WLS1	(1 << 1)	/* Word Length Select */
#define LCR_WLS0	(1 << 0)	/* Word Length Select */
#define LCR_WLEN8	(LCR_WLS1 | LCR_WLS0)
					/* Wordlength: 8 bits */

#define LSR_FIFOE	(1 << 7)	/* FIFO Error Status */
#define LSR_TEMT	(1 << 6)	/* Transmitter Empty */
#define LSR_TDRQ	(1 << 5)	/* Transmit Data Request */
#define LSR_BI		(1 << 4)	/* Break Interrupt */
#define LSR_FE		(1 << 3)	/* Framing Error */
#define LSR_PE		(1 << 2)	/* Parity Error */
#define LSR_OE		(1 << 1)	/* Overrun Error */
#define LSR_DR		(1 << 0)	/* Data Ready */

#define MCR_LOOP	(1 << 4)	/* */
#define MCR_OUT2	(1 << 3)	/* force MSR_DCD in loopback mode */
#define MCR_OUT1	(1 << 2)	/* force MSR_RI in loopback mode */
#define MCR_RTS		(1 << 1)	/* Request to Send */
#define MCR_DTR		(1 << 0)	/* Data Terminal Ready */

#define MSR_DCD		(1 << 7)	/* Data Carrier Detect */
#define MSR_RI		(1 << 6)	/* Ring Indicator */
#define MSR_DSR		(1 << 5)	/* Data Set Ready */
#define MSR_CTS		(1 << 4)	/* Clear To Send */
#define MSR_DDCD	(1 << 3)	/* Delta Data Carrier Detect */
#define MSR_TERI	(1 << 2)	/* Trailing Edge Ring Indicator */
#define MSR_DDSR	(1 << 1)	/* Delta Data Set Ready */
#define MSR_DCTS	(1 << 0)	/* Delta Clear To Send */

struct pxa_serial_priv {
	void __iomem *regs;
	struct console_device cdev;
};

static void __iomem *to_regs(struct console_device *cdev)
{
	struct pxa_serial_priv *priv =
		container_of(cdev, struct pxa_serial_priv, cdev);
	return priv->regs;
}

static void pxa_serial_putc(struct console_device *cdev, char c)
{
	while (!(readl(to_regs(cdev) + LSR) & LSR_TEMT));

	writel(c, to_regs(cdev) + THR);
}

static int pxa_serial_tstc(struct console_device *cdev)
{
	return readl(to_regs(cdev) + LSR) & LSR_DR;
}

static int pxa_serial_getc(struct console_device *cdev)
{
	while (!(readl(to_regs(cdev) + LSR) & LSR_DR));

	return readl(to_regs(cdev) + RBR) & 0xff;
}

static void pxa_serial_flush(struct console_device *cdev)
{
}

static int pxa_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	unsigned char cval = LCR_WLEN8;			/* 8N1 */
	unsigned int quot;

	/* enable uart */
	writel(IER_UUE, to_regs(cdev) + IER);

	/* write divisor */
	quot = (pxa_get_uartclk() + (8 * baudrate)) / (16 * baudrate);

	writel(cval | LCR_DLAB, to_regs(cdev) + LCR);	/* set DLAB */
	writel(quot & 0xff, to_regs(cdev) + DLL);
	/*
	 * work around Errata #75 according to Intel(R) PXA27x
	 * Processor Family Specification Update (Nov 2005)
	 */
	readl(to_regs(cdev) + DLL);
	writel(quot >> 8, to_regs(cdev) + DLH);
	writel(cval, to_regs(cdev) + LCR);		/* reset DLAB */

	/* enable fifos */
	writel(FCR_TRFIFOE, to_regs(cdev) + FCR);

	return 0;
}

static int pxa_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct console_device *cdev;
	struct pxa_serial_priv *priv;

	priv = xzalloc(sizeof(*priv));
	cdev = &priv->cdev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);

	dev->priv = priv;
	cdev->dev = dev;
	cdev->tstc = pxa_serial_tstc;
	cdev->putc = pxa_serial_putc;
	cdev->getc = pxa_serial_getc;
	cdev->flush = pxa_serial_flush;
	cdev->setbrg = pxa_serial_setbaudrate;

	console_register(cdev);

	return 0;
}

static struct driver_d pxa_serial_driver = {
	.name = "pxa_serial",
	.probe = pxa_serial_probe,
};
console_platform_driver(pxa_serial_driver);

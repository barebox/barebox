/*
 * (C) Copyright 2005
 * Sascha Hauer, Pengutronix <s.hauer@pengutronix.de>
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

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <asm/blackfin.h>

#define UART_IER_ERBFI		0x01
#define UART_IER_ETBEI		0x02
#define UART_IER_ELSI		0x04
#define UART_IER_EDDSI		0x08

#define UART_IIR_NOINT		0x01
#define UART_IIR_STATUS		0x06
#define UART_IIR_LSR		0x06
#define UART_IIR_RBR		0x04
#define UART_IIR_THR		0x02
#define UART_IIR_MSR		0x00

#define UART_LCR_WLS5		0
#define UART_LCR_WLS6		0x01
#define UART_LCR_WLS7		0x02
#define UART_LCR_WLS8		0x03
#define UART_LCR_STB		0x04
#define UART_LCR_PEN		0x08
#define UART_LCR_EPS		0x10
#define UART_LCR_SP		0x20
#define UART_LCR_SB		0x40
#define UART_LCR_DLAB		0x80

#define UART_LSR_DR		0x01
#define UART_LSR_OE		0x02
#define UART_LSR_PE		0x04
#define UART_LSR_FE		0x08
#define UART_LSR_BI		0x10
#define UART_LSR_THRE		0x20
#define UART_LSR_TEMT		0x40

#define UART_GCTL_UCEN		0x01

static int blackfin_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	int divisor, oldlcr;

	oldlcr = readw(UART_LCR);

	divisor = (get_sclk() + (baudrate * 0)) / (baudrate * 16);

	/* Set DLAB in LCR to Access DLL and DLH */
	writew(UART_LCR_DLAB, UART_LCR);

	writew(divisor & 0xff, UART_DLL);
	writew((divisor >> 8) & 0xff, UART_DLH);

	/* Clear DLAB in LCR to Access THR RBR IER */
	writew(oldlcr, UART_LCR);

	return 0;
}

static int blackfin_serial_init_port(struct console_device *cdev)
{
	/* Enable UART */
	writew(UART_GCTL_UCEN, UART_GCTL);

	/* Set LCR to Word Lengh 8-bit word select */
	writew(UART_LCR_WLS8, UART_LCR);

	return 0;
}

static void blackfin_serial_putc(struct console_device *cdev, char c)
{
	while (!(readw(UART_LSR) & UART_LSR_TEMT));

	writew(c, UART_THR);
}

static int blackfin_serial_getc(struct console_device *cdev)
{
	while (!(readw(UART_LSR) & UART_LSR_DR));

	return readw(UART_RBR);
}

static int blackfin_serial_tstc(struct console_device *cdev)
{
	return (readw(UART_LSR) & UART_LSR_DR) ? 1 : 0;
}

static int blackfin_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	cdev->dev = dev;
	cdev->tstc = blackfin_serial_tstc;
	cdev->putc = blackfin_serial_putc;
	cdev->getc = blackfin_serial_getc;
	cdev->setbrg = blackfin_serial_setbaudrate;

	blackfin_serial_init_port(cdev);

	console_register(cdev);

	return 0;
}

static struct driver_d blackfin_serial_driver = {
        .name  = "blackfin_serial",
        .probe = blackfin_serial_probe,
};
console_platform_driver(blackfin_serial_driver);

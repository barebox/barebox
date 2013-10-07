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
#include <mach/netx-regs.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>

enum uart_regs {
	UART_DR              = 0x00,
	UART_SR              = 0x04,
	UART_LINE_CR         = 0x08,
	UART_BAUDDIV_MSB     = 0x0c,
	UART_BAUDDIV_LSB     = 0x10,
	UART_CR              = 0x14,
	UART_FR              = 0x18,
	UART_IIR             = 0x1c,
	UART_ILPR            = 0x20,
	UART_RTS_CR          = 0x24,
	UART_RTS_LEAD        = 0x28,
	UART_RTS_TRAIL       = 0x2c,
	UART_DRV_ENABLE      = 0x30,
	UART_BRM_CR          = 0x34,
	UART_RXFIFO_IRQLEVEL = 0x38,
	UART_TXFIFO_IRQLEVEL = 0x3c,
};

#define LINE_CR_5BIT	(0<<5)
#define LINE_CR_6BIT    (1<<5)
#define LINE_CR_7BIT    (2<<5)
#define LINE_CR_8BIT    (3<<5)
#define LINE_CR_FEN	(1<<4)

#define CR_UARTEN	(1<<0)

#define FR_TXFE (1<<7)
#define FR_RXFF (1<<6)
#define FR_TXFF (1<<5)
#define FR_RXFE (1<<4)
#define FR_BUSY (1<<3)
#define FR_DCD  (1<<2)
#define FR_DSR  (1<<1)
#define FR_CTS  (1<<0)

#define DRV_ENABLE_TX  (1<<1)
#define DRV_ENABLE_RTS (1<<0)

#define BRM_CR_BAUD_RATE_MODE (1<<0)

static int netx_serial_init_port(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	void __iomem *base = dev->priv;
	unsigned int divisor;

	/* disable uart */
	writel(0, base + UART_CR);
	writel(BRM_CR_BAUD_RATE_MODE, base + UART_BRM_CR);

	/* set baud rate */
	divisor = 115200 * 4096;
	divisor /= 1000;
	divisor *= 256;
	divisor /= 100000;

	writel(divisor & 0xff, base + UART_BAUDDIV_LSB);
	writel((divisor >> 8) & 0xff, base + UART_BAUDDIV_MSB);
	writel(DRV_ENABLE_TX | DRV_ENABLE_RTS, base + UART_DRV_ENABLE);

	writel(LINE_CR_8BIT | LINE_CR_FEN, base + UART_LINE_CR);

	/* Finally, enable the UART */
	writel(CR_UARTEN, base + UART_CR);

	return 0;
}

static int netx_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	return 0;
}

static void netx_serial_putc(struct console_device *cdev, char c)
{
	struct device_d *dev = cdev->dev;
	void __iomem *base = dev->priv;

	while (readl(base + UART_FR) & FR_TXFF );

	writel(c, base + UART_DR);
}

static int netx_serial_getc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	void __iomem *base = dev->priv;
	int c;

	while (readl(base + UART_FR) & FR_RXFE );

	c = readl(base + UART_DR);

	readl(base + UART_SR);

	return c;
}

static int netx_serial_tstc(struct console_device *cdev)
{
	struct device_d *dev = cdev->dev;
	void __iomem *base = dev->priv;

	return (readl(base + UART_FR) & FR_RXFE) ? 0 : 1;
}

static int netx_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	dev->priv = dev_request_mem_region(dev, 0);
	cdev->dev = dev;
	cdev->tstc = netx_serial_tstc;
	cdev->putc = netx_serial_putc;
	cdev->getc = netx_serial_getc;
	cdev->setbrg = netx_serial_setbaudrate;

	netx_serial_init_port(cdev);

	console_register(cdev);

	return 0;
}

static struct driver_d netx_serial_driver = {
        .name  = "netx_serial",
        .probe = netx_serial_probe,
};
console_platform_driver(netx_serial_driver);

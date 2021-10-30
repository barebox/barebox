// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2011, Franck JULLIEN, <elec4fun@gmail.com>
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <asm/nios2-io.h>

struct altera_serial_priv {
	struct console_device cdev;
	void __iomem *regs;
};

static int altera_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct altera_serial_priv *priv = container_of(cdev,
		struct altera_serial_priv, cdev);

	struct nios_uart *uart = priv->regs;
	uint16_t div;

	div = (CPU_FREQ / baudrate) - 1;
	writew(div, &uart->divisor);

	return 0;
}

static void altera_serial_putc(struct console_device *cdev, char c)
{
	struct altera_serial_priv *priv = container_of(cdev,
		struct altera_serial_priv, cdev);

	struct nios_uart *uart = priv->regs;

	while ((readw(&uart->status) & NIOS_UART_TRDY) == 0);

	writew(c, &uart->txdata);
}

static int altera_serial_tstc(struct console_device *cdev)
{
	struct altera_serial_priv *priv = container_of(cdev,
		struct altera_serial_priv, cdev);

	struct nios_uart *uart = priv->regs;

	return readw(&uart->status) & NIOS_UART_RRDY;
}

static int altera_serial_getc(struct console_device *cdev)
{
	struct altera_serial_priv *priv = container_of(cdev,
		struct altera_serial_priv, cdev);

	struct nios_uart *uart = priv->regs;

	while (altera_serial_tstc(cdev) == 0);

	return readw(&uart->rxdata) & 0x000000FF;
}

static int altera_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct console_device *cdev;
	struct altera_serial_priv *priv;

	priv = xzalloc(sizeof(*priv));
	cdev = &priv->cdev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	cdev->dev = dev;
	cdev->tstc = altera_serial_tstc;
	cdev->putc = altera_serial_putc;
	cdev->getc = altera_serial_getc;
	cdev->setbrg = altera_serial_setbaudrate;

	console_register(cdev);

	return 0;
}

static struct driver_d altera_serial_driver = {
	.name = "altera_serial",
	.probe = altera_serial_probe,
};
console_platform_driver(altera_serial_driver);

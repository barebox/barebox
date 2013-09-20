/*
 * (C) Copyright 2004, Psyent Corporation <www.psyent.com>
 * Scott McNutt <smcnutt@psyent.com>
 *
 * (C) Copyright 2011 - Franck JULLIEN <elec4fun@gmail.com>
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
#include <asm/nios2-io.h>

struct altera_serial_jtag_priv {
	struct console_device cdev;
	void __iomem *regs;
};


static int altera_serial_jtag_setbaudrate(struct console_device *cdev, int baudrate)
{
	return 0;
}

static void altera_serial_jtag_putc(struct console_device *cdev, char c)
{
	struct altera_serial_jtag_priv *priv = container_of(cdev,
		struct altera_serial_jtag_priv, cdev);

	struct nios_jtag *jtag = priv->regs;
	uint32_t st;

	while (1) {
		st = readl(&jtag->control);
		if (NIOS_JTAG_WSPACE(st))
			break;
	}

	writel(c, &jtag->data);
}

static int altera_serial_jtag_tstc(struct console_device *cdev)
{
	struct altera_serial_jtag_priv *priv = container_of(cdev,
		struct altera_serial_jtag_priv, cdev);

	struct nios_jtag *jtag = priv->regs;

	return readl(&jtag->control) & NIOS_JTAG_RRDY;
}

static int altera_serial_jtag_getc(struct console_device *cdev)
{
	struct altera_serial_jtag_priv *priv = container_of(cdev,
		struct altera_serial_jtag_priv, cdev);

	struct nios_jtag *jtag = priv->regs;
	uint32_t val;

	while (1) {
		val = readl(&jtag->data);
		if (val & NIOS_JTAG_RVALID)
			break;
	}

	return val & 0xFF;
}

static int altera_serial_jtag_probe(struct device_d *dev) {

	struct console_device *cdev;
	struct altera_serial_jtag_priv *priv;

	priv = xzalloc(sizeof(*priv));
	cdev = &priv->cdev;

	priv->regs = dev_request_mem_region(dev, 0);
	cdev->dev = dev;
	cdev->tstc = altera_serial_jtag_tstc;
	cdev->putc = altera_serial_jtag_putc;
	cdev->getc = altera_serial_jtag_getc;
	cdev->setbrg = altera_serial_jtag_setbaudrate;

	console_register(cdev);

	return 0;
}

static struct driver_d altera_serial_jtag_driver = {
	.name = "altera_serial_jtag",
	.probe = altera_serial_jtag_probe,
};
console_platform_driver(altera_serial_jtag_driver);

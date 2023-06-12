// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2013, 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <io.h>

#include <mach/digic/uart.h>

/*
 * This driver is based on the "Serial terminal" docs here:
 *  http://magiclantern.wikia.com/wiki/Register_Map#Misc_Registers
 *
 * See also disassembler output for Canon A1100IS firmware
 * (version a1100_100c):
 *   * a outc-like function can be found at address 0xffff18f0;
 *   * a getc-like function can be found at address 0xffff192c.
 */

static inline uint32_t digic_serial_readl(struct console_device *cdev,
						uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	return readl(base + offset);
}

static inline void digic_serial_writel(struct console_device *cdev,
					uint32_t value, uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	writel(value, base + offset);
}

static int digic_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	/* I don't know how to setup baudrate :( */

	return 0;
}

static void digic_serial_putc(struct console_device *cdev, char c)
{
	uint32_t status;

	do {
		status = digic_serial_readl(cdev, DIGIC_UART_ST);
	} while (!(status & DIGIC_UART_ST_TX_RDY));

	digic_serial_writel(cdev, 0x06, DIGIC_UART_ST);
	digic_serial_writel(cdev, c, DIGIC_UART_TX);
}

static int digic_serial_getc(struct console_device *cdev)
{
	uint32_t status;

	do {
		status = digic_serial_readl(cdev, DIGIC_UART_ST);
	} while (!(status & DIGIC_UART_ST_RX_RDY));

	digic_serial_writel(cdev, 0x01, DIGIC_UART_ST);

	return digic_serial_readl(cdev, DIGIC_UART_RX);
}

static int digic_serial_tstc(struct console_device *cdev)
{
	uint32_t status = digic_serial_readl(cdev, DIGIC_UART_ST);

	return ((status & DIGIC_UART_ST_RX_RDY) != 0);

	/*
	 * Canon folks use additional check, something like this:
	 *
	 *	if (digic_serial_readl(cdev, DIGIC_UART_ST) & 0x38) {
	 *		digic_serial_writel(cdev, 0x38, DIGIC_UART_ST);
	 *		return 0;
	 *	}
	 *
	 * But I know nothing about these magic bits in the status register...
	 *
	*/
}

static int digic_serial_probe(struct device *dev)
{
	struct resource *iores;
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	dev->priv = IOMEM(iores->start);
	cdev->dev = dev;
	cdev->tstc = &digic_serial_tstc;
	cdev->putc = &digic_serial_putc;
	cdev->getc = &digic_serial_getc;
	cdev->setbrg = &digic_serial_setbaudrate;

	console_register(cdev);

	return 0;
}

static __maybe_unused struct of_device_id digic_serial_dt_ids[] = {
	{
		.compatible = "canon,digic-uart",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, digic_serial_dt_ids);

static struct driver digic_serial_driver = {
	.name  = "digic-uart",
	.probe = digic_serial_probe,
	.of_compatible = DRV_OF_COMPAT(digic_serial_dt_ids),
};
console_platform_driver(digic_serial_driver);

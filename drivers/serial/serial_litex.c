// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Antony Pavlov <antonynpavlov@gmail.com>
 *
 */

#include <common.h>
#include <init.h>
#include <io.h>

#define UART_RXTX	0x00
#define UART_TXFULL	0x04
#define UART_RXEMPTY	0x08
#define UART_EV_PENDING	0x10
#define  UART_EV_RX	(1 << 1)

static inline uint32_t litex_serial_readb(struct console_device *cdev,
						uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	return readb(base + offset);
}

static inline void litex_serial_writeb(struct console_device *cdev,
					uint32_t value, uint32_t offset)
{
	void __iomem *base = cdev->dev->priv;

	writeb(value, base + offset);
}

static void litex_serial_putc(struct console_device *cdev, char c)
{
	while (litex_serial_readb(cdev, UART_TXFULL))
		;

	litex_serial_writeb(cdev, c, UART_RXTX);
}

static int litex_serial_getc(struct console_device *cdev)
{
	int c;

	while (litex_serial_readb(cdev, UART_RXEMPTY))
		;

	c = litex_serial_readb(cdev, UART_RXTX);

	/* refresh UART_RXEMPTY by writing UART_EV_RX to UART_EV_PENDING */
	litex_serial_writeb(cdev, UART_EV_RX, UART_EV_PENDING);

	return c;
}

static int litex_serial_tstc(struct console_device *cdev)
{
	return !litex_serial_readb(cdev, UART_RXEMPTY);
}

static int litex_serial_probe(struct device *dev)
{
	struct resource *iores;
	struct console_device *cdev;

	cdev = xzalloc(sizeof(struct console_device));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	dev->priv = IOMEM(iores->start);
	cdev->dev = dev;
	cdev->tstc = &litex_serial_tstc;
	cdev->putc = &litex_serial_putc;
	cdev->getc = &litex_serial_getc;
	cdev->setbrg = NULL;
	cdev->linux_console_name = "ttyLXU";
	cdev->linux_earlycon_name = "liteuart";
	cdev->phys_base = IOMEM(iores->start);

	console_register(cdev);

	return 0;
}

static __maybe_unused struct of_device_id litex_serial_dt_ids[] = {
	{
		.compatible = "litex,uart",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, litex_serial_dt_ids);

static struct driver litex_serial_driver = {
	.name  = "litex-uart",
	.probe = litex_serial_probe,
	.of_compatible = DRV_OF_COMPAT(litex_serial_dt_ids),
};
console_platform_driver(litex_serial_driver);

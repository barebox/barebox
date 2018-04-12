/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 *
 * This code was inspired by some patches made for u-boot covered by:
 * (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2009 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

/*
 * Note: This is the driver for the debug UART. There is
 * only one of these UARTs on the Freescale/SigmaTel parts
 */

#include <common.h>
#include <init.h>
#include <notifier.h>
#include <gpio.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mach/clock.h>

#define UARTDBGDR 0x00
#define UARTDBGFR 0x18
# define TXFE (1 << 7)
# define TXFF (1 << 5)
# define RXFE (1 << 4)
#define UARTDBGIBRD 0x24
#define UARTDBGFBRD 0x28
#define UARTDBGLCR_H 0x2c
# define WLEN8 (3 << 5)
# define WLEN7 (2 << 5)
# define WLEN6 (1 << 5)
# define WLEN5 (0 << 5)
# define FEN (1 << 4)
#define UARTDBGCR 0x30
# define UARTEN (1 << 0)
# define TXE (1 << 8)
# define RXE (1 << 9)
#define UARTDBGIMSC 0x38

struct stm_priv {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
	void __iomem *base;
	struct clk *clk;
};

static void stm_serial_putc(struct console_device *cdev, char c)
{
	struct stm_priv *priv = container_of(cdev, struct stm_priv, cdev);

	/* Wait for room in TX FIFO */
	while (readl(priv->base + UARTDBGFR) & TXFF)
		;

	writel(c, priv->base + UARTDBGDR);
}

static int stm_serial_tstc(struct console_device *cdev)
{
	struct stm_priv *priv = container_of(cdev, struct stm_priv, cdev);

	/* Check if RX FIFO is not empty */
	return !(readl(priv->base + UARTDBGFR) & RXFE);
}

static int stm_serial_getc(struct console_device *cdev)
{
	struct stm_priv *priv = container_of(cdev, struct stm_priv, cdev);

	/* Wait while TX FIFO is empty */
	while (readl(priv->base + UARTDBGFR) & RXFE)
		;

	return readl(priv->base + UARTDBGDR) & 0xff;
}

static void stm_serial_flush(struct console_device *cdev)
{
	struct stm_priv *priv = container_of(cdev, struct stm_priv, cdev);

	/* Wait for TX FIFO empty */
	while (!(readl(priv->base + UARTDBGFR) & TXFE))
		;
}

static int stm_serial_setbaudrate(struct console_device *cdev, int new_baudrate)
{
	struct stm_priv *priv = container_of(cdev, struct stm_priv, cdev);
	uint32_t cr, lcr_h, quot;

	/* Disable everything */
	cr = readl(priv->base + UARTDBGCR);
	writel(0, priv->base + UARTDBGCR);

	/* Calculate and set baudrate */
	quot = (clk_get_rate(priv->clk) * 4) / new_baudrate;
	writel(quot & 0x3f, priv->base + UARTDBGFBRD);
	writel(quot >> 6, priv->base + UARTDBGIBRD);

	/* Set 8n1 mode, enable FIFOs */
	lcr_h = WLEN8 | FEN;
	writel(lcr_h, priv->base + UARTDBGLCR_H);

	/* Re-enable debug UART */
	writel(cr, priv->base + UARTDBGCR);

	priv->baudrate = new_baudrate;

	return 0;
}

static int stm_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	struct stm_priv *priv = container_of(nb, struct stm_priv, notify);

	return stm_serial_setbaudrate(&priv->cdev, priv->baudrate);
}

static int stm_serial_init_port(struct stm_priv *priv)
{
	/* Disable UART */
	writel(0, priv->base + UARTDBGCR);

	/* Mask interrupts */
	writel(0, priv->base + UARTDBGIMSC);

	return 0;
}

static int stm_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct stm_priv *priv;
	struct console_device *cdev;

	priv = xzalloc(sizeof *priv);

	cdev = &priv->cdev;

	cdev->tstc = stm_serial_tstc;
	cdev->putc = stm_serial_putc;
	cdev->getc = stm_serial_getc;
	cdev->flush = stm_serial_flush;
	cdev->setbrg = stm_serial_setbaudrate;
	cdev->dev = dev;

	dev->priv = priv;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->base = IOMEM(iores->start);
	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	stm_serial_init_port(priv);
	stm_serial_setbaudrate(cdev, CONFIG_BAUDRATE);

	/* Enable UART */
	writel(TXE | RXE | UARTEN, priv->base + UARTDBGCR);

	console_register(cdev);
	priv->notify.notifier_call = stm_clocksource_clock_change;
	clock_register_client(&priv->notify);

	return 0;
}

static __maybe_unused struct of_device_id stm_serial_dt_ids[] = {
	{
		.compatible = "arm,pl011",
	}, {
		/* sentinel */
	}
};

static struct driver_d stm_serial_driver = {
        .name   = "stm_serial",
        .probe  = stm_serial_probe,
	.of_compatible = DRV_OF_COMPAT(stm_serial_dt_ids),
};
console_platform_driver(stm_serial_driver);

/*
 * (C) 2013 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * Based on the stm-serial driver:
 *
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
 *
 * ...also based on the u-boot auart driver:
 *
 * (C) 2011 Wolfgang Ocker <weo@reccoware.de>
 *
 * Based on the standard DUART serial driver:
 *
 * (C) 2007 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * (C) Copyright 2009-2010 Freescale Semiconductor, Inc.
 *
 * Further based on the Linux mxs-auart.c driver:
 *
 * Freescale STMP37XX/STMP378X Application UART driver
 *
 * Author: dmitry pervushin <dimka@embeddedalley.com>
 *
 * Copyright 2008-2010 Freescale Semiconductor, Inc.
 * Copyright 2008 Embedded Alley Solutions, Inc All Rights Reserved.
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
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <notifier.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/err.h>

#include <mach/clock.h>

#define HW_UARTAPP_CTRL0		(0x00000000)

#define HW_UARTAPP_CTRL2		(0x00000020)
#define HW_UARTAPP_CTRL2_SET		(0x00000024)
#define HW_UARTAPP_CTRL2_CLR		(0x00000028)
#define BM_UARTAPP_CTRL2_CTSEN		(0x00008000)
#define BM_UARTAPP_CTRL2_RTSEN		(0x00004000)
#define BM_UARTAPP_CTRL2_RXE		(0x00000200)
#define BM_UARTAPP_CTRL2_TXE		(0x00000100)
#define BM_UARTAPP_CTRL2_USE_LCR2	(0x00000040)
#define BM_UARTAPP_CTRL2_UARTEN		(0x00000001)

#define HW_UARTAPP_LINECTRL		(0x00000030)
#define BM_UARTAPP_LINECTRL_FEN		(0x00000010)

#define BM_UARTAPP_LINECTRL_BAUD_DIVFRAC (0x00003F00)
#define BF_UARTAPP_LINECTRL_BAUD_DIVFRAC(v) \
	(((v) << 8) & BM_UARTAPP_LINECTRL_BAUD_DIVFRAC)

#define BP_UARTAPP_LINECTRL_BAUD_DIVINT	(16)
#define BM_UARTAPP_LINECTRL_BAUD_DIVINT	(0xFFFF0000)
#define BF_UARTAPP_LINECTRL_BAUD_DIVINT(v) \
	(((v) << 16) & BM_UARTAPP_LINECTRL_BAUD_DIVINT)

#define BP_UARTAPP_LINECTRL_WLEN	(5)
#define BM_UARTAPP_LINECTRL_WLEN	(0x00000060)
#define BF_UARTAPP_LINECTRL_WLEN(v) \
	(((v) << 5) & BM_UARTAPP_LINECTRL_WLEN)

#define HW_UARTAPP_LINECTRL2_SET	(0x00000044)

#define HW_UARTAPP_INTR			(0x00000050)

#define HW_UARTAPP_DATA			(0x00000060)
#define BM_UARTAPP_STAT_RXFE		(0x01000000)
#define BM_UARTAPP_STAT_TXFE		(0x08000000)

#define HW_UARTAPP_STAT			(0x00000070)
#define BM_UARTAPP_STAT_TXFF		(0x02000000)

struct auart_priv {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
	void __iomem *base;
	struct clk *clk;
};

static void auart_serial_putc(struct console_device *cdev, char c)
{
	struct auart_priv *priv = container_of(cdev, struct auart_priv, cdev);

	/* Wait for room in TX FIFO */
	while (readl(priv->base + HW_UARTAPP_STAT) & BM_UARTAPP_STAT_TXFF)
		;

	writel(c, priv->base + HW_UARTAPP_DATA);
}

static int auart_serial_tstc(struct console_device *cdev)
{
	struct auart_priv *priv = container_of(cdev, struct auart_priv, cdev);

	/* Check if RX FIFO is not empty */
	return !(readl(priv->base + HW_UARTAPP_STAT) & BM_UARTAPP_STAT_RXFE);
}

static int auart_serial_getc(struct console_device *cdev)
{
	struct auart_priv *priv = container_of(cdev, struct auart_priv, cdev);

	/* Wait while RX FIFO is empty */
	while (!auart_serial_tstc(cdev))
		;

	return readl(priv->base + HW_UARTAPP_DATA) & 0xff;
}

static void auart_serial_flush(struct console_device *cdev)
{
	struct auart_priv *priv = container_of(cdev, struct auart_priv, cdev);

	/* Wait for TX FIFO empty */
	while (!(readl(priv->base + HW_UARTAPP_STAT) & BM_UARTAPP_STAT_TXFE))
		;
}

static int auart_serial_setbaudrate(struct console_device *cdev, int new_baudrate)
{
	struct auart_priv *priv = container_of(cdev, struct auart_priv, cdev);
	uint32_t ctrl2, quot, reg;

	/* Disable everything */
	ctrl2 = readl(priv->base + HW_UARTAPP_CTRL2);
	writel(0x0, priv->base + HW_UARTAPP_CTRL2);

	/* Calculate and set baudrate */
	quot = (clk_get_rate(priv->clk) * 32) / new_baudrate;
	reg = BF_UARTAPP_LINECTRL_BAUD_DIVFRAC(quot & 0x3F) |
		BF_UARTAPP_LINECTRL_BAUD_DIVINT(quot >> 6) |
		BF_UARTAPP_LINECTRL_WLEN(3) |
		BM_UARTAPP_LINECTRL_FEN;

	writel(reg, priv->base + HW_UARTAPP_LINECTRL);

	/* Re-enable UART */
	writel(ctrl2, priv->base + HW_UARTAPP_CTRL2);

	priv->baudrate = new_baudrate;

	return 0;
}

static int auart_clocksource_clock_change(struct notifier_block *nb, unsigned long event, void *data)
{
	struct auart_priv *priv = container_of(nb, struct auart_priv, notify);

	return auart_serial_setbaudrate(&priv->cdev, priv->baudrate);
}

static void auart_serial_init_port(struct auart_priv *priv)
{
	stmp_reset_block(priv->base + HW_UARTAPP_CTRL0, 0);

	/* Disable UART */
	writel(0x0, priv->base + HW_UARTAPP_CTRL2);
	/* Mask interrupts */
	writel(0x0, priv->base + HW_UARTAPP_INTR);
}

static int auart_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct auart_priv *priv;
	struct console_device *cdev;

	priv = xzalloc(sizeof *priv);
	cdev = &priv->cdev;

	cdev->tstc = auart_serial_tstc;
	cdev->putc = auart_serial_putc;
	cdev->getc = auart_serial_getc;
	cdev->flush = auart_serial_flush;
	cdev->setbrg = auart_serial_setbaudrate;
	cdev->dev = dev;

	dev->priv = priv;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->base = IOMEM(iores->start);
	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);

	auart_serial_init_port(priv);

	/* Disable RTS/CTS, enable Rx, Tx, UART */
	writel(BM_UARTAPP_CTRL2_RTSEN | BM_UARTAPP_CTRL2_CTSEN |
	       BM_UARTAPP_CTRL2_USE_LCR2,
	       priv->base + HW_UARTAPP_CTRL2_CLR);
	writel(BM_UARTAPP_CTRL2_RXE | BM_UARTAPP_CTRL2_TXE |
	       BM_UARTAPP_CTRL2_UARTEN,
	       priv->base + HW_UARTAPP_CTRL2_SET);

	console_register(cdev);
	priv->notify.notifier_call = auart_clocksource_clock_change;
	clock_register_client(&priv->notify);

	return 0;
}

static const __maybe_unused struct of_device_id auart_serial_dt_ids[] = {
	{
		.compatible = "fsl,imx23-auart",
	}, {
		/* sentinel */
	}
};

static struct driver_d auart_serial_driver = {
	.name = "auart_serial",
	.probe = auart_serial_probe,
	.of_compatible = DRV_OF_COMPAT(auart_serial_dt_ids),
};
console_platform_driver(auart_serial_driver);

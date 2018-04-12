/*
 * Simple CLPS711X serial driver
 *
 * (C) Copyright 2012-2014 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <malloc.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <mfd/syscon.h>

#define UARTDR			0x00
# define UARTDR_FRMERR		(1 << 8)
# define UARTDR_PARERR		(1 << 9)
# define UARTDR_OVERR		(1 << 10)
#define UBRLCR			0x40
# define UBRLCR_BAUD_MASK	((1 << 12) - 1)
# define UBRLCR_BREAK		(1 << 12)
# define UBRLCR_PRTEN		(1 << 13)
# define UBRLCR_EVENPRT		(1 << 14)
# define UBRLCR_XSTOP		(1 << 15)
# define UBRLCR_FIFOEN		(1 << 16)
# define UBRLCR_WRDLEN5		(0 << 17)
# define UBRLCR_WRDLEN6		(1 << 17)
# define UBRLCR_WRDLEN7		(2 << 17)
# define UBRLCR_WRDLEN8		(3 << 17)
# define UBRLCR_WRDLEN_MASK	(3 << 17)

#define SYSCON			0x00
# define SYSCON_UARTEN		(1 << 8)
#define SYSFLG			0x40
# define SYSFLG_UBUSY		(1 << 11)
# define SYSFLG_URXFE		(1 << 22)
# define SYSFLG_UTXFF		(1 << 23)

struct clps711x_uart {
	void __iomem		*base;
	void __iomem		*syscon;
	struct clk		*uart_clk;
	struct console_device	cdev;
};

static int clps711x_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct clps711x_uart *s = cdev->dev->priv;
	int divisor;
	u32 tmp;

	divisor = (clk_get_rate(s->uart_clk) / 16) / baudrate;

	tmp = readl(s->base + UBRLCR) & ~UBRLCR_BAUD_MASK;
	tmp |= divisor - 1;
	writel(tmp, s->base + UBRLCR);

	return 0;
}

static void clps711x_init_port(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	/* Disable the UART */
	tmp = readl(s->syscon + SYSCON);
	writel(tmp & ~SYSCON_UARTEN, s->syscon + SYSCON);

	/* Setup Line Control Register */
	tmp = readl(s->base + UBRLCR) & UBRLCR_BAUD_MASK;
	tmp |= UBRLCR_FIFOEN | UBRLCR_WRDLEN8; /* FIFO on, 8N1 mode */
	writel(tmp, s->base + UBRLCR);

	/* Enable the UART */
	tmp = readl(s->syscon + SYSCON);
	writel(tmp | SYSCON_UARTEN, s->syscon + SYSCON);
}

static void clps711x_putc(struct console_device *cdev, char c)
{
	struct clps711x_uart *s = cdev->dev->priv;

	/* Wait until there is space in the FIFO */
	do {
	} while (readl(s->syscon + SYSFLG) & SYSFLG_UTXFF);

	/* Send the character */
	writew(c, s->base + UARTDR);
}

static int clps711x_getc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u16 data;

	/* Wait until there is data in the FIFO */
	do {
	} while (readl(s->syscon + SYSFLG) & SYSFLG_URXFE);

	data = readw(s->base + UARTDR);

	/* Check for an error flag */
	if (data & (UARTDR_FRMERR | UARTDR_PARERR | UARTDR_OVERR))
		return -1;

	return (int)data;
}

static int clps711x_tstc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;

	return !(readl(s->syscon + SYSFLG) & SYSFLG_URXFE);
}

static void clps711x_flush(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;

	do {
	} while (readl(s->syscon + SYSFLG) & SYSFLG_UBUSY);
}

static int clps711x_probe(struct device_d *dev)
{
	struct clps711x_uart *s;
	int err, id = dev->id;
	char syscon_dev[8];

	if (dev->device_node)
		id = of_alias_get_id(dev->device_node, "serial");

	if (id != 0 && id != 1)
		return -EINVAL;

	s = xzalloc(sizeof(struct clps711x_uart));
	s->uart_clk = clk_get(dev, NULL);
	if (IS_ERR(s->uart_clk)) {
		err = PTR_ERR(s->uart_clk);
		goto out_err;
	}

	s->base = dev_get_mem_region(dev, 0);
	if (IS_ERR(s->base))
		return PTR_ERR(s->base);

	if (!dev->device_node) {
		sprintf(syscon_dev, "syscon%i", id + 1);
		s->syscon = syscon_base_lookup_by_pdevname(syscon_dev);
	} else {
		s->syscon = syscon_base_lookup_by_phandle(dev->device_node,
							  "syscon");
	}

	if (IS_ERR(s->syscon)) {
		err = PTR_ERR(s->syscon);
		goto out_err;
	}

	dev->priv	= s;
	s->cdev.dev	= dev;
	s->cdev.tstc	= clps711x_tstc;
	s->cdev.putc	= clps711x_putc;
	s->cdev.getc	= clps711x_getc;
	s->cdev.flush	= clps711x_flush;
	s->cdev.setbrg	= clps711x_setbaudrate;
	clps711x_init_port(&s->cdev);

	err = console_register(&s->cdev);

out_err:
	if (err)
		free(s);

	return err;
}

static struct of_device_id __maybe_unused clps711x_uart_dt_ids[] = {
	{ .compatible = "cirrus,clps711x-uart", },
};

static struct driver_d clps711x_driver = {
	.name		= "clps711x-uart",
	.probe		= clps711x_probe,
	.of_compatible	= DRV_OF_COMPAT(clps711x_uart_dt_ids),
};
console_platform_driver(clps711x_driver);

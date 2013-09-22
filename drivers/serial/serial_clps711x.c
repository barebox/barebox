/*
 * Simple CLPS711X serial driver
 *
 * (C) Copyright 2012 Alexander Shiyan <shc_work@mail.ru>
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

#include <mach/clps711x.h>

struct clps711x_uart {
	void __iomem		*UBRLCR;
	void __iomem		*UARTDR;
	void __iomem		*syscon;
	struct clk		*uart_clk;
	struct console_device	cdev;
};

#define SYSCON(x)	((x)->syscon + 0x00)
#define SYSFLG(x)	((x)->syscon + 0x40)

static int clps711x_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct clps711x_uart *s = cdev->dev->priv;
	int divisor;
	u32 tmp;

	divisor = (clk_get_rate(s->uart_clk) / 16) / baudrate;

	tmp = readl(s->UBRLCR) & ~UBRLCR_BAUD_MASK;
	tmp |= divisor - 1;
	writel(tmp, s->UBRLCR);

	return 0;
}

static void clps711x_init_port(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u32 tmp;

	/* Disable the UART */
	writel(readl(SYSCON(s)) & ~SYSCON_UARTEN, SYSCON(s));

	/* Setup Line Control Register */
	tmp = readl(s->UBRLCR) & UBRLCR_BAUD_MASK;
	tmp |= UBRLCR_FIFOEN | UBRLCR_WRDLEN8; /* FIFO on, 8N1 mode */
	writel(tmp, s->UBRLCR);

	/* Enable the UART */
	writel(readl(SYSCON(s)) | SYSCON_UARTEN, SYSCON(s));
}

static void clps711x_putc(struct console_device *cdev, char c)
{
	struct clps711x_uart *s = cdev->dev->priv;

	/* Wait until there is space in the FIFO */
	while (readl(SYSFLG(s)) & SYSFLG_UTXFF)
		barrier();

	/* Send the character */
	writew(c, s->UARTDR);
}

static int clps711x_getc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u16 data;

	/* Wait until there is data in the FIFO */
	while (readl(SYSFLG(s)) & SYSFLG_URXFE)
		barrier();

	data = readw(s->UARTDR);

	/* Check for an error flag */
	if (data & (UARTDR_FRMERR | UARTDR_PARERR | UARTDR_OVERR))
		return -1;

	return (int)data;
}

static int clps711x_tstc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;

	return !(readl(SYSFLG(s)) & SYSFLG_URXFE);
}

static void clps711x_flush(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;

	while (readl(SYSFLG(s)) & SYSFLG_UBUSY)
		barrier();
}

static int clps711x_probe(struct device_d *dev)
{
	struct clps711x_uart *s;
	char syscon_dev[18];

	BUG_ON(dev->num_resources != 2);
	BUG_ON((dev->id != 0) && (dev->id != 1));

	s = xzalloc(sizeof(struct clps711x_uart));
	s->uart_clk = clk_get(dev, NULL);
	BUG_ON(IS_ERR(s->uart_clk));

	s->UBRLCR = dev_get_mem_region(dev, 0);
	s->UARTDR = dev_get_mem_region(dev, 1);

	sprintf(syscon_dev, "clps711x-syscon%i", dev->id + 1);
	s->syscon = syscon_base_lookup_by_pdevname(syscon_dev);
	BUG_ON(IS_ERR(s->syscon));

	dev->priv	= s;
	s->cdev.dev	= dev;
	s->cdev.tstc	= clps711x_tstc;
	s->cdev.putc	= clps711x_putc;
	s->cdev.getc	= clps711x_getc;
	s->cdev.flush	= clps711x_flush;
	s->cdev.setbrg	= clps711x_setbaudrate;
	clps711x_init_port(&s->cdev);

	return console_register(&s->cdev);
}

static void clps711x_remove(struct device_d *dev)
{
	struct clps711x_uart *s = dev->priv;

	clps711x_flush(&s->cdev);
	console_unregister(&s->cdev);
	free(s);
}

static struct driver_d clps711x_driver = {
	.name	= "clps711x_serial",
	.probe	= clps711x_probe,
	.remove	= clps711x_remove,
};
console_platform_driver(clps711x_driver);

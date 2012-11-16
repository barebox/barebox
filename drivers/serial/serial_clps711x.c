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

#include <mach/clps711x.h>

struct clps711x_uart {
	void __iomem		*UBRLCR;
	void __iomem		*SYSCON;
	void __iomem		*SYSFLG;
	void __iomem		*UARTDR;
	struct clk		*uart_clk;
	struct console_device	cdev;
};

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
	writel(readl(s->SYSCON) & ~SYSCON_UARTEN, s->SYSCON);

	/* Setup Line Control Register */
	tmp = readl(s->UBRLCR) & UBRLCR_BAUD_MASK;
	tmp |= UBRLCR_FIFOEN | UBRLCR_WRDLEN8; /* FIFO on, 8N1 mode */
	writel(tmp, s->UBRLCR);

	/* Set default baudrate on initialization */
	clps711x_setbaudrate(cdev, CONFIG_BAUDRATE);

	/* Enable the UART */
	writel(readl(s->SYSCON) | SYSCON_UARTEN, s->SYSCON);
}

static void clps711x_putc(struct console_device *cdev, char c)
{
	struct clps711x_uart *s = cdev->dev->priv;

	/* Wait until there is space in the FIFO */
	while (readl(s->SYSFLG) & SYSFLG_UTXFF)
		barrier();

	/* Send the character */
	writew(c, s->UARTDR);
}

static int clps711x_getc(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;
	u16 data;

	/* Wait until there is data in the FIFO */
	while (readl(s->SYSFLG) & SYSFLG_URXFE)
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

	return !(readl(s->SYSFLG) & SYSFLG_URXFE);
}

static void clps711x_flush(struct console_device *cdev)
{
	struct clps711x_uart *s = cdev->dev->priv;

	while (readl(s->SYSFLG) & SYSFLG_UBUSY)
		barrier();
}

static int clps711x_probe(struct device_d *dev)
{
	struct clps711x_uart *s;

	BUG_ON(dev->num_resources != 4);

	s = xzalloc(sizeof(struct clps711x_uart));
	s->uart_clk = clk_get(dev, NULL);
	BUG_ON(IS_ERR(s->uart_clk));

	s->UBRLCR = dev_get_mem_region(dev, 0);
	s->SYSCON = dev_get_mem_region(dev, 1);
	s->SYSFLG = dev_get_mem_region(dev, 2);
	s->UARTDR = dev_get_mem_region(dev, 3);

	dev->priv	= s;
	s->cdev.dev	= dev;
	s->cdev.f_caps	= CONSOLE_STDIN | CONSOLE_STDOUT | CONSOLE_STDERR;
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

static int clps711x_init(void)
{
	return platform_driver_register(&clps711x_driver);
}
console_initcall(clps711x_init);

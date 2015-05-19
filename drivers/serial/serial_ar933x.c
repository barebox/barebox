/*
 * based on linux.git/drivers/tty/serial/serial_ar933x.c
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
#include <asm-generic/div64.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "serial_ar933x.h"

#define AR933X_UART_MAX_SCALE	0xff
#define AR933X_UART_MAX_STEP	0xffff

struct ar933x_uart_priv {
	void __iomem	*base;
	struct clk	*clk;
};

static inline void ar933x_serial_writel(struct console_device *cdev,
	u32 b, int offset)
{
	struct ar933x_uart_priv *priv = cdev->dev->priv;

	__raw_writel(b, priv->base + offset);
}

static inline u32 ar933x_serial_readl(struct console_device *cdev,
	int offset)
{
	struct ar933x_uart_priv *priv = cdev->dev->priv;

	return __raw_readl(priv->base + offset);
}

/*
 * baudrate = (clk / (scale + 1)) * (step * (1 / 2^17))
 * take from linux.
 */
static unsigned long ar933x_uart_get_baud(unsigned int clk,
					  unsigned int scale,
					  unsigned int step)
{
	u64 t;
	u32 div;

	div = (2 << 16) * (scale + 1);
	t = clk;
	t *= step;
	t += (div / 2);
	do_div(t, div);

	return t;
}

static void ar933x_uart_get_scale_step(unsigned int clk,
				       unsigned int baud,
				       unsigned int *scale,
				       unsigned int *step)
{
	unsigned int tscale;
	long min_diff;

	*scale = 0;
	*step = 0;

	min_diff = baud;
	for (tscale = 0; tscale < AR933X_UART_MAX_SCALE; tscale++) {
		u64 tstep;
		int diff;

		tstep = baud * (tscale + 1);
		tstep *= (2 << 16);
		do_div(tstep, clk);

		if (tstep > AR933X_UART_MAX_STEP)
			break;

		diff = abs(ar933x_uart_get_baud(clk, tscale, tstep) - baud);
		if (diff < min_diff) {
			min_diff = diff;
			*scale = tscale;
			*step = tstep;
		}
	}
}

static int ar933x_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct ar933x_uart_priv *priv = cdev->dev->priv;
	unsigned int scale, step;

	ar933x_uart_get_scale_step(clk_get_rate(priv->clk), baudrate, &scale,
			&step);
	ar933x_serial_writel(cdev, (scale << AR933X_UART_CLOCK_SCALE_S) | step,
			AR933X_UART_CLOCK_REG);

	return 0;
}

static void ar933x_serial_putc(struct console_device *cdev, char ch)
{
	u32 data;

	/* wait transmitter ready */
	data = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);
	while (!(data & AR933X_UART_DATA_TX_CSR))
		data = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);

	data = (ch & AR933X_UART_DATA_TX_RX_MASK) | AR933X_UART_DATA_TX_CSR;
	ar933x_serial_writel(cdev, data, AR933X_UART_DATA_REG);
}

static int ar933x_serial_tstc(struct console_device *cdev)
{
	u32 rdata;

	rdata = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);

	return rdata & AR933X_UART_DATA_RX_CSR;
}

static int ar933x_serial_getc(struct console_device *cdev)
{
	u32 rdata;

	while (!ar933x_serial_tstc(cdev))
		;

	rdata = ar933x_serial_readl(cdev, AR933X_UART_DATA_REG);

	/* remove the character from the FIFO */
	ar933x_serial_writel(cdev, AR933X_UART_DATA_RX_CSR,
		AR933X_UART_DATA_REG);

	return rdata & AR933X_UART_DATA_TX_RX_MASK;
}

static int ar933x_serial_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct ar933x_uart_priv	*priv;
	u32 uart_cs;

	cdev = xzalloc(sizeof(struct console_device));
	priv = xzalloc(sizeof(struct ar933x_uart_priv));
	priv->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(priv->base))
		return PTR_ERR(priv->base);

	dev->priv = priv;

	cdev->dev = dev;
	cdev->tstc = ar933x_serial_tstc;
	cdev->putc = ar933x_serial_putc;
	cdev->getc = ar933x_serial_getc;
	cdev->setbrg = ar933x_serial_setbaudrate;

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "unable to get UART clock\n");
		return PTR_ERR(priv->clk);
	}

	uart_cs = (AR933X_UART_CS_IF_MODE_DCE << AR933X_UART_CS_IF_MODE_S)
		| AR933X_UART_CS_TX_READY_ORIDE
		| AR933X_UART_CS_RX_READY_ORIDE;
	ar933x_serial_writel(cdev, uart_cs, AR933X_UART_CS_REG);
	/* FIXME: need ar933x_serial_init_port(cdev); */

	console_register(cdev);

	return 0;
}

static struct of_device_id ar933x_serial_dt_ids[] = {
	{
		.compatible = "qca,ar9330-uart",
	}, {
		/* sentinel */
	},
};

static struct driver_d ar933x_serial_driver = {
	.name  = "ar933x_serial",
	.probe = ar933x_serial_probe,
	.of_compatible = DRV_OF_COMPAT(ar933x_serial_dt_ids),
};
console_platform_driver(ar933x_serial_driver);

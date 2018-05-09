/*
 * (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>
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
 */
#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <notifier.h>
#include <io.h>
#include <linux/err.h>
#include <linux/clk.h>

#define CADENCE_UART_CONTROL		0x00
#define CADENCE_UART_MODE		0x04
#define CADENCE_UART_BAUD_GEN		0x18
#define CADENCE_UART_CHANNEL_STS	0x2C
#define CADENCE_UART_RXTXFIFO		0x30
#define CADENCE_UART_BAUD_DIV		0x34

#define CADENCE_CTRL_RXRES		(1 << 0)
#define CADENCE_CTRL_TXRES		(1 << 1)
#define CADENCE_CTRL_RXEN		(1 << 2)
#define CADENCE_CTRL_RXDIS		(1 << 3)
#define CADENCE_CTRL_TXEN		(1 << 4)
#define CADENCE_CTRL_TXDIS		(1 << 5)
#define CADENCE_CTRL_RSTTO		(1 << 6)
#define CADENCE_CTRL_STTBRK		(1 << 7)
#define CADENCE_CTRL_STPBRK		(1 << 8)

#define CADENCE_MODE_CLK_REF		(0 << 0)
#define CADENCE_MODE_CLK_REF_DIV	(1 << 0)
#define CADENCE_MODE_CHRL_6		(3 << 1)
#define CADENCE_MODE_CHRL_7		(2 << 1)
#define CADENCE_MODE_CHRL_8		(0 << 1)
#define CADENCE_MODE_PAR_EVEN		(0 << 3)
#define CADENCE_MODE_PAR_ODD		(1 << 3)
#define CADENCE_MODE_PAR_SPACE		(2 << 3)
#define CADENCE_MODE_PAR_MARK		(3 << 3)
#define CADENCE_MODE_PAR_NONE		(4 << 3)

#define CADENCE_STS_REMPTY		(1 << 1)
#define CADENCE_STS_RFUL		(1 << 2)
#define CADENCE_STS_TEMPTY		(1 << 3)
#define CADENCE_STS_TFUL		(1 << 4)

/*
 * create default values for different platforms
 */
struct cadence_serial_devtype_data {
	u32 ctrl;
	u32 mode;
};

static struct cadence_serial_devtype_data cadence_r1p08_data = {
	.ctrl = CADENCE_CTRL_RXEN | CADENCE_CTRL_TXEN,
	.mode = CADENCE_MODE_CLK_REF | CADENCE_MODE_CHRL_8 | CADENCE_MODE_PAR_NONE,
};

struct cadence_serial_priv {
	struct console_device cdev;
	int baudrate;
	struct notifier_block notify;
	void __iomem *regs;
	struct clk *clk;
	struct cadence_serial_devtype_data *devtype;
};

static int cadence_serial_reset(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	/* Soft-Reset Tx/Rx paths */
	writel(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES, priv->regs +
		CADENCE_UART_CONTROL);

	while (readl(priv->regs + CADENCE_UART_CONTROL) &
		(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES))
		;

	return 0;
}

static int cadence_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);
	unsigned int gen, div;
	int calc_rate;
	unsigned long clk;
	int error;
	int val;

	clk = clk_get_rate(priv->clk);
	priv->baudrate = baudrate;

	/* disable transmitter and receiver */
	val = readl(priv->regs + CADENCE_UART_CONTROL);
	val &= ~CADENCE_CTRL_TXEN & ~CADENCE_CTRL_RXEN;
	writel(val, priv->regs + CADENCE_UART_CONTROL);

	/*
	 *	      clk
	 * rate = -----------
	 *	  gen*(div+1)
	 */

	for (div = 4; div < 256; div++) {
		gen = clk / (baudrate * (div + 1));

		if (gen < 1 || gen > 65535)
			continue;

		calc_rate = clk / (gen * (div + 1));
		error = baudrate - calc_rate;
		if (error < 0)
			error *= -1;
		if (((error * 100) / baudrate) < 3)
			break;
	}

	writel(gen, priv->regs + CADENCE_UART_BAUD_GEN);
	writel(div, priv->regs + CADENCE_UART_BAUD_DIV);

	/* Soft-Reset Tx/Rx paths */
	writel(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES, priv->regs +
		CADENCE_UART_CONTROL);

	while (readl(priv->regs + CADENCE_UART_CONTROL) &
		(CADENCE_CTRL_RXRES | CADENCE_CTRL_TXRES))
		;

	/* Enable UART */
	writel(priv->devtype->ctrl, priv->regs + CADENCE_UART_CONTROL);

	return 0;
}

static int cadence_serial_init_port(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	cadence_serial_reset(cdev);

	/* Enable UART */
	writel(priv->devtype->ctrl, priv->regs + CADENCE_UART_CONTROL);
	writel(priv->devtype->mode, priv->regs + CADENCE_UART_MODE);

	return 0;
}

static void cadence_serial_putc(struct console_device *cdev, char c)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		CADENCE_STS_TFUL) != 0)
		;

	writel(c, priv->regs + CADENCE_UART_RXTXFIFO);
}

static int cadence_serial_tstc(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	return ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		 CADENCE_STS_REMPTY) == 0);
}

static int cadence_serial_getc(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while (!cadence_serial_tstc(cdev))
		;

	return readl(priv->regs + CADENCE_UART_RXTXFIFO);
}

static void cadence_serial_flush(struct console_device *cdev)
{
	struct cadence_serial_priv *priv = container_of(cdev,
					struct cadence_serial_priv, cdev);

	while ((readl(priv->regs + CADENCE_UART_CHANNEL_STS) &
		CADENCE_STS_TEMPTY) != 0)
		;
}

static int cadence_clocksource_clock_change(struct notifier_block *nb,
			unsigned long event, void *data)
{
	struct cadence_serial_priv *priv = container_of(nb,
					struct cadence_serial_priv, notify);

	cadence_serial_setbaudrate(&priv->cdev, priv->baudrate);

	return 0;
}

static int cadence_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct console_device *cdev;
	struct cadence_serial_priv *priv;
	struct cadence_serial_devtype_data *devtype;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));
	priv->devtype = devtype;
	cdev = &priv->cdev;
	dev->priv = priv;

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		ret = -ENODEV;
		goto err_free;
	}

	if (devtype->mode & CADENCE_MODE_CLK_REF_DIV)
		clk_set_rate(priv->clk, clk_get_rate(priv->clk) / 8);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	priv->regs = IOMEM(iores->start);

	cdev->dev = dev;
	cdev->tstc = cadence_serial_tstc;
	cdev->putc = cadence_serial_putc;
	cdev->getc = cadence_serial_getc;
	cdev->flush = cadence_serial_flush;
	cdev->setbrg = cadence_serial_setbaudrate;

	cadence_serial_init_port(cdev);

	console_register(cdev);
	priv->notify.notifier_call = cadence_clocksource_clock_change;
	clock_register_client(&priv->notify);

	return 0;

err_free:
	free(priv);
	return ret;
}

static __maybe_unused struct of_device_id cadence_serial_dt_ids[] = {
	{
		.compatible = "xlnx,xuartps",
		.data = &cadence_r1p08_data,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id cadence_serial_ids[] = {
	{
		.name = "cadence-uart",
		.driver_data = (unsigned long)&cadence_r1p08_data,
	}, {
		/* sentinel */
	},
};

static struct driver_d cadence_serial_driver = {
	.name   = "cadence_serial",
	.probe  = cadence_serial_probe,
	.of_compatible = DRV_OF_COMPAT(cadence_serial_dt_ids),
	.id_table = cadence_serial_ids,
};

static int cadence_serial_init(void)
{
	return platform_driver_register(&cadence_serial_driver);
}
console_initcall(cadence_serial_init);

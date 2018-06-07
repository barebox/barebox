/*
 * (c) 2004 Sascha Hauer <sascha@saschahauer.de>
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

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <notifier.h>
#include <io.h>
#include <of.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <serial/imx-uart.h>

/*
 * create default values for different platforms
 */
struct imx_serial_devtype_data {
	u32 ucr1_val;
	u32 ucr3_val;
	u32 ucr4_val;
	u32 uts;
	u32 onems;
};

static struct imx_serial_devtype_data imx1_data = {
	.ucr1_val = UCR1_UARTCLKEN,
	.ucr3_val = 0,
	.ucr4_val = UCR4_CTSTL_32 | UCR4_REF16,
	.uts = 0xd0,
	.onems = 0,
};

static struct imx_serial_devtype_data imx21_data = {
	.ucr1_val = 0,
	.ucr3_val = 0x700 | UCR3_RXDMUXSEL | UCR3_ADNIMP,
	.ucr4_val = UCR4_CTSTL_32,
	.uts = 0xb4,
	.onems = 0xb0,
};

struct imx_serial_priv {
	struct console_device cdev;
	int baudrate;
	int dte_mode;
	struct notifier_block notify;
	void __iomem *regs;
	struct clk *clk;
	struct imx_serial_devtype_data *devtype;
};

static int imx_serial_reffreq(struct imx_serial_priv *priv)
{
	ulong rfdiv;

	rfdiv = (readl(priv->regs + UFCR) >> 7) & 7;
	rfdiv = rfdiv < 6 ? 6 - rfdiv : 7;

	return clk_get_rate(priv->clk) / rfdiv;
}

/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
static int imx_serial_init_port(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	void __iomem *regs = priv->regs;
	uint32_t val;

	writel(priv->devtype->ucr1_val, regs + UCR1);
	writel(UCR2_WS | UCR2_IRTS, regs + UCR2);
	writel(priv->devtype->ucr3_val, regs + UCR3);
	writel(priv->devtype->ucr4_val, regs + UCR4);
	writel(0x0000002B, regs + UESC);
	writel(0, regs + UTIM);
	writel(0, regs + UBIR);
	writel(0, regs + UBMR);
	writel(0, regs + priv->devtype->uts);

	/* Configure FIFOs */
	val = 0xa81;
	if (priv->dte_mode)
		val |= UFCR_DCEDTE;

	writel(val, regs + UFCR);


	if (priv->devtype->onems)
		writel(imx_serial_reffreq(priv) / 1000, regs + priv->devtype->onems);

	/* Enable FIFOs */
	val = readl(regs + UCR2);
	val |= UCR2_SRST | UCR2_RXEN | UCR2_TXEN;
	writel(val, regs + UCR2);

  	/* Clear status flags */
	val = readl(regs + USR2);
	val |= USR2_ADET | USR2_DTRF | USR2_IDLE | USR2_IRINT | USR2_WAKE |
	       USR2_RTSF | USR2_BRCD | USR2_ORE | USR2_RDR;
	writel(val, regs + USR2);

  	/* Clear status flags */
	val = readl(regs + USR1);
	val |= USR1_PARITYERR | USR1_RTSD | USR1_ESCF | USR1_FRAMERR | USR1_AIRINT |
	       USR1_AWAKE;
	writel(val, regs + USR1);

	return 0;
}

static void imx_serial_putc(struct console_device *cdev, char c)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	/* Wait for Tx FIFO not full */
	while (readl(priv->regs + priv->devtype->uts) & UTS_TXFULL);

        writel(c, priv->regs + URTX0);
}

static int imx_serial_tstc(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	/* If receive fifo is empty, return false */
	if (readl(priv->regs + priv->devtype->uts) & UTS_RXEMPTY)
		return 0;
	return 1;
}

static int imx_serial_getc(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	unsigned char ch;

	while (readl(priv->regs + priv->devtype->uts) & UTS_RXEMPTY);

	ch = readl(priv->regs + URXD0);

	return ch;
}

static void imx_serial_flush(struct console_device *cdev)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);

	while (!(readl(priv->regs + USR2) & USR2_TXDC));
}

static int imx_serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	struct imx_serial_priv *priv = container_of(cdev,
					struct imx_serial_priv, cdev);
	void __iomem *regs = priv->regs;
	uint32_t val;
	uint32_t ucr1 = readl(regs + UCR1);

	/* disable UART */
	val = readl(regs + UCR1);
	val &= ~UCR1_UARTEN;
	writel(val, regs + UCR1);

	/* Set the numerator value minus one of the BRM ratio */
	writel(baudrate_to_ubir(baudrate), regs + UBIR);
	/* Set the denominator value minus one of the BRM ratio    */
	writel(refclock_to_ubmr(imx_serial_reffreq(priv)), regs + UBMR);

	writel(ucr1, regs + UCR1);

	priv->baudrate = baudrate;

	return 0;
}

static int imx_clocksource_clock_change(struct notifier_block *nb,
			unsigned long event, void *data)
{
	struct imx_serial_priv *priv = container_of(nb,
				struct imx_serial_priv, notify);

	imx_serial_setbaudrate(&priv->cdev, priv->baudrate);

        return 0;
}

static int imx_serial_probe(struct device_d *dev)
{
	struct resource *iores;
	struct console_device *cdev;
	struct imx_serial_priv *priv;
	uint32_t val;
	struct imx_serial_devtype_data *devtype;
	int ret;
	const char *devname;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));
	priv->devtype = devtype;
	cdev = &priv->cdev;
	dev->priv = priv;

	priv->clk = clk_get(dev, "per");
	if (IS_ERR(priv->clk)) {
		ret = PTR_ERR(priv->clk);
		goto err_free;
	}
	clk_enable(priv->clk);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	cdev->dev = dev;
	cdev->tstc = imx_serial_tstc;
	cdev->putc = imx_serial_putc;
	cdev->getc = imx_serial_getc;
	cdev->flush = imx_serial_flush;
	cdev->setbrg = imx_serial_setbaudrate;
	cdev->linux_console_name = "ttymxc";
	if (dev->device_node) {
		devname = of_alias_get(dev->device_node);
		if (devname) {
			cdev->devname = xstrdup(devname);
			cdev->devid = DEVICE_ID_SINGLE;
		}
	}

	if (of_property_read_bool(dev->device_node, "fsl,dte-mode"))
		priv->dte_mode = 1;

	imx_serial_init_port(cdev);

	/* Enable UART */
	val = readl(priv->regs + UCR1);
	val |= UCR1_UARTEN;
	writel(val, priv->regs + UCR1);

	console_register(cdev);
	priv->notify.notifier_call = imx_clocksource_clock_change;
	clock_register_client(&priv->notify);

	return 0;

err_free:
	free(priv);

	return ret;
}

static __maybe_unused struct of_device_id imx_serial_dt_ids[] = {
	{
		.compatible = "fsl,imx1-uart",
		.data = &imx1_data,
	}, {
		.compatible = "fsl,imx21-uart",
		.data = &imx21_data,
	}, {
		.compatible = "fsl,imx6ul-uart",
		.data = &imx21_data,
	}, {
		.compatible = "fsl,imx7d-uart",
		.data = &imx21_data,
	}, {
		.compatible = "fsl,imx8mq-uart",
		.data = &imx21_data,
	}, {
		/* sentinel */
	}
};

static struct platform_device_id imx_serial_ids[] = {
	{
		.name = "imx1-uart",
		.driver_data = (unsigned long)&imx1_data,
	}, {
		.name = "imx21-uart",
		.driver_data = (unsigned long)&imx21_data,
	}, {
		/* sentinel */
	},
};

static struct driver_d imx_serial_driver = {
	.name   = "imx_serial",
	.probe  = imx_serial_probe,
	.of_compatible = DRV_OF_COMPAT(imx_serial_dt_ids),
	.id_table = imx_serial_ids,
};
console_platform_driver(imx_serial_driver);

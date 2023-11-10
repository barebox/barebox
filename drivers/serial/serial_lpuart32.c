// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2023 Pengutronix
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
#include <serial/lpuart32.h>

struct lpuart32_devtype_data {
	unsigned int reg_offs;
};

struct lpuart32 {
	struct console_device cdev;
	int baudrate;
	int dte_mode;
	struct notifier_block notify;
	struct resource *io;
	void __iomem *base;
	struct clk *clk;
};

static struct lpuart32 *cdev_to_lpuart32(struct console_device *cdev)
{
	return container_of(cdev, struct lpuart32, cdev);
}

static void lpuart32_enable(struct lpuart32 *lpuart32)
{
	writel(LPUART32_UARTCTRL_TE | LPUART32_UARTCTRL_RE,
	       lpuart32->base + LPUART32_UARTCTRL);
}

static void lpuart32_disable(struct lpuart32 *lpuart32)
{
	writel(0, lpuart32->base + LPUART32_UARTCTRL);
}

static int lpuart32_serial_setbaudrate(struct console_device *cdev,
				     int baudrate)
{
	struct lpuart32 *lpuart32 = cdev_to_lpuart32(cdev);

	lpuart32_disable(lpuart32);

	/*
	 * We treat baudrate of 0 as a request to disable UART
	 */
	if (baudrate) {
		lpuart32_setbrg(lpuart32->base, clk_get_rate(lpuart32->clk),
			      baudrate);
		lpuart32_enable(lpuart32);
	}

	lpuart32->baudrate = baudrate;

	return 0;
}

static int lpuart32_serial_getc(struct console_device *cdev)
{
	struct lpuart32 *lpuart32 = cdev_to_lpuart32(cdev);

	while (!(readl(lpuart32->base + LPUART32_UARTSTAT) & LPUART32_UARTSTAT_RDRF));

	return readl(lpuart32->base + LPUART32_UARTDATA) & 0xff;
}

static void lpuart32_serial_putc(struct console_device *cdev, char c)
{
	struct lpuart32 *lpuart32 = cdev_to_lpuart32(cdev);

	lpuart32_putc(lpuart32->base, c);
}

/* Test whether a character is in the RX buffer */
static int lpuart32_serial_tstc(struct console_device *cdev)
{
	struct lpuart32 *lpuart32 = cdev_to_lpuart32(cdev);

	return readl(lpuart32->base + LPUART32_UARTSTAT) & LPUART32_UARTSTAT_RDRF;
}

static void lpuart32_serial_flush(struct console_device *cdev)
{
}

static int lpuart32_serial_probe(struct device *dev)
{
	int ret;
	struct console_device *cdev;
	struct lpuart32 *lpuart32;
	const char *devname;
	struct lpuart32_devtype_data *devtype;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	lpuart32 = xzalloc(sizeof(*lpuart32));
	cdev = &lpuart32->cdev;
	dev->priv = lpuart32;

	lpuart32->io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(lpuart32->io)) {
		ret = PTR_ERR(lpuart32->io);
		goto err_free;
	}
	lpuart32->base = IOMEM(lpuart32->io->start) + devtype->reg_offs;

	lpuart32->clk = clk_get(dev, NULL);
	if (IS_ERR(lpuart32->clk)) {
		ret = PTR_ERR(lpuart32->clk);
		dev_err(dev, "Failed to get UART clock %d\n", ret);
		goto io_release;
	}

	ret = clk_enable(lpuart32->clk);
	if (ret) {
		dev_err(dev, "Failed to enable UART clock %d\n", ret);
		goto io_release;
	}

	cdev->dev    = dev;
	cdev->tstc   = lpuart32_serial_tstc;
	cdev->putc   = lpuart32_serial_putc;
	cdev->getc   = lpuart32_serial_getc;
	cdev->flush  = lpuart32_serial_flush;
	cdev->setbrg = lpuart32_serial_setbaudrate;

	if (dev->of_node) {
		devname = of_alias_get(dev->of_node);
		if (devname) {
			cdev->devname = xstrdup(devname);
			cdev->devid   = DEVICE_ID_SINGLE;
		}
	}

	cdev->linux_console_name = "ttyLP";
	cdev->linux_earlycon_name = "lpuart";
	cdev->phys_base = lpuart32->base;

	lpuart32_setup(lpuart32->base, clk_get_rate(lpuart32->clk));

	ret = console_register(cdev);
	if (!ret)
		return 0;

	clk_put(lpuart32->clk);
io_release:
	release_region(lpuart32->io);
err_free:
	free(lpuart32);

	return ret;
}

static struct lpuart32_devtype_data imx7ulp_data = {
	.reg_offs = 0x10,
};

static struct of_device_id lpuart32_serial_dt_ids[] = {
	{
		.compatible = "fsl,imx7ulp-lpuart",
		.data = &imx7ulp_data,
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, lpuart32_serial_dt_ids);

static struct driver lpuart32_serial_driver = {
	.name   = "lpuart32-serial",
	.probe  = lpuart32_serial_probe,
	.of_compatible = DRV_OF_COMPAT(lpuart32_serial_dt_ids),
};
console_platform_driver(lpuart32_serial_driver);

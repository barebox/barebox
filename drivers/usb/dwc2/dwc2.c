// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2012 Oleksandr Tymoshenko <gonzo@freebsd.org>
 * Copyright (C) 2014 Marek Vasut <marex@denx.de>
 *
 * Copied from u-Boot
 */
#include <common.h>
#include <of.h>
#include <dma.h>
#include <init.h>
#include <errno.h>
#include <driver.h>
#include <linux/clk.h>

#include "dwc2.h"

static void dwc2_uninit_common(struct dwc2 *dwc2)
{
	uint32_t hprt0;

	hprt0 = dwc2_readl(dwc2, HPRT0);

	/* Put everything in reset. */
	hprt0 &= ~(HPRT0_ENA | HPRT0_ENACHG | HPRT0_CONNDET | HPRT0_OVRCURRCHG);
	hprt0 |= HPRT0_RST;

	dwc2_writel(dwc2, hprt0, HPRT0);
}

static int dwc2_probe(struct device_d *dev)
{
	struct resource *iores;
	struct dwc2 *dwc2;
	int ret;

	dwc2 = xzalloc(sizeof(*dwc2));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	dwc2->regs = IOMEM(iores->start);
	dwc2->dev = dev;

	ret = dwc2_core_snpsid(dwc2);
	if (ret)
		goto error;

	/*
	 * Reset before dwc2_get_hwparams() then it could get power-on real
	 * reset value form registers.
	 */
	ret = dwc2_core_reset(dwc2);
	if (ret)
		goto error;

	/* Detect config values from hardware */
	dwc2_get_hwparams(dwc2);

	dwc2_get_dr_mode(dwc2);

	dwc2_set_default_params(dwc2);

	dma_set_mask(dev, DMA_BIT_MASK(32));
	dev->priv = dwc2;

	ret = dwc2_register_host(dwc2);
error:
	return ret;
}

static void dwc2_remove(struct device_d *dev)
{
	struct dwc2 *dwc2 = dev->priv;

	dwc2_uninit_common(dwc2);
}

static const struct of_device_id dwc2_platform_dt_ids[] = {
	{ .compatible = "brcm,bcm2835-usb", },
	{ .compatible = "brcm,bcm2708-usb", },
	{ .compatible = "snps,dwc2", },
	{ }
};

static struct driver_d dwc2_driver = {
	.name	= "dwc2",
	.probe	= dwc2_probe,
	.remove = dwc2_remove,
	.of_compatible = DRV_OF_COMPAT(dwc2_platform_dt_ids),
};
device_platform_driver(dwc2_driver);

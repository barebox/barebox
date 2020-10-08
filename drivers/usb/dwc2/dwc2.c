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

static int dwc2_set_mode(void *ctx, enum usb_dr_mode mode)
{
	struct dwc2 *dwc2 = ctx;
	int ret = -ENODEV;

	if (mode == USB_DR_MODE_HOST || mode == USB_DR_MODE_OTG) {
		if (IS_ENABLED(CONFIG_USB_DWC2_HOST))
			ret = dwc2_register_host(dwc2);
		else
			dwc2_err(dwc2, "Host support not available\n");
	}
	if (mode == USB_DR_MODE_PERIPHERAL || mode == USB_DR_MODE_OTG) {
		if (IS_ENABLED(CONFIG_USB_DWC2_GADGET))
			ret = dwc2_gadget_init(dwc2);
		else
			dwc2_err(dwc2, "Peripheral support not available\n");
	}

	return ret;
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

	ret = dwc2_get_dr_mode(dwc2);

	dwc2_set_default_params(dwc2);

	dma_set_mask(dev, DMA_BIT_MASK(32));
	dev->priv = dwc2;

	if (dwc2->dr_mode == USB_DR_MODE_OTG)
		ret = usb_register_otg_device(dwc2->dev, dwc2_set_mode, dwc2);
	else
		ret = dwc2_set_mode(dwc2, dwc2->dr_mode);

error:
	return ret;
}

static void dwc2_remove(struct device_d *dev)
{
	struct dwc2 *dwc2 = dev->priv;

	dwc2_host_uninit(dwc2);
	dwc2_gadget_uninit(dwc2);
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

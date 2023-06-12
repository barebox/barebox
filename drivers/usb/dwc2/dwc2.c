// SPDX-License-Identifier: GPL-2.0-or-later
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
#include <linux/reset.h>
#include <of_device.h>

#include "dwc2.h"

static void dwc2_set_stm32mp15_fsotg_params(struct dwc2 *dwc2)
{
	struct dwc2_core_params *p = &dwc2->params;

	p->otg_cap &= ~(DWC2_CAP_PARAM_HNP_SRP_CAPABLE
			 | DWC2_CAP_PARAM_SRP_ONLY_CAPABLE);
	p->otg_cap |= DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE;
	p->speed = DWC2_SPEED_PARAM_FULL;
	p->host_rx_fifo_size = 128;
	p->host_nperio_tx_fifo_size = 96;
	p->host_perio_tx_fifo_size = 96;
	p->max_packet_count = 256;
	p->phy_type = DWC2_PHY_TYPE_PARAM_FS;
	p->i2c_enable = false;
	p->activate_stm_fs_transceiver = true;
	p->ahbcfg = GAHBCFG_HBSTLEN_INCR16 << GAHBCFG_HBSTLEN_SHIFT;
	p->power_down = DWC2_POWER_DOWN_PARAM_NONE;
	p->host_support_fs_ls_low_power = true;
	p->host_ls_low_power_phy_clk = true;
}

static void dwc2_set_stm32mp15_hsotg_params(struct dwc2 *dwc2)
{
	struct dwc2_core_params *p = &dwc2->params;

	p->otg_cap &= ~(DWC2_CAP_PARAM_HNP_SRP_CAPABLE
			 | DWC2_CAP_PARAM_SRP_ONLY_CAPABLE);
	p->otg_cap |= DWC2_CAP_PARAM_NO_HNP_SRP_CAPABLE;
	p->host_rx_fifo_size = 440;
	p->host_nperio_tx_fifo_size = 256;
	p->host_perio_tx_fifo_size = 256;
	p->ahbcfg = GAHBCFG_HBSTLEN_INCR16 << GAHBCFG_HBSTLEN_SHIFT;
	p->power_down = DWC2_POWER_DOWN_PARAM_NONE;
	p->lpm = false;
	p->lpm_clock_gating = false;
	p->besl = false;
	p->hird_threshold_en = false;
}

static int dwc2_set_mode(void *ctx, enum usb_dr_mode mode)
{
	struct dwc2 *dwc2 = ctx;
	int ret = -ENODEV;
	int oldmode = dwc2->dr_mode;

	dwc2->dr_mode = mode;

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

	if (ret)
		dwc2->dr_mode = oldmode;

	return ret;
}

typedef void (*set_params_cb)(struct dwc2 *dwc2);

static int dwc2_probe(struct device *dev)
{
	struct resource *iores;
	struct dwc2 *dwc2;
	set_params_cb set_params;
	int ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	dwc2 = xzalloc(sizeof(*dwc2));
	dwc2->regs = IOMEM(iores->start);
	dwc2->dev = dev;

	dwc2->clk = clk_get(dev, "otg");
	if (IS_ERR(dwc2->clk)) {
		ret = PTR_ERR(dwc2->clk);
		dev_err(dev, "Failed to get USB clock %d\n", ret);
		goto release_region;
	}

	ret = clk_enable(dwc2->clk);
	if (ret)
		goto clk_put;

	ret = device_reset_us(dev, 2);
	if (ret)
		goto clk_disable;

	dwc2->phy = phy_optional_get(dev, "usb2-phy");
	if (IS_ERR(dwc2->phy)) {
		ret = PTR_ERR(dwc2->phy);
		goto clk_disable;
	}

	ret = phy_power_on(dwc2->phy);
	if (ret)
		goto clk_disable;

	ret = phy_init(dwc2->phy);
	if (ret)
		goto phy_power_off;

	ret = dwc2_check_core_version(dwc2);
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
	if (ret)
		goto error;

	dwc2_set_default_params(dwc2);
	dwc2_get_device_properties(dwc2);

	set_params = of_device_get_match_data(dev);
	if (set_params)
		set_params(dwc2);

	dma_set_mask(dev, DMA_BIT_MASK(32));
	dev->priv = dwc2;

	if (dwc2->dr_mode == USB_DR_MODE_OTG)
		ret = usb_register_otg_device(dwc2->dev, dwc2_set_mode, dwc2);
	else
		ret = dwc2_set_mode(dwc2, dwc2->dr_mode);

	if (ret)
		goto error;

	return 0;
error:
	phy_exit(dwc2->phy);
phy_power_off:
	phy_power_off(dwc2->phy);
clk_disable:
	clk_disable(dwc2->clk);
clk_put:
	clk_put(dwc2->clk);
release_region:
	release_region(iores);

	free(dwc2);

	return ret;
}

static void dwc2_remove(struct device *dev)
{
	struct dwc2 *dwc2 = dev->priv;

	dwc2_host_uninit(dwc2);
	dwc2_gadget_uninit(dwc2);

	phy_exit(dwc2->phy);
	phy_power_off(dwc2->phy);
}

static const struct of_device_id dwc2_platform_dt_ids[] = {
	{ .compatible = "brcm,bcm2835-usb", },
	{ .compatible = "brcm,bcm2708-usb", },
	{ .compatible = "snps,dwc2", },
	{ .compatible = "st,stm32mp15-fsotg",
	  .data = dwc2_set_stm32mp15_fsotg_params },
	{ .compatible = "st,stm32mp15-hsotg",
	  .data = dwc2_set_stm32mp15_hsotg_params },
	{ }
};
MODULE_DEVICE_TABLE(of, dwc2_platform_dt_ids);

static struct driver dwc2_driver = {
	.name	= "dwc2",
	.probe	= dwc2_probe,
	.remove = dwc2_remove,
	.of_compatible = DRV_OF_COMPAT(dwc2_platform_dt_ids),
};
device_platform_driver(dwc2_driver);

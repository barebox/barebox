/*
 * (C) Copyright 2010
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
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

/*
 * Designware ethernet IP driver for u-boot
 */

#include <common.h>
#include <init.h>
#include "designware.h"

static struct dw_eth_drvdata dwmac_370a_drvdata = {
	.enh_desc = 1,
};

static int dwc_ether_probe(struct device_d *dev)
{
	struct dw_eth_dev *dwc;

	dwc = dwc_drv_probe(dev);
	if (IS_ERR(dwc))
		return PTR_ERR(dwc);

	return 0;
}

static __maybe_unused struct of_device_id dwc_ether_compatible[] = {
	{
		.compatible = "snps,dwmac-3.70a",
		.data = &dwmac_370a_drvdata,
	}, {
		.compatible = "snps,dwmac-3.72a",
		.data = &dwmac_370a_drvdata,
	}, {
		/* sentinel */
	}
};

static struct driver_d dwc_ether_driver = {
	.name = "designware_eth",
	.probe = dwc_ether_probe,
	.of_compatible = DRV_OF_COMPAT(dwc_ether_compatible),
};
device_platform_driver(dwc_ether_driver);

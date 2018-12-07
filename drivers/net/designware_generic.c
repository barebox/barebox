// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2010
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
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
	.remove	= dwc_drv_remove,
	.of_compatible = DRV_OF_COMPAT(dwc_ether_compatible),
};
device_platform_driver(dwc_ether_driver);

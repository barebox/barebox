// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <mfd/syscon.h>
#include <soc/starfive/sysmain.h>
#include "designware.h"

/*
 * GMAC_GTXCLK
 * bit         name                 access  default         description
 * [31]        _gmac_gtxclk enable  RW      0x0             "1:enable; 0:disable"
 * [30]        reserved             -       0x0             reserved
 * [29:8]      reserved             -       0x0             reserved
 * [7:0]       gmac_gtxclk ratio    RW      0x4             divider value
 *
 * 1000M: gtxclk@125M => 500/125 = 0x4
 * 100M:  gtxclk@25M  => 500/25  = 0x14
 * 10M:   gtxclk@2.5M => 500/2.5 = 0xc8
 */

#define CLKGEN_BASE                    0x11800000
#define CLKGEN_GMAC_GTXCLK_OFFSET      0x1EC
#define CLKGEN_GMAC_GTXCLK_ADDR        (CLKGEN_BASE + CLKGEN_GMAC_GTXCLK_OFFSET)


#define CLKGEN_125M_DIV                0x4
#define CLKGEN_25M_DIV                 0x14
#define CLKGEN_2_5M_DIV                0xc8

static void dwmac_fixed_speed(int speed)
{
	/* TODO: move this into clk driver */
	void __iomem *addr = IOMEM(CLKGEN_GMAC_GTXCLK_ADDR);
	u32 value;

	value = readl(addr) & (~0x000000FF);

	switch (speed) {
	case SPEED_1000: value |= CLKGEN_125M_DIV; break;
	case SPEED_100:  value |= CLKGEN_25M_DIV;  break;
	case SPEED_10:   value |= CLKGEN_2_5M_DIV; break;
	default: return;
	}

	writel(value, addr);
}

static struct dw_eth_drvdata starfive_drvdata = {
	.enh_desc = 1,
	.fix_mac_speed = dwmac_fixed_speed,
};

static int starfive_dwc_ether_probe(struct device *dev)
{
	struct dw_eth_dev *dwc;
	struct regmap *regmap;
	int ret;
	struct clk_bulk_data clks[] = {
		{ .id = "stmmaceth" },
		{ .id = "ptp_ref" },
		{ .id = "tx" },
	};

	regmap = syscon_regmap_lookup_by_phandle(dev->of_node,
						 "starfive,sysmain");
	if (IS_ERR(regmap)) {
		dev_err(dev, "Could not get starfive,sysmain node\n");
		return PTR_ERR(regmap);
	}

	ret = clk_bulk_get(dev, ARRAY_SIZE(clks), clks);
	if (ret)
		return ret;

	ret = clk_bulk_enable(ARRAY_SIZE(clks), clks);
	if (ret < 0)
		return ret;

	ret = device_reset(dev);
	if (ret)
		return ret;

	dwc = dwc_drv_probe(dev);
	if (IS_ERR(dwc))
		return PTR_ERR(dwc);

	if (phy_interface_mode_is_rgmii(dwc->interface)) {
		regmap_update_bits(regmap, SYSMAIN_GMAC_PHY_INTF_SEL, 0x7, 0x1);
		regmap_write(regmap, SYSMAIN_GMAC_GTXCLK_DLYCHAIN_SEL, 0x4);
	}

	return 0;
}

static struct of_device_id starfive_dwc_ether_compatible[] = {
	{ .compatible = "starfive,stmmac", .data = &starfive_drvdata },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, starfive_dwc_ether_compatible);

static struct driver starfive_dwc_ether_driver = {
	.name = "starfive-designware_eth",
	.probe = starfive_dwc_ether_probe,
	.of_compatible = starfive_dwc_ether_compatible,
};
device_platform_driver(starfive_dwc_ether_driver);

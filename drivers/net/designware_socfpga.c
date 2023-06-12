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
#include <io.h>
#include <net.h>
#include <of_net.h>
#include <linux/reset.h>
#include <mfd/syscon.h>
#include "designware.h"

#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII 	0x0
#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII		0x1
#define SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RMII		0x2
#define SYSMGR_EMACGRP_CTRL_PHYSEL0_LSB			0
#define SYSMGR_EMACGRP_CTRL_PHYSEL1_LSB			2
#define SYSMGR_EMACGRP_CTRL_PHYSEL_MASK			0x00000003
#define SYSMGR_EMACGRP_CTRL_PTP_REF_CLK_MASK		0x00000010
#define SYSMGR_GEN10_EMACGRP_CTRL_PTP_REF_CLK_MASK	0x00000100

#define SYSMGR_FPGAGRP_MODULE				0x00000028
#define SYSMGR_FPGAGRP_MODULE_EMAC			0x00000004
#define SYSMGR_FPGAINTF_EMAC_REG			0x00000070
#define SYSMGR_FPGAINTF_EMAC_BIT			0x1

struct socfpga_dwc_dev;
struct socfpga_dwmac_ops {
	int (*set_phy_mode)(struct socfpga_dwc_dev *dwmac_priv);
};

struct socfpga_dwc_dev {
	struct dw_eth_dev *priv;
	u32		   reg_offset;
	u32		   reg_shift;
	void __iomem	  *sys_mgr_base;
	bool		   f2h_ptp_ref_clk;
	const struct	   socfpga_dwmac_ops *ops;
};

static int socfpga_set_phy_mode_common(int phymode, u32 *val)
{
	switch (phymode) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		*val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
		break;
	case PHY_INTERFACE_MODE_MII:
	case PHY_INTERFACE_MODE_GMII:
	case PHY_INTERFACE_MODE_SGMII:
		*val = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII;
		break;
	default:
		return -EINVAL;
	}
	return 0;
};

static int socfpga_gen5_set_phy_mode(struct socfpga_dwc_dev *dwc_dev)
{
	struct dw_eth_dev *eth_dev = dwc_dev->priv;
	int phymode = eth_dev->interface;
	u32 reg_offset = dwc_dev->reg_offset;
	u32 reg_shift = dwc_dev->reg_shift;
	u32 ctrl, val;

	if (socfpga_set_phy_mode_common(phymode, &val)) {
		dev_err(&eth_dev->netdev.dev, "bad phy mode %d\n", phymode);
		return -EINVAL;
	}

	/* Assert reset to the enet controller before changing the phy mode */
	reset_control_assert(eth_dev->rst);

	ctrl = readl(dwc_dev->sys_mgr_base + reg_offset);
	ctrl &= ~(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK << reg_shift);
	ctrl |= val << reg_shift;

	if (dwc_dev->f2h_ptp_ref_clk ||
	    phymode == PHY_INTERFACE_MODE_MII ||
	    phymode == PHY_INTERFACE_MODE_GMII ||
	    phymode == PHY_INTERFACE_MODE_SGMII) {
		u32 module;

		ctrl |= SYSMGR_EMACGRP_CTRL_PTP_REF_CLK_MASK << (reg_shift / 2);
		module = readl(dwc_dev->sys_mgr_base + SYSMGR_FPGAGRP_MODULE);
		module |= (SYSMGR_FPGAGRP_MODULE_EMAC << (reg_shift / 2));

		writel(module, dwc_dev->sys_mgr_base + SYSMGR_FPGAGRP_MODULE);
	} else {
		ctrl &= ~(SYSMGR_EMACGRP_CTRL_PTP_REF_CLK_MASK << (reg_shift / 2));
	}

	writel(ctrl, dwc_dev->sys_mgr_base + reg_offset);

	/* Deassert reset for the phy configuration to be sampled by
	 * the enet controller, and operation to start in requested mode
	 */
	reset_control_deassert(eth_dev->rst);

	return 0;
}

static int socfpga_gen10_set_phy_mode(struct socfpga_dwc_dev *dwc_dev)
{
	struct dw_eth_dev *eth_dev = dwc_dev->priv;
	int phymode = eth_dev->interface;
	u32 reg_offset = dwc_dev->reg_offset;
	u32 reg_shift = dwc_dev->reg_shift;
	u32 ctrl, val;

	if (socfpga_set_phy_mode_common(phymode, &val)) {
		dev_err(&eth_dev->netdev.dev, "bad phy mode %d\n", phymode);
		return -EINVAL;
	}

	/* Assert reset to the enet controller before changing the phy mode */
	reset_control_assert(eth_dev->rst);

	ctrl = readl(dwc_dev->sys_mgr_base + reg_offset);
	ctrl &= ~(SYSMGR_EMACGRP_CTRL_PHYSEL_MASK << reg_shift);
	ctrl |= val << reg_shift;

	if (dwc_dev->f2h_ptp_ref_clk ||
	    phymode == PHY_INTERFACE_MODE_MII ||
	    phymode == PHY_INTERFACE_MODE_GMII ||
	    phymode == PHY_INTERFACE_MODE_SGMII) {
		u32 module;

		ctrl |= SYSMGR_GEN10_EMACGRP_CTRL_PTP_REF_CLK_MASK;
		module = readl(dwc_dev->sys_mgr_base + SYSMGR_FPGAINTF_EMAC_REG);
		module |= (SYSMGR_FPGAINTF_EMAC_BIT << reg_shift);

		writel(module, dwc_dev->sys_mgr_base + SYSMGR_FPGAINTF_EMAC_REG);
	} else {
		ctrl &= ~SYSMGR_GEN10_EMACGRP_CTRL_PTP_REF_CLK_MASK;
	}

	writel(ctrl, dwc_dev->sys_mgr_base + reg_offset);

	/* Deassert reset for the phy configuration to be sampled by
	 * the enet controller, and operation to start in requested mode
	 */
	reset_control_deassert(eth_dev->rst);

	return 0;
}


static int socfpga_dwc_probe_dt(struct device *dev,
				struct socfpga_dwc_dev *priv)
{
	u32 reg_offset, reg_shift;
	int ret;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return -ENODEV;

	ret = of_property_read_u32_index(dev->of_node, "altr,sysmgr-syscon",
					 1, &reg_offset);
	if (ret) {
		dev_err(dev, "Could not read reg_offset from sysmgr-syscon! Please update the devicetree.\n");

		return -EINVAL;
	}

	ret = of_property_read_u32_index(dev->of_node, "altr,sysmgr-syscon",
					 2, &reg_shift);
	if (ret) {
		dev_err(dev, "Could not read reg_shift from sysmgr-syscon! Please update the devicetree.\n");
		return -EINVAL;
	}

	priv->f2h_ptp_ref_clk = of_property_read_bool(dev->of_node,
						      "altr,f2h_ptp_ref_clk");

	priv->reg_offset = reg_offset;
	priv->reg_shift = reg_shift;

	return 0;
}

static int socfpga_dwc_ether_probe(struct device *dev)
{
	struct socfpga_dwc_dev *dwc_dev;
	struct dw_eth_dev *priv;
	struct dw_eth_drvdata *drvdata;
	int ret;

	dwc_dev = xzalloc(sizeof(*dwc_dev));

	ret = dev_get_drvdata(dev, (const void **)&drvdata);
	if (ret)
		return ret;

	if (drvdata && drvdata->priv)
		dwc_dev->ops = (struct socfpga_dwmac_ops *)drvdata->priv;
	else
		return -EINVAL;

        priv = dwc_drv_probe(dev);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	priv->rst = reset_control_get(dev, NULL);
	if (IS_ERR(priv->rst)) {
		dev_err(dev, "Invalid reset lines.\n");
		return PTR_ERR(priv->rst);
	}

	dwc_dev->priv = priv;

	dwc_dev->sys_mgr_base = syscon_base_lookup_by_phandle(dev->of_node,
							      "altr,sysmgr-syscon");
	if (IS_ERR(dwc_dev->sys_mgr_base)) {
		dev_err(dev, "Could not get sysmgr-syscon node\n");
		return PTR_ERR(dwc_dev->sys_mgr_base);
	}

	ret = socfpga_dwc_probe_dt(dev, dwc_dev);
	if (ret)
		return ret;

	return dwc_dev->ops->set_phy_mode(dwc_dev);
}

static struct socfpga_dwmac_ops socfpga_gen5_ops = {
	.set_phy_mode = socfpga_gen5_set_phy_mode,
};

static const struct dw_eth_drvdata socfpga_gen5_drvdata = {
	.enh_desc = 1,
	.priv = &socfpga_gen5_ops,
};

static struct socfpga_dwmac_ops socfpga_gen10_ops = {
	.set_phy_mode = socfpga_gen10_set_phy_mode,
};
static const struct dw_eth_drvdata socfpga_gen10_drvdata = {
	.enh_desc = 1,
	.priv = &socfpga_gen10_ops,
};

static __maybe_unused struct of_device_id socfpga_dwc_ether_compatible[] = {
	{
		.compatible = "altr,socfpga-stmmac",
		.data = &socfpga_gen5_drvdata,
	},
	{
		.compatible = "altr,socfpga-stmmac-a10-s10",
		.data = &socfpga_gen10_drvdata,
	},
	{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, socfpga_dwc_ether_compatible);

static struct driver socfpga_dwc_ether_driver = {
	.name = "socfpga_designware_eth",
	.probe = socfpga_dwc_ether_probe,
	.remove	= dwc_drv_remove,
	.of_compatible = DRV_OF_COMPAT(socfpga_dwc_ether_compatible),
};
device_platform_driver(socfpga_dwc_ether_driver);

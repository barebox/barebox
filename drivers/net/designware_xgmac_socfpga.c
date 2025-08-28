// SPDX-License-Identifier: GPL-2.0
/*
 *
 * Copyright (c) 2023, Intel Corporation
 */
#include <common.h>
#include <init.h>
#include <errno.h>
#include <malloc.h>
#include <net.h>
#include <linux/reset.h>
#include <linux/phy.h>
#include <mfd/syscon.h>
#include <linux/clk.h>
#include <mach/socfpga/soc64-system-manager.h>
#include <mach/socfpga/secure_reg_helper.h>

#include "designware_xgmac.h"

#define SOCFPGA_XGMAC_SYSCON_ARG_COUNT 2

static int dwxgmac_socfpga_do_setphy(struct device *dev, u32 modereg)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	u32 modemask = SYSMGR_EMACGRP_CTRL_PHYSEL_MASK;
	u32 index = (xgmac->reg_offset - SYSMGR_SOC64_EMAC0) >> 2;
	u32 id = SOCFPGA_SECURE_REG_SYSMGR_SOC64_EMAC0 + index;
	int ret;

	ret = socfpga_secure_reg_update32(id, modemask, modereg);
	if (ret) {
		dev_err(dev, "Failed to set PHY register via SMC call\n");
		return ret;
	}

	return 0;
}

static int xgmac_probe_resources_socfpga(struct device *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);
	phy_interface_t interface;
	int ret;
	u32 modereg;

	interface = xgmac->interface;

	switch (interface) {
	case PHY_INTERFACE_MODE_MII:
	case PHY_INTERFACE_MODE_GMII:
		modereg = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_GMII_MII;
		break;
	case PHY_INTERFACE_MODE_RMII:
		modereg = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RMII;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
		modereg = SYSMGR_EMACGRP_CTRL_PHYSEL_ENUM_RGMII;
		break;
	default:
		dev_err(dev, "Unsupported PHY mode\n");
		return -EINVAL;
	}

	/* Get PHY syscon */
	ret = of_property_read_u32_index(dev->of_node, "altr,sysmgr-syscon",
					 1, &xgmac->reg_offset);
	if (ret) {
		dev_err(dev, "Could not read reg_offset from sysmgr-syscon! Please update the devicetree.\n");

		return -EINVAL;
	}

	ret = of_property_read_u32_index(dev->of_node, "altr,sysmgr-syscon",
					 2, &xgmac->reg_shift);
	if (ret) {
		dev_err(dev, "Could not read reg_shift from sysmgr-syscon! Please update the devicetree.\n");
		return -EINVAL;
	}

	xgmac->rst = reset_control_get(dev, "stmmaceth");
	if (IS_ERR(xgmac->rst)) {
		dev_err(dev, "Invalid reset line 'stmmaceth'.\n");
		return PTR_ERR(xgmac->rst);
	}
	xgmac->rst_ocp = reset_control_get(dev, "stmmaceth-ocp");
	if (IS_ERR(xgmac->rst_ocp)) {
		dev_err(dev, "Invalid reset line 'stmmaceth-ocp'.\n");
		return PTR_ERR(xgmac->rst_ocp);
	}

	reset_control_assert(xgmac->rst_ocp);
	reset_control_assert(xgmac->rst);

	ret = dwxgmac_socfpga_do_setphy(dev, modereg);
	if (ret)
		return ret;

	reset_control_deassert(xgmac->rst_ocp);
	reset_control_deassert(xgmac->rst);

	return 0;
}

static int xgmac_get_enetaddr_socfpga(struct device *dev)
{
	return -ENOTSUPP;
}

static int xgmac_start_resets_socfpga(struct device *dev)
{
	struct xgmac_priv *xgmac = dev_get_priv(dev);

	reset_control_assert(xgmac->rst);
	reset_control_assert(xgmac->rst_ocp);

	udelay(2);

	reset_control_deassert(xgmac->rst);
	reset_control_deassert(xgmac->rst_ocp);

	return 0;
}

static struct xgmac_ops xgmac_socfpga_ops = {
	.xgmac_probe_resources = xgmac_probe_resources_socfpga,
	.xgmac_start_resets = xgmac_start_resets_socfpga,
	.xgmac_get_enetaddr = xgmac_get_enetaddr_socfpga,
};

struct xgmac_config __maybe_unused xgmac_socfpga_config = {
	.reg_access_always_ok = false,
	.swr_wait = 50,
	.config_mac = XGMAC_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
	.config_mac_mdio = XGMAC_MAC_MDIO_ADDRESS_CR_350_400,
	.axi_bus_width = XGMAC_AXI_WIDTH_64,
	.ops = &xgmac_socfpga_ops
};

static __maybe_unused struct of_device_id xgmac_socfpga_compatible[] = {
	{
		.compatible = "intel,socfpga-dwxgmac",
		.data = &xgmac_socfpga_config
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, xgmac_socfpga_compatible);

static struct driver xgmac_socfpga_driver = {
	.name = "designware-xgmac-socfpga",
	.probe = xgmac_probe,
	.remove = xgmac_remove,
	.of_compatible = DRV_OF_COMPAT(xgmac_socfpga_compatible),
};
device_platform_driver(xgmac_socfpga_driver);

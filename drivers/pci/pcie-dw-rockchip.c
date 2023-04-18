// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe host controller driver for Rockchip SoCs.
 *
 * Copyright (C) 2021 Rockchip Electronics Co., Ltd.
 *		http://www.rock-chips.com
 *
 * Author: Simon Xue <xxm@rock-chips.com>
 */

#include <common.h>
#include <clock.h>
#include <abort.h>
#include <malloc.h>
#include <io.h>
#include <init.h>
#include <gpio.h>
#include <asm/mmu.h>
#include <of_gpio.h>
#include <of_device.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <of_address.h>
#include <of_pci.h>
#include <gpiod.h>
#include <linux/pci.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>
#include <linux/sizes.h>
#include <linux/bitfield.h>

#include "pcie-designware.h"

/*
 * The upper 16 bits of PCIE_CLIENT_CONFIG are a write
 * mask for the lower 16 bits.
 */
#define HIWORD_UPDATE(mask, val) (((mask) << 16) | (val))
#define HIWORD_UPDATE_BIT(val)	HIWORD_UPDATE(val, val)
#define HIWORD_DISABLE_BIT(val)	HIWORD_UPDATE(val, ~val)

#define PCIE_CLIENT_RC_MODE		HIWORD_UPDATE_BIT(0x40)
#define PCIE_CLIENT_ENABLE_LTSSM	HIWORD_UPDATE_BIT(0xc)
#define PCIE_SMLH_LINKUP		BIT(16)
#define PCIE_RDLH_LINKUP		BIT(17)
#define PCIE_LINKUP			(PCIE_SMLH_LINKUP | PCIE_RDLH_LINKUP)
#define PCIE_L0S_ENTRY			0x11
#define PCIE_CLIENT_GENERAL_CONTROL	0x0
#define PCIE_CLIENT_INTR_STATUS_LEGACY	0x8
#define PCIE_CLIENT_INTR_MASK_LEGACY	0x1c
#define PCIE_CLIENT_GENERAL_DEBUG	0x104
#define PCIE_CLIENT_HOT_RESET_CTRL	0x180
#define PCIE_CLIENT_LTSSM_STATUS	0x300
#define PCIE_LTSSM_ENABLE_ENHANCE	BIT(4)
#define PCIE_LTSSM_STATUS_MASK		GENMASK(5, 0)

struct rockchip_pcie {
	struct dw_pcie			pci;
	void __iomem			*apb_base;
	struct phy			*phy;
	struct clk_bulk_data		*clks;
	unsigned int			clk_cnt;
	struct reset_control		*rst;
	int				rst_gpio;
	struct regulator                *vpcie3v3;
	struct irq_domain		*irq_domain;
};

static inline struct rockchip_pcie *to_rockchip_pcie(struct dw_pcie *dw)
{
	return container_of(dw, struct rockchip_pcie, pci);
}

static int rockchip_pcie_readl_apb(struct rockchip_pcie *rockchip,
					     u32 reg)
{
	return readl_relaxed(rockchip->apb_base + reg);
}

static void rockchip_pcie_writel_apb(struct rockchip_pcie *rockchip,
						u32 val, u32 reg)
{
	writel_relaxed(val, rockchip->apb_base + reg);
}

static void rockchip_pcie_enable_ltssm(struct rockchip_pcie *rockchip)
{
	rockchip_pcie_writel_apb(rockchip, PCIE_CLIENT_ENABLE_LTSSM,
				 PCIE_CLIENT_GENERAL_CONTROL);
}

static int rockchip_pcie_link_up(struct dw_pcie *pci)
{
	struct rockchip_pcie *rockchip = to_rockchip_pcie(pci);
	u32 val = rockchip_pcie_readl_apb(rockchip, PCIE_CLIENT_LTSSM_STATUS);

	if ((val & PCIE_LINKUP) == PCIE_LINKUP &&
	    (val & PCIE_LTSSM_STATUS_MASK) == PCIE_L0S_ENTRY)
		return 1;

	return 0;
}

static int rockchip_pcie_start_link(struct dw_pcie *pci)
{
	struct rockchip_pcie *rockchip = to_rockchip_pcie(pci);

	/* Reset device */
	gpio_set_value(rockchip->rst_gpio, 0);

	rockchip_pcie_enable_ltssm(rockchip);

	/*
	 * PCIe requires the refclk to be stable for 100µs prior to releasing
	 * PERST. See table 2-4 in section 2.6.2 AC Specifications of the PCI
	 * Express Card Electromechanical Specification, 1.1. However, we don't
	 * know if the refclk is coming from RC's PHY or external OSC. If it's
	 * from RC, so enabling LTSSM is the just right place to release #PERST.
	 * We need more extra time as before, rather than setting just
	 * 100us as we don't know how long should the device need to reset.
	 */
	mdelay(100);
	gpio_set_value(rockchip->rst_gpio, 1);

	return 0;
}

static int rockchip_pcie_host_init(struct pcie_port *pp)
{
	struct dw_pcie *pci = to_dw_pcie_from_pp(pp);
	struct rockchip_pcie *rockchip = to_rockchip_pcie(pci);
	u32 val = HIWORD_UPDATE_BIT(PCIE_LTSSM_ENABLE_ENHANCE);

	/* LTSSM enable control mode */
	rockchip_pcie_writel_apb(rockchip, val, PCIE_CLIENT_HOT_RESET_CTRL);

	rockchip_pcie_writel_apb(rockchip, PCIE_CLIENT_RC_MODE,
				 PCIE_CLIENT_GENERAL_CONTROL);

	dw_pcie_setup_rc(pp);
	rockchip_pcie_start_link(pci);

	return 0;
}

static const struct dw_pcie_host_ops rockchip_pcie_host_ops = {
	.host_init = rockchip_pcie_host_init,
};

static int rockchip_pcie_clk_init(struct rockchip_pcie *rockchip)
{
	struct device *dev = rockchip->pci.dev;
	int ret;

	ret = clk_bulk_get_all(dev, &rockchip->clks);
	if (ret < 0)
		return ret;

	rockchip->clk_cnt = ret;

	return clk_bulk_enable(rockchip->clk_cnt, rockchip->clks);
}

static int rockchip_pcie_resource_get(struct device *dev,
				      struct rockchip_pcie *rockchip)
{
	struct resource *r;

	r = dev_request_mem_resource_by_name(dev, "apb");
	if (IS_ERR(r))
		return PTR_ERR(r);
	rockchip->apb_base = IOMEM(r->start);


	r = dev_request_mem_resource_by_name(dev, "dbi");
	if (IS_ERR(r))
		return PTR_ERR(r);
	rockchip->pci.dbi_base = IOMEM(r->start);

	rockchip->rst_gpio = gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (rockchip->rst_gpio < 0 && rockchip->rst_gpio != -ENOENT)
		return rockchip->rst_gpio;

	rockchip->rst = reset_control_array_get(dev);
	if (IS_ERR(rockchip->rst))
		return dev_err_probe(dev, PTR_ERR(rockchip->rst),
				     "failed to get reset lines\n");

	return 0;
}

static int rockchip_pcie_phy_init(struct rockchip_pcie *rockchip)
{
	struct device *dev = rockchip->pci.dev;
	int ret;

	rockchip->phy = phy_get(dev, "pcie-phy");
	if (IS_ERR(rockchip->phy))
		return dev_err_probe(dev, PTR_ERR(rockchip->phy),
				     "missing PHY\n");

	ret = phy_init(rockchip->phy);
	if (ret < 0)
		return ret;

	ret = phy_power_on(rockchip->phy);
	if (ret)
		phy_exit(rockchip->phy);

	return ret;
}

static void rockchip_pcie_phy_deinit(struct rockchip_pcie *rockchip)
{
	phy_exit(rockchip->phy);
	phy_power_off(rockchip->phy);
}

static const struct dw_pcie_ops dw_pcie_ops = {
	.link_up = rockchip_pcie_link_up,
};

static int rockchip_pcie_probe(struct device *dev)
{
	struct rockchip_pcie *rockchip;
	struct pcie_port *pp;
	int ret;

	rockchip = xzalloc(sizeof(*rockchip));
	if (!rockchip)
		return -ENOMEM;

	rockchip->pci.dev = dev;
	rockchip->pci.ops = &dw_pcie_ops;

	pp = &rockchip->pci.pp;
	pp->ops = &rockchip_pcie_host_ops;

	ret = rockchip_pcie_resource_get(dev, rockchip);
	if (ret)
		return ret;

	ret = reset_control_assert(rockchip->rst);
	if (ret)
		return ret;

	/* DON'T MOVE ME: must be enable before PHY init */
	rockchip->vpcie3v3 = regulator_get(dev, "vpcie3v3");
	if (IS_ERR(rockchip->vpcie3v3)) {
		if (PTR_ERR(rockchip->vpcie3v3) != -ENODEV)
			return dev_err_probe(dev, PTR_ERR(rockchip->vpcie3v3),
					"failed to get vpcie3v3 regulator\n");
		rockchip->vpcie3v3 = NULL;
	} else {
		ret = regulator_enable(rockchip->vpcie3v3);
		if (ret) {
			dev_err(dev, "failed to enable vpcie3v3 regulator\n");
			return ret;
		}
	}

	ret = rockchip_pcie_phy_init(rockchip);
	if (ret)
		goto disable_regulator;

	ret = reset_control_deassert(rockchip->rst);
	if (ret)
		goto deinit_phy;

	ret = rockchip_pcie_clk_init(rockchip);
	if (ret)
		goto deinit_phy;

	ret = dw_pcie_host_init(pp);
	if (!ret)
		return 0;

	clk_bulk_disable(rockchip->clk_cnt, rockchip->clks);
deinit_phy:
	rockchip_pcie_phy_deinit(rockchip);
disable_regulator:
	if (rockchip->vpcie3v3)
		regulator_disable(rockchip->vpcie3v3);

	return ret;
}

static const struct of_device_id rockchip_pcie_of_match[] = {
	{ .compatible = "rockchip,rk3568-pcie", },
	{ .compatible = "rockchip,rk3588-pcie", },
	{},
};

static struct driver rockchip_pcie_driver = {
        .name = "rockchip-dw-pcie",
        .of_compatible = DRV_OF_COMPAT(rockchip_pcie_of_match),
        .probe = rockchip_pcie_probe,
};
device_platform_driver(rockchip_pcie_driver);

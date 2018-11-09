/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * Partly based on code
 * Copyright (C) 2014, NVIDIA CORPORATION.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <clock.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <pinctrl.h>
#include <linux/err.h>
#include <linux/reset.h>
#include <linux/phy/phy.h>

#include <dt-bindings/pinctrl/pinctrl-tegra-xusb.h>

#define XUSB_PADCTL_ELPG_PROGRAM 0x01c
#define XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_VCORE_DOWN (1 << 26)
#define XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN_EARLY (1 << 25)
#define XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN (1 << 24)

#define XUSB_PADCTL_IOPHY_PLL_P0_CTL1 0x040
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL1_PLL0_LOCKDET (1 << 19)
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL1_REFCLK_SEL_MASK (0xf << 12)
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL1_PLL_RST (1 << 1)

#define XUSB_PADCTL_IOPHY_PLL_P0_CTL2 0x044
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL2_REFCLKBUF_EN (1 << 6)
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL2_TXCLKREF_EN (1 << 5)
#define XUSB_PADCTL_IOPHY_PLL_P0_CTL2_TXCLKREF_SEL (1 << 4)

#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1 0x138
#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_LOCKDET (1 << 27)
#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_MODE (1 << 24)
#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_PWR_OVRD (1 << 3)
#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_RST (1 << 1)
#define XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_IDDQ (1 << 0)

#define XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1 0x148
#define XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ_OVRD (1 << 1)
#define XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ (1 << 0)

struct tegra_xusb_padctl_soc {
	const struct tegra_xusb_padctl_lane *lanes;
	unsigned int num_lanes;
};

struct tegra_xusb_padctl_lane {
	const char *name;

	unsigned int offset;
	unsigned int shift;
	unsigned int mask;
	unsigned int iddq;

	const char **funcs;
	unsigned int num_funcs;
};

struct tegra_xusb_padctl {
	struct device_d *dev;
	void __iomem *regs;
	struct reset_control *rst;

	const struct tegra_xusb_padctl_soc *soc;
	struct pinctrl_device pinctrl;

	struct phy_provider *provider;
	struct phy *phys[2];

	unsigned int enable;
};

static inline void padctl_writel(struct tegra_xusb_padctl *padctl, u32 value,
				 unsigned long offset)
{
	writel(value, padctl->regs + offset);
}

static inline u32 padctl_readl(struct tegra_xusb_padctl *padctl,
			       unsigned long offset)
{
	return readl(padctl->regs + offset);
}

static int tegra_xusb_padctl_enable(struct tegra_xusb_padctl *padctl)
{
	u32 value;

	if (padctl->enable++ > 0)
		return 0;

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value &= ~XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	udelay(100);

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value &= ~XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN_EARLY;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	udelay(100);

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value &= ~XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_VCORE_DOWN;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	return 0;
}

static int tegra_xusb_padctl_disable(struct tegra_xusb_padctl *padctl)
{
	u32 value;

	if (WARN_ON(padctl->enable == 0))
		return 0;

	if (--padctl->enable > 0)
		return 0;

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value |= XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_VCORE_DOWN;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	udelay(100);

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value |= XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN_EARLY;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	udelay(100);

	value = padctl_readl(padctl, XUSB_PADCTL_ELPG_PROGRAM);
	value |= XUSB_PADCTL_ELPG_PROGRAM_AUX_MUX_LP0_CLAMP_EN;
	padctl_writel(padctl, value, XUSB_PADCTL_ELPG_PROGRAM);

	return 0;
}

static int tegra_xusb_phy_init(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);

	return tegra_xusb_padctl_enable(padctl);
}

static int tegra_xusb_phy_exit(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);

	return tegra_xusb_padctl_disable(padctl);
}

static int pcie_phy_power_on(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);
	int err;
	u32 value;

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_PLL_P0_CTL1_REFCLK_SEL_MASK;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_P0_CTL2);
	value |= XUSB_PADCTL_IOPHY_PLL_P0_CTL2_REFCLKBUF_EN |
		 XUSB_PADCTL_IOPHY_PLL_P0_CTL2_TXCLKREF_EN |
		 XUSB_PADCTL_IOPHY_PLL_P0_CTL2_TXCLKREF_SEL;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_P0_CTL2);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);
	value |= XUSB_PADCTL_IOPHY_PLL_P0_CTL1_PLL_RST;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);

	err = wait_on_timeout(50 * MSECOND,
			padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_P0_CTL1) &
			XUSB_PADCTL_IOPHY_PLL_P0_CTL1_PLL0_LOCKDET);

	return err;
}

static int pcie_phy_power_off(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);
	u32 value;

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_PLL_P0_CTL1_PLL_RST;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_P0_CTL1);

	return 0;
}

static const struct phy_ops pcie_phy_ops = {

	.init = tegra_xusb_phy_init,
	.exit = tegra_xusb_phy_exit,
	.power_on = pcie_phy_power_on,
	.power_off = pcie_phy_power_off,
};

static int sata_phy_power_on(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);
	int err;
	u32 value;

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ_OVRD;
	value &= ~XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_PWR_OVRD;
	value &= ~XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_IDDQ;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value |= XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_MODE;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value |= XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_RST;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	err = wait_on_timeout(50 * MSECOND,
			padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1) &
			XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_LOCKDET);

	return err;
}

static int sata_phy_power_off(struct phy *phy)
{
	struct tegra_xusb_padctl *padctl = phy_get_drvdata(phy);
	u32 value;

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_RST;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value &= ~XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL1_MODE;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);
	value |= XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_PWR_OVRD;
	value |= XUSB_PADCTL_IOPHY_PLL_S0_CTL1_PLL_IDDQ;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_PLL_S0_CTL1);

	value = padctl_readl(padctl, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1);
	value |= ~XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ_OVRD;
	value |= ~XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1_IDDQ;
	padctl_writel(padctl, value, XUSB_PADCTL_IOPHY_MISC_PAD_S0_CTL1);

	return 0;
}

static const struct phy_ops sata_phy_ops = {

	.init = tegra_xusb_phy_init,
	.exit = tegra_xusb_phy_exit,
	.power_on = sata_phy_power_on,
	.power_off = sata_phy_power_off,
};

static struct phy *tegra_xusb_padctl_xlate(struct device_d *dev,
					   struct of_phandle_args *args)
{
	struct tegra_xusb_padctl *padctl = dev->priv;
	unsigned int index = args->args[0];

	if (args->args_count <= 0)
		return ERR_PTR(-EINVAL);

	if (index >= ARRAY_SIZE(padctl->phys))
		return ERR_PTR(-EINVAL);

	return padctl->phys[index];
}

static int pinctrl_tegra_xusb_set_state(struct pinctrl_device *pdev,
					struct device_node *np)
{
	struct tegra_xusb_padctl *padctl =
			container_of(pdev, struct tegra_xusb_padctl, pinctrl);
	struct device_node *childnode;
	int iddq = -1, i, j, k;
	const char *lanes, *func = NULL;
	const struct tegra_xusb_padctl_lane *lane = NULL;
	u32 val;

	/*
	 * At first look if the node we are pointed at has children,
	 * which we may want to visit.
	 */
	list_for_each_entry(childnode, &np->children, parent_list)
		pinctrl_tegra_xusb_set_state(pdev, childnode);

	/* read relevant state from devicetree */
	of_property_read_string(np, "nvidia,function", &func);
	of_property_read_u32_array(np, "nvidia,iddq", &iddq, 1);

	/* iterate over all lanes referenced in the dt node */
	for (i = 0; ; i++) {
		if (of_property_read_string_index(np, "nvidia,lanes", i, &lanes))
			break;

		for (j = 0; j < padctl->soc->num_lanes; j++) {
			if (!strcmp(lanes, padctl->soc->lanes[j].name)) {
				lane = &padctl->soc->lanes[j];
				break;
			}
		}
		/* if no matching lane is found */
		if (j == padctl->soc->num_lanes) {
			/* nothing matching found, warn and bail out */
			dev_warn(padctl->pinctrl.dev,
				 "invalid lane %s referenced in node %s\n",
				 lanes, np->name);
			continue;
		}

		if (func) {
			for (k = 0; k < lane->num_funcs; k++) {
				if (!strcmp(func, lane->funcs[k]))
					break;
			}
			if (k < lane->num_funcs) {
				val = padctl_readl(padctl, lane->offset);
				val &= ~(lane->mask << lane->shift);
				val |= k << lane->shift;
				padctl_writel(padctl, val, lane->offset);
			} else {
				dev_warn(padctl->pinctrl.dev,
					 "invalid function %s for lane %s in node %s\n",
					 func, lane->name, np->name);
			}
		}

		if (iddq >= 0) {
			if (lane->iddq) {
				val = padctl_readl(padctl, lane->offset);
				if (iddq)
					val &= ~BIT(lane->iddq);
				else
					val |= BIT(lane->iddq);
				padctl_writel(padctl, val, lane->offset);
			} else {
				dev_warn(padctl->pinctrl.dev,
					 "invalid iddq setting for lane %s in node %s\n",
					 lane->name, np->name);
			}
		}
	}

	return 0;
}

static struct pinctrl_ops pinctrl_tegra_xusb_ops = {
	.set_state = pinctrl_tegra_xusb_set_state,
};

static int pinctrl_tegra_xusb_probe(struct device_d *dev)
{
	struct resource *iores;
	struct tegra_xusb_padctl *padctl;
	struct phy *phy;
	int err;

	padctl = xzalloc(sizeof(*padctl));

	dev->priv = padctl;
	padctl->dev = dev;

	dev_get_drvdata(dev, (const void **)&padctl->soc);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "Could not get iomem region\n");
		return PTR_ERR(iores);
	}
	padctl->regs = IOMEM(iores->start);

	padctl->rst = reset_control_get(dev, NULL);
	if (IS_ERR(padctl->rst))
		return PTR_ERR(padctl->rst);

	err = reset_control_deassert(padctl->rst);
	if (err < 0)
		return err;

	padctl->pinctrl.dev = dev;
	padctl->pinctrl.ops = &pinctrl_tegra_xusb_ops;

	err = pinctrl_register(&padctl->pinctrl);
	if (err) {
		dev_err(dev, "failed to register pincontrol\n");
		err = -ENODEV;
		goto reset;
	}

	phy = phy_create(dev, NULL, &pcie_phy_ops, NULL);
	if (IS_ERR(phy)) {
		err = PTR_ERR(phy);
		goto unregister;
	}

	padctl->phys[TEGRA_XUSB_PADCTL_PCIE] = phy;
	phy_set_drvdata(phy, padctl);

	phy = phy_create(dev, NULL, &sata_phy_ops, NULL);
	if (IS_ERR(phy)) {
		err = PTR_ERR(phy);
		goto unregister;
	}

	padctl->phys[TEGRA_XUSB_PADCTL_SATA] = phy;
	phy_set_drvdata(phy, padctl);

	padctl->provider = of_phy_provider_register(dev, tegra_xusb_padctl_xlate);
	if (IS_ERR(padctl->provider)) {
		err = PTR_ERR(padctl->provider);
		dev_err(dev, "failed to register PHYs: %d\n", err);
		goto unregister;
	}

	return 0;

unregister:
	pinctrl_unregister(&padctl->pinctrl);
reset:
	reset_control_assert(padctl->rst);
	return err;
}

static const char *tegra124_otg_functions[] = {
	"snps",
	"xusb",
	"uart",
	"rsvd",
};

static const char *tegra124_usb_functions[] = {
	"snps",
	"xusb",
};

static const char *tegra124_pci_functions[] = {
	"pcie",
	"usb3",
	"sata",
	"rsvd",
};

#define TEGRA124_LANE(_name, _offs, _shift, _mask, _iddq, _funcs, _num)	\
	{								\
		.name = _name,						\
		.offset = _offs,					\
		.shift = _shift,					\
		.mask = _mask,						\
		.iddq = _iddq,						\
		.num_funcs = _num,					\
		.funcs = tegra124_##_funcs##_functions,			\
	}

static const struct tegra_xusb_padctl_lane tegra124_lanes[] = {
	TEGRA124_LANE("otg-0",  0x004,  0, 0x3, 0, otg, 4),
	TEGRA124_LANE("otg-1",  0x004,  2, 0x3, 0, otg, 4),
	TEGRA124_LANE("otg-2",  0x004,  4, 0x3, 0, otg, 4),
	TEGRA124_LANE("ulpi-0", 0x004, 12, 0x1, 0, usb, 2),
	TEGRA124_LANE("hsic-0", 0x004, 14, 0x1, 0, usb, 2),
	TEGRA124_LANE("hsic-1", 0x004, 15, 0x1, 0, usb, 2),
	TEGRA124_LANE("pcie-0", 0x134, 16, 0x3, 1, pci, 4),
	TEGRA124_LANE("pcie-1", 0x134, 18, 0x3, 2, pci, 4),
	TEGRA124_LANE("pcie-2", 0x134, 20, 0x3, 3, pci, 4),
	TEGRA124_LANE("pcie-3", 0x134, 22, 0x3, 4, pci, 4),
	TEGRA124_LANE("pcie-4", 0x134, 24, 0x3, 5, pci, 4),
	TEGRA124_LANE("sata-0", 0x134, 26, 0x3, 6, pci, 4),
};

static const struct tegra_xusb_padctl_soc tegra124_soc = {
	.num_lanes = ARRAY_SIZE(tegra124_lanes),
	.lanes = tegra124_lanes,
};

static __maybe_unused struct of_device_id pinctrl_tegra_xusb_dt_ids[] = {
	{
		.compatible = "nvidia,tegra124-xusb-padctl",
		.data = &tegra124_soc,
	}, {
		/* sentinel */
	}
};

static struct driver_d pinctrl_tegra_xusb_driver = {
	.name		= "pinctrl-tegra-xusb",
	.probe		= pinctrl_tegra_xusb_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_tegra_xusb_dt_ids),
};

static int pinctrl_tegra_xusb_init(void)
{
	return platform_driver_register(&pinctrl_tegra_xusb_driver);
}
core_initcall(pinctrl_tegra_xusb_init);

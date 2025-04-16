// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 *
 * Portions based on U-Boot's rtl8169.c and dwc_eth_qos.
 */

#include <common.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <mfd/syscon.h>
#include <net.h>
#include <of_device.h>

#include "designware_eqos.h"

#define SYSCFG_PMCR_ETH_CLK_SEL		BIT(16)
#define SYSCFG_PMCR_ETH_REF_CLK_SEL	BIT(17)

/*  Ethernet PHY interface selection in register SYSCFG Configuration
 *------------------------------------------
 * src	 |BIT(23)| BIT(22)| BIT(21)|BIT(20)|
 *------------------------------------------
 * MII   |   0	 |   0	  |   0    |   1   |
 *------------------------------------------
 * GMII  |   0	 |   0	  |   0    |   0   |
 *------------------------------------------
 * RGMII |   0	 |   0	  |   1	   |  n/a  |
 *------------------------------------------
 * RMII  |   1	 |   0	  |   0	   |  n/a  |
 *------------------------------------------
 */
#define SYSCFG_PMCR_ETH_SEL_MII		BIT(20)
#define SYSCFG_PMCR_ETH_SEL_RGMII	BIT(21)
#define SYSCFG_PMCR_ETH_SEL_RMII	BIT(23)
#define SYSCFG_PMCR_ETH_SEL_GMII	0
#define SYSCFG_MCU_ETH_SEL_MII		0
#define SYSCFG_MCU_ETH_SEL_RMII		1

/* Descriptors */

#define SYSCFG_MCU_ETH_MASK		BIT(23)
#define SYSCFG_MP1_ETH_MASK		GENMASK(23, 16)
#define SYSCFG_PMCCLRR_OFFSET		0x40

struct stm32_ops {
	const struct eqos_ops *eqos_ops;
	bool is_mp13;
	u32 syscfg_clr_off;
};

struct eqos_stm32 {
	struct clk_bulk_data *clks;
	int num_clks;
	struct regmap *regmap;
	u32 mode_reg;
	u32 mode_mask;
	int eth_clk_sel_reg;
	int eth_ref_clk_sel_reg;
	const struct stm32_ops *ops;
};

static inline struct eqos_stm32 *to_stm32(struct eqos *eqos)
{
	return eqos->priv;
}

enum { CLK_STMMACETH, CLK_MAX_RX, CLK_MAX_TX, };
static const struct clk_bulk_data stm32_clks[] = {
	[CLK_STMMACETH] = { .id = "stmmaceth" },
	[CLK_MAX_RX]    = { .id = "mac-clk-rx" },
	[CLK_MAX_TX]    = { .id = "mac-clk-tx" },
};

static unsigned long eqos_get_csr_clk_rate_stm32(struct eqos *eqos)
{
	return clk_get_rate(to_stm32(eqos)->clks[CLK_STMMACETH].clk);
}

static int eqos_set_mode_stm32(struct eqos_stm32 *priv, phy_interface_t interface)
{
	u32 reg = priv->mode_reg;
	u32 val = 0;
	int ret;

	switch (interface) {
	case PHY_INTERFACE_MODE_MII:
		/*
		 * STM32MP15xx supports both MII and GMII, STM32MP13xx MII only.
		 * SYSCFG_PMCSETR ETH_SELMII is present only on STM32MP15xx and
		 * acts as a selector between 0:GMII and 1:MII. As STM32MP13xx
		 * supports only MII, ETH_SELMII is not present.
		 */
		if (!priv->ops->is_mp13)  /* Select MII mode on STM32MP15xx */
			val |= SYSCFG_PMCR_ETH_SEL_MII;
		break;
	case PHY_INTERFACE_MODE_GMII:
		val = SYSCFG_PMCR_ETH_SEL_GMII;
		if (priv->eth_clk_sel_reg)
			val |= SYSCFG_PMCR_ETH_CLK_SEL;
		break;
	case PHY_INTERFACE_MODE_RMII:
		val = SYSCFG_PMCR_ETH_SEL_RMII;
		if (priv->eth_ref_clk_sel_reg)
			val |= SYSCFG_PMCR_ETH_REF_CLK_SEL;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = SYSCFG_PMCR_ETH_SEL_RGMII;
		if (priv->eth_clk_sel_reg)
			val |= SYSCFG_PMCR_ETH_CLK_SEL;
		break;
	default:
		return -EINVAL;
	}

	/* Shift value at correct ethernet MAC offset in SYSCFG_PMCSETR */
	val <<= ffs(priv->mode_mask) - ffs(SYSCFG_MP1_ETH_MASK);

	/* Need to update PMCCLRR (clear register) */
	ret = regmap_write(priv->regmap, priv->ops->syscfg_clr_off,
			   priv->mode_mask);
	if (ret)
		return ret;

	return regmap_update_bits(priv->regmap, reg, priv->mode_mask, val);
}

static int eqos_init_stm32(struct device *dev, struct eqos *eqos)
{
	struct device_node *np = dev->of_node;
	struct eqos_stm32 *priv = to_stm32(eqos);
	struct clk_bulk_data *eth_ck;
	int ret;

	/* Gigabit Ethernet 125MHz clock selection. */
	priv->eth_clk_sel_reg = of_property_read_bool(np, "st,eth-clk-sel");

	/* Ethernet 50Mhz RMII clock selection */
	priv->eth_ref_clk_sel_reg =
		of_property_read_bool(np, "st,eth-ref-clk-sel");

	priv->regmap = syscon_regmap_lookup_by_phandle(dev->of_node,
						       "st,syscon");
	if (IS_ERR(priv->regmap)) {
		dev_err(dev, "Could not get st,syscon node\n");
		return PTR_ERR(priv->regmap);
	}

	ret = of_property_read_u32_index(dev->of_node, "st,syscon",
					 1, &priv->mode_reg);
	if (ret) {
		dev_err(dev, "Can't get sysconfig mode offset (%pe)\n",
			ERR_PTR(ret));
		return -EINVAL;
	}

	priv->mode_mask = SYSCFG_MP1_ETH_MASK;
	ret = of_property_read_u32_index(np, "st,syscon", 2, &priv->mode_mask);
	if (ret) {
		if (priv->ops->is_mp13) {
			dev_err(dev, "Sysconfig register mask must be set (%pe)\n",
				ERR_PTR(ret));
		} else {
			dev_dbg(dev, "Warning sysconfig register mask not set (%pe)\n",
				ERR_PTR(ret));
		}
	}

	ret = eqos_set_mode_stm32(priv, eqos->interface);
	if (ret)
		dev_warn(dev, "Configuring syscfg failed: %pe\n", ERR_PTR(ret));

	priv->num_clks = ARRAY_SIZE(stm32_clks) + 1;
	priv->clks = xmalloc(priv->num_clks * sizeof(*priv->clks));
	memcpy(priv->clks, stm32_clks, sizeof stm32_clks);

	ret = clk_bulk_get(dev, ARRAY_SIZE(stm32_clks), priv->clks);
	if (ret) {
		dev_err(dev, "Failed to get clks: %pe\n", ERR_PTR(ret));
		return ret;
	}

	eth_ck = &priv->clks[ARRAY_SIZE(stm32_clks)];
	eth_ck->id = "eth-ck";
	eth_ck->clk = clk_get(dev, eth_ck->id);
	if (IS_ERR(eth_ck->clk)) {
		priv->num_clks--;
		dev_dbg(dev, "No phy clock provided. Continuing without.\n");
	}

	return clk_bulk_enable(priv->num_clks, priv->clks);
}

static struct eqos_ops stm32mp13_ops = {
	.init = eqos_init_stm32,
	.get_ethaddr = eqos_get_ethaddr,
	.set_ethaddr = eqos_set_ethaddr,
	.adjust_link = eqos_adjust_link,
	.get_csr_clk_rate = eqos_get_csr_clk_rate_stm32,

	.clk_csr = EQOS_MDIO_ADDR_CR_250_300,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
};

static struct eqos_ops stm32mp15_ops = {
	.init = eqos_init_stm32,
	.get_ethaddr = eqos_get_ethaddr,
	.set_ethaddr = eqos_set_ethaddr,
	.adjust_link = eqos_adjust_link,
	.get_csr_clk_rate = eqos_get_csr_clk_rate_stm32,

	.clk_csr = EQOS_MDIO_ADDR_CR_250_300,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV,
};

static struct stm32_ops stm32mp13_dwmac_data = {
	.eqos_ops = &stm32mp13_ops,
	.syscfg_clr_off = 0x08,
	.is_mp13 = true,
};

static struct stm32_ops stm32mp15_dwmac_data = {
	.eqos_ops = &stm32mp15_ops,
	.syscfg_clr_off = 0x44,
	.is_mp13 = false,
};

static int eqos_probe_stm32(struct device *dev)
{
	const struct stm32_ops *data;
	struct eqos_stm32 *priv;

	priv = xzalloc(sizeof(*priv));
	if (!priv)
		return -ENOMEM;

	data = of_device_get_match_data(dev);
	if (!data)
		return -EINVAL;

	priv->ops = data;

	return eqos_probe(dev, data->eqos_ops, priv);
}

static void eqos_remove_stm32(struct device *dev)
{
	struct eqos_stm32 *priv = to_stm32(dev->priv);

	eqos_remove(dev);

	clk_bulk_disable(priv->num_clks, priv->clks);
	clk_bulk_put(priv->num_clks, priv->clks);
}

static const struct of_device_id eqos_stm32_ids[] = {
	{ .compatible = "st,stm32mp1-dwmac", .data = &stm32mp15_dwmac_data},
	{ .compatible = "st,stm32mp13-dwmac", .data = &stm32mp13_dwmac_data},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, eqos_stm32_ids);

static struct driver eqos_stm32_driver = {
	.name = "eqos-stm32",
	.probe = eqos_probe_stm32,
	.remove	= eqos_remove_stm32,
	.of_compatible = DRV_OF_COMPAT(eqos_stm32_ids),
};
device_platform_driver(eqos_stm32_driver);

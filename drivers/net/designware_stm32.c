// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 *
 * Portions based on U-Boot's rtl8169.c and dwc_eth_qos.
 */

#include <common.h>
#include <init.h>
#include <net.h>
#include <linux/clk.h>
#include <mfd/syscon.h>

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

struct eqos_stm32 {
	struct clk_bulk_data *clks;
	int num_clks;
	struct regmap *regmap;
	u32 mode_reg;
	int eth_clk_sel_reg;
	int eth_ref_clk_sel_reg;
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
	u32 val, reg = priv->mode_reg;
	int ret;

	switch (interface) {
	case PHY_INTERFACE_MODE_MII:
		val = SYSCFG_PMCR_ETH_SEL_MII;
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

	/* Need to update PMCCLRR (clear register) */
	ret = regmap_write(priv->regmap, reg + SYSCFG_PMCCLRR_OFFSET,
			   SYSCFG_MP1_ETH_MASK);
	if (ret)
		return -EIO;

	/* Update PMCSETR (set register) */
	regmap_update_bits(priv->regmap, reg, GENMASK(23, 16), val);

	return 0;
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
		dev_err(dev, "Can't get sysconfig mode offset (%s)\n",
			strerror(-ret));
		return -EINVAL;
	}

	ret = eqos_set_mode_stm32(priv, eqos->interface);
	if (ret)
		dev_warn(dev, "Configuring syscfg failed: %s\n", strerror(-ret));

	priv->num_clks = ARRAY_SIZE(stm32_clks) + 1;
	priv->clks = xmalloc(priv->num_clks * sizeof(*priv->clks));
	memcpy(priv->clks, stm32_clks, sizeof stm32_clks);

	ret = clk_bulk_get(dev, ARRAY_SIZE(stm32_clks), priv->clks);
	if (ret) {
		dev_err(dev, "Failed to get clks: %s\n", strerror(-ret));
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

static struct eqos_ops stm32_ops = {
	.init = eqos_init_stm32,
	.get_ethaddr = eqos_get_ethaddr,
	.set_ethaddr = eqos_set_ethaddr,
	.adjust_link = eqos_adjust_link,
	.get_csr_clk_rate = eqos_get_csr_clk_rate_stm32,

	.clk_csr = EQOS_MDIO_ADDR_CR_250_300,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV,
};

static int eqos_probe_stm32(struct device *dev)
{
	return eqos_probe(dev, &stm32_ops, xzalloc(sizeof(struct eqos_stm32)));
}

static void eqos_remove_stm32(struct device *dev)
{
	struct eqos_stm32 *priv = to_stm32(dev->priv);

	eqos_remove(dev);

	clk_bulk_disable(priv->num_clks, priv->clks);
	clk_bulk_put(priv->num_clks, priv->clks);
}

static const struct of_device_id eqos_stm32_ids[] = {
	{ .compatible = "st,stm32mp1-dwmac" },
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

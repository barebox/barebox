// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <net.h>
#include <regmap.h>
#include <mfd/syscon.h>
#include <linux/clk.h>

#include "designware_eqos.h"

#define GPR_ENET_QOS_INTF_MODE_MASK	GENMASK(21, 16)
#define GPR_ENET_QOS_INTF_SEL_MII	(0x0 << 16)
#define GPR_ENET_QOS_INTF_SEL_RGMII	(0x1 << 16)
#define GPR_ENET_QOS_INTF_SEL_RMII	(0x4 << 16)
#define GPR_ENET_QOS_CLK_GEN_EN		BIT(19)
#define GPR_ENET_QOS_CLK_TX_CLK_SEL	BIT(20)
#define GPR_ENET_QOS_RGMII_EN		BIT(21)


struct eqos_imx8_priv {
	struct device *dev;
	struct clk_bulk_data *clks;
	int num_clks;
	struct regmap *intf_regmap;
	u32 intf_reg_off;
	bool rmii_refclk_ext;
};

enum { CLK_STMMACETH, CLK_PCLK, CLK_PTP_REF, CLK_TX};
static const struct clk_bulk_data imx8_clks[] = {
	[CLK_STMMACETH] = { .id = "stmmaceth" },
	[CLK_PCLK]      = { .id = "pclk" },
	[CLK_PTP_REF]   = { .id = "ptp_ref" },
	[CLK_TX]        = { .id = "tx" },
};

static unsigned long eqos_get_csr_clk_rate_imx8(struct eqos *eqos)
{
	struct eqos_imx8_priv *priv = eqos->priv;

	return clk_get_rate(priv->clks[CLK_PCLK].clk);
}


static void eqos_adjust_link_imx8(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	struct eqos_imx8_priv *priv = eqos->priv;
	unsigned long rate;
	int ret;

	switch (edev->phydev->speed) {
	case SPEED_10:
		rate = 2500000;
		break;
	case SPEED_100:
		rate = 25000000;
		break;
	case SPEED_1000:
		rate = 125000000;
		break;
	default:
		dev_err(priv->dev, "unknown speed value for GMAC speed=%d",
			edev->phydev->speed);
		return;
	}

	ret = clk_set_rate(priv->clks[CLK_TX].clk, rate);
	if (ret)
		dev_err(priv->dev, "set TX clk rate %ld failed %d\n",
			rate, ret);

	eqos_adjust_link(edev);
}

static void eqos_imx8_set_interface_mode(struct eqos *eqos)
{
	struct eqos_imx8_priv *priv = eqos->priv;
	struct device_node *np = priv->dev->device_node;
	int val;

	if (!of_device_is_compatible(np, "nxp,imx8mp-dwmac-eqos"))
		return;

	switch (eqos->interface) {
	case PHY_INTERFACE_MODE_MII:
		val = GPR_ENET_QOS_INTF_SEL_MII;
		break;
	case PHY_INTERFACE_MODE_RMII:
		val = GPR_ENET_QOS_INTF_SEL_RMII;
		val |= (priv->rmii_refclk_ext ? 0 : GPR_ENET_QOS_CLK_TX_CLK_SEL);
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = GPR_ENET_QOS_INTF_SEL_RGMII |
		      GPR_ENET_QOS_RGMII_EN;
		break;
	default:
		dev_err(priv->dev, "no valid interface mode found!\n");
		return;
	}

	val |= GPR_ENET_QOS_CLK_GEN_EN;

	regmap_update_bits(priv->intf_regmap, priv->intf_reg_off,
			   GPR_ENET_QOS_INTF_MODE_MASK, val);
}

static int eqos_init_imx8(struct device *dev, struct eqos *eqos)
{
	struct device_node *np = dev->device_node;
	struct eqos_imx8_priv *priv = eqos->priv;
	int ret;

	priv->dev = dev;

	if (of_get_property(np, "snps,rmii_refclk_ext", NULL))
		priv->rmii_refclk_ext = true;

	if (of_device_is_compatible(np, "nxp,imx8mp-dwmac-eqos")) {
		priv->intf_regmap = syscon_regmap_lookup_by_phandle(np, "intf_mode");
		if (IS_ERR(priv->intf_regmap))
			return PTR_ERR(priv->intf_regmap);

		ret = of_property_read_u32_index(np, "intf_mode", 1, &priv->intf_reg_off);
		if (ret) {
			dev_err(dev, "Can't get intf mode reg offset (%d)\n", ret);
			return ret;
		}
	}

	priv->num_clks = ARRAY_SIZE(imx8_clks);
	priv->clks = xmalloc(priv->num_clks * sizeof(*priv->clks));
	memcpy(priv->clks, imx8_clks, sizeof imx8_clks);

	ret = clk_bulk_get(dev, priv->num_clks, priv->clks);
	if (ret) {
		dev_err(dev, "Failed to get clks: %s\n", strerror(-ret));
		return ret;
	}

	ret = clk_bulk_enable(priv->num_clks, priv->clks);
	if (ret) {
		dev_err(dev, "Failed to enable clks: %s\n", strerror(-ret));
		return ret;
	}

	eqos_imx8_set_interface_mode(eqos);

	return 0;
}

static struct eqos_ops imx8_ops = {
	.init = eqos_init_imx8,
	.get_ethaddr = eqos_get_ethaddr,
	.set_ethaddr = eqos_set_ethaddr,
	.adjust_link = eqos_adjust_link_imx8,
	.get_csr_clk_rate = eqos_get_csr_clk_rate_imx8,

	.clk_csr = EQOS_MDIO_ADDR_CR_250_300,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
};

static int eqos_probe_imx8(struct device *dev)
{
	return eqos_probe(dev, &imx8_ops, xzalloc(sizeof(struct eqos_imx8_priv)));
}

static void eqos_remove_imx8(struct device *dev)
{
	struct eqos *eqos = dev->priv;
	struct eqos_imx8_priv *priv = eqos->priv;

	eqos_remove(dev);

	clk_bulk_disable(priv->num_clks, priv->clks);
	clk_bulk_put(priv->num_clks, priv->clks);
}

static __maybe_unused struct of_device_id eqos_imx8_ids[] = {
	{ .compatible = "nxp,imx8mp-dwmac-eqos"},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, eqos_imx8_ids);

static struct driver_d eqos_imx8_driver = {
	.name = "eqos-imx8",
	.probe = eqos_probe_imx8,
	.remove = eqos_remove_imx8,
	.of_compatible = DRV_OF_COMPAT(eqos_imx8_ids),
};
device_platform_driver(eqos_imx8_driver);

// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <net.h>
#include <linux/regmap.h>
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

#define MX93_GPR_ENET_QOS_INTF_MODE_MASK	GENMASK(3, 0)
#define MX93_GPR_ENET_QOS_INTF_MASK		GENMASK(3, 1)
#define MX93_GPR_ENET_QOS_INTF_SEL_MII		(0x0 << 1)
#define MX93_GPR_ENET_QOS_INTF_SEL_RMII		(0x4 << 1)
#define MX93_GPR_ENET_QOS_INTF_SEL_RGMII	(0x1 << 1)
#define MX93_GPR_ENET_QOS_CLK_GEN_EN		(0x1 << 0)

struct eqos_imx_soc_data {
	int (*set_interface_mode)(struct eqos *eqos);
	bool mac_rgmii_txclk_auto_adj;
};

struct eqos_imx_priv {
	struct device *dev;
	struct clk_bulk_data *clks;
	int num_clks;
	struct regmap *intf_regmap;
	u32 intf_reg_off;
	bool rmii_refclk_ext;
	struct eqos_imx_soc_data *soc_data;
};

enum { CLK_STMMACETH, CLK_PCLK, CLK_PTP_REF, CLK_TX};
static const struct clk_bulk_data imx_clks[] = {
	[CLK_STMMACETH] = { .id = "stmmaceth" },
	[CLK_PCLK]      = { .id = "pclk" },
	[CLK_PTP_REF]   = { .id = "ptp_ref" },
	[CLK_TX]        = { .id = "tx" },
};

static unsigned long eqos_get_csr_clk_rate_imx(struct eqos *eqos)
{
	struct eqos_imx_priv *priv = eqos->priv;

	return clk_get_rate(priv->clks[CLK_PCLK].clk);
}

static int eqos_set_txclk(struct eqos *eqos, int speed)
{
	struct eqos_imx_priv *priv = eqos->priv;
	unsigned long rate;
	int ret;

	switch (speed) {
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
		dev_err(priv->dev, "unknown speed value for GMAC speed=%d", speed);
		return -EINVAL;
	}

	ret = clk_set_rate(priv->clks[CLK_TX].clk, rate);
	if (ret)
		dev_err(priv->dev, "set TX clk rate %ld failed %d\n", rate, ret);

	return ret;
}

static void eqos_adjust_link_imx(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	struct eqos_imx_priv *priv = eqos->priv;

	if (!priv->soc_data->mac_rgmii_txclk_auto_adj)
		eqos_set_txclk(eqos, edev->phydev->speed);

	eqos_adjust_link(edev);
}

static int eqos_imx8mp_set_interface_mode(struct eqos *eqos)
{
	struct eqos_imx_priv *priv = eqos->priv;
	int val;

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
		return -EINVAL;
	}

	val |= GPR_ENET_QOS_CLK_GEN_EN;

	return regmap_update_bits(priv->intf_regmap, priv->intf_reg_off,
				  GPR_ENET_QOS_INTF_MODE_MASK, val);
}

static int eqos_imx93_set_interface_mode(struct eqos *eqos)
{
	struct eqos_imx_priv *priv = eqos->priv;

	int val;

	switch (eqos->interface) {
	case PHY_INTERFACE_MODE_MII:
		val = MX93_GPR_ENET_QOS_INTF_SEL_MII;
		break;
	case PHY_INTERFACE_MODE_RMII:
		val = MX93_GPR_ENET_QOS_INTF_SEL_RMII;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		val = MX93_GPR_ENET_QOS_INTF_SEL_RGMII;
		break;
	default:
		dev_dbg(priv->dev, "imx dwmac doesn't support %d interface\n",
			 eqos->interface);
		return -EINVAL;
	}

	val |= MX93_GPR_ENET_QOS_CLK_GEN_EN;

	return regmap_update_bits(priv->intf_regmap, priv->intf_reg_off,
				  MX93_GPR_ENET_QOS_INTF_MODE_MASK, val);
};

static int eqos_init_imx(struct device *dev, struct eqos *eqos)
{
	struct device_node *np = dev->device_node;
	struct eqos_imx_priv *priv = eqos->priv;
	int ret;

	priv->intf_regmap = syscon_regmap_lookup_by_phandle(np, "intf_mode");
	if (IS_ERR(priv->intf_regmap))
		return PTR_ERR(priv->intf_regmap);

	ret = of_property_read_u32_index(np, "intf_mode", 1, &priv->intf_reg_off);
	if (ret) {
		dev_err(dev, "Can't get intf mode reg offset (%d)\n", ret);
		return ret;
	}

	ret = priv->soc_data->set_interface_mode(eqos);
	if (ret)
		return ret;

	return 0;
}

static struct eqos_ops imx_ops = {
	.init = eqos_init_imx,
	.get_ethaddr = eqos_get_ethaddr,
	.set_ethaddr = eqos_set_ethaddr,
	.adjust_link = eqos_adjust_link_imx,
	.get_csr_clk_rate = eqos_get_csr_clk_rate_imx,

	.clk_csr = EQOS_MDIO_ADDR_CR_250_300,
	.config_mac = EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB,
};

static int eqos_probe_imx(struct device *dev)
{
	struct device_node *np = dev->device_node;
	struct eqos_imx_soc_data *soc_data;
	struct eqos_imx_priv *priv;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&soc_data);
	if (ret)
		return ret;

	priv = xzalloc(sizeof(*priv));

	priv->soc_data = soc_data;
	priv->dev = dev;

	if (of_get_property(np, "snps,rmii_refclk_ext", NULL))
		priv->rmii_refclk_ext = true;

	priv->num_clks = ARRAY_SIZE(imx_clks);
	priv->clks = xmalloc(priv->num_clks * sizeof(*priv->clks));
	memcpy(priv->clks, imx_clks, sizeof imx_clks);

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

	return eqos_probe(dev, &imx_ops, priv);
}

static void eqos_remove_imx(struct device *dev)
{
	struct eqos *eqos = dev->priv;
	struct eqos_imx_priv *priv = eqos->priv;

	eqos_remove(dev);

	clk_bulk_disable(priv->num_clks, priv->clks);
	clk_bulk_put(priv->num_clks, priv->clks);
}

static struct eqos_imx_soc_data imx93_soc_data = {
	.set_interface_mode = eqos_imx93_set_interface_mode,
	.mac_rgmii_txclk_auto_adj = true,
};

static struct eqos_imx_soc_data imx8mp_soc_data = {
	.set_interface_mode = eqos_imx8mp_set_interface_mode,
	.mac_rgmii_txclk_auto_adj = false,
};

static __maybe_unused struct of_device_id eqos_imx_ids[] = {
	{
		.compatible = "nxp,imx93-dwmac-eqos",
		.data = &imx93_soc_data,
	}, {
		.compatible = "nxp,imx8mp-dwmac-eqos",
		.data = &imx8mp_soc_data,
	},
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, eqos_imx_ids);

static struct driver eqos_imx_driver = {
	.name = "eqos-imx",
	.probe = eqos_probe_imx,
	.remove = eqos_remove_imx,
	.of_compatible = DRV_OF_COMPAT(eqos_imx_ids),
};
device_platform_driver(eqos_imx_driver);

// SPDX-License-Identifier: GPL-2.0-only
/*
 * FPGA to/from HPS Bridge Driver for Altera SoCFPGA Devices
 *
 *  Copyright (C) 2013-2016 Altera Corporation, All Rights Reserved.
 *
 * Includes this patch from the mailing list:
 *   fpga: altera-hps2fpga: fix HPS2FPGA bridge visibility to L3 masters
 *   Signed-off-by: Anatolij Gustschin <agust@denx.de>
 */

/*
 * This driver manages bridges on a Altera SOCFPGA between the ARM host
 * processor system (HPS) and the embedded FPGA.
 *
 * This driver supports enabling and disabling of the configured ports, which
 * allows for safe reprogramming of the FPGA, assuming that the new FPGA image
 * uses the same port configuration.  Bridges must be disabled before
 * reprogramming the FPGA and re-enabled after the FPGA has been programmed.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <fpga-bridge.h>
#include <mfd/syscon.h>
#include <of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>

#define SOCFPGA_L3_ADDR				0xff800000
#define ALT_L3_REMAP_OFST			0x0
#define ALT_L3_REMAP_MPUZERO_MSK		0x00000001
#define ALT_L3_REMAP_H2F_MSK			0x00000008
#define ALT_L3_REMAP_LWH2F_MSK			0x00000010

#define HPS2FPGA_BRIDGE_NAME			"hps2fpga"
#define LWHPS2FPGA_BRIDGE_NAME			"lwhps2fpga"
#define FPGA2HPS_BRIDGE_NAME			"fpga2hps"

struct altera_hps2fpga_data {
	struct device *dev;
	const char *name;
	struct reset_control *bridge_reset;
	unsigned int remap_mask;
	struct clk *clk;
};

/* The L3 REMAP register is write only, so keep a cached value. */
static unsigned int l3_remap_shadow;

static int _alt_hps2fpga_enable_set(struct altera_hps2fpga_data *priv,
				    bool enable)
{
	int ret;

	/* bring bridge out of reset */
	if (enable)
		ret = reset_control_deassert(priv->bridge_reset);
	else
		ret = reset_control_assert(priv->bridge_reset);
	if (ret)
		return ret;

	/* Allow bridge to be visible to L3 masters or not */
	if (priv->remap_mask) {
		l3_remap_shadow |= ALT_L3_REMAP_MPUZERO_MSK;

		if (enable)
			l3_remap_shadow |= priv->remap_mask;
		else
			l3_remap_shadow &= ~priv->remap_mask;

		dev_dbg(priv->dev, "setting L3 visibility to 0x%08x\n",
			l3_remap_shadow);

		writel(l3_remap_shadow, SOCFPGA_L3_ADDR + ALT_L3_REMAP_OFST);
	}

	return ret;
}

static int alt_hps2fpga_enable_set(struct fpga_bridge *bridge, bool enable)
{
	return _alt_hps2fpga_enable_set(bridge->priv, enable);
}

static const struct fpga_bridge_ops altera_hps2fpga_br_ops = {
	.enable_set = alt_hps2fpga_enable_set,
};

static struct altera_hps2fpga_data hps2fpga_data  = {
	.name = HPS2FPGA_BRIDGE_NAME,
	.remap_mask = ALT_L3_REMAP_H2F_MSK,
};

static struct altera_hps2fpga_data lwhps2fpga_data  = {
	.name = LWHPS2FPGA_BRIDGE_NAME,
	.remap_mask = ALT_L3_REMAP_LWH2F_MSK,
};

static struct altera_hps2fpga_data fpga2hps_data  = {
	.name = FPGA2HPS_BRIDGE_NAME,
};

static struct of_device_id altera_fpga_of_match[] = {
	{ .compatible = "altr,socfpga-hps2fpga-bridge",
	  .data = &hps2fpga_data },
	{ .compatible = "altr,socfpga-lwhps2fpga-bridge",
	  .data = &lwhps2fpga_data },
	{ .compatible = "altr,socfpga-fpga2hps-bridge",
	  .data = &fpga2hps_data },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, altera_fpga_of_match);

static int alt_fpga_bridge_probe(struct device *dev)
{
	struct altera_hps2fpga_data *priv;
	const struct of_device_id *of_id;
	u32 enable;
	int ret;

	of_id = of_match_device(altera_fpga_of_match, dev);
	priv = (struct altera_hps2fpga_data *)of_id->data;

	priv->bridge_reset = of_reset_control_get(dev->of_node, NULL);
	if (IS_ERR(priv->bridge_reset)) {
		dev_err(dev, "Could not get %s reset control\n", priv->name);
		return PTR_ERR(priv->bridge_reset);
	}

	priv->clk = clk_get(dev, NULL);
	if (IS_ERR(priv->clk)) {
		dev_err(dev, "no clock specified\n");
		return PTR_ERR(priv->clk);
	}

	ret = clk_enable(priv->clk);
	if (ret) {
		dev_err(dev, "could not enable clock\n");
		return -EBUSY;
	}

	priv->dev = dev;

	if (!of_property_read_u32(dev->of_node, "bridge-enable", &enable)) {
		if (enable > 1) {
			dev_warn(dev, "invalid bridge-enable %u > 1\n", enable);
		} else {
			dev_info(dev, "%s bridge\n",
				 (enable ? "enabling" : "disabling"));

			ret = _alt_hps2fpga_enable_set(priv, enable);
			if (ret)
				return ret;
		}
	}

	return fpga_bridge_register(dev, priv->name, &altera_hps2fpga_br_ops,
				    priv);
}

static struct driver alt_fpga_bridge_driver = {
	.probe = alt_fpga_bridge_probe,
	.name = "altera-hps2fpga-bridge",
	.of_compatible = DRV_OF_COMPAT(altera_fpga_of_match),
};
device_platform_driver(alt_fpga_bridge_driver);

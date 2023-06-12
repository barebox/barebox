// SPDX-License-Identifier: GPL-2.0-only
/*
 * FPGA to SDRAM Bridge Driver for Altera SoCFPGA Devices
 *
 *  Copyright (C) 2013-2016 Altera Corporation, All Rights Reserved.
 */

/*
 * This driver manages a bridge between an FPGA and the SDRAM used by the ARM
 * host processor system (HPS).
 *
 * The bridge contains 4 read ports, 4 write ports, and 6 command ports.
 * Reconfiguring these ports requires that no SDRAM transactions occur during
 * reconfiguration.  The code reconfiguring the ports cannot run out of SDRAM
 * nor can the FPGA access the SDRAM during reconfiguration.  This driver does
 * not support reconfiguring the ports.  The ports are configured by code
 * running out of on chip ram before Linux is started and the configuration
 * is passed in a handoff register in the system manager.
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

#define SOCFPGA_SDRCTL_ADDR			0xffc25000
#define ALT_SDR_CTL_FPGAPORTRST_OFST		0x80
#define ALT_SDR_CTL_FPGAPORTRST_PORTRSTN_MSK	0x00003fff
#define ALT_SDR_CTL_FPGAPORTRST_RD_SHIFT	0
#define ALT_SDR_CTL_FPGAPORTRST_WR_SHIFT	4
#define ALT_SDR_CTL_FPGAPORTRST_CTRL_SHIFT	8

#define SOCFPGA_SYSMGR_ADDR			0xffd08000
/*
 * From the Cyclone V HPS Memory Map document:
 *   These registers are used to store handoff information between the
 *   preloader and the OS. These 8 registers can be used to store any
 *   information. The contents of these registers have no impact on
 *   the state of the HPS hardware.
 */
#define SYSMGR_ISWGRP_HANDOFF3          (0x8C)

#define F2S_BRIDGE_NAME "fpga2sdram"

struct alt_fpga2sdram_data {
	struct device *dev;
	int mask;
};

static inline int _alt_fpga2sdram_enable_set(struct alt_fpga2sdram_data *priv,
					     bool enable)
{
	int val;

	val = readl(SOCFPGA_SDRCTL_ADDR + ALT_SDR_CTL_FPGAPORTRST_OFST);

	if (enable)
		val |= priv->mask;
	else
		val = 0;

	/* The kernel driver expects this value in this register :-( */
	writel(priv->mask, SOCFPGA_SYSMGR_ADDR + SYSMGR_ISWGRP_HANDOFF3);

	dev_dbg(priv->dev, "setting fpgaportrst to 0x%08x\n", val);

	return writel(val, SOCFPGA_SDRCTL_ADDR + ALT_SDR_CTL_FPGAPORTRST_OFST);
}

static int alt_fpga2sdram_enable_set(struct fpga_bridge *bridge, bool enable)
{
	return _alt_fpga2sdram_enable_set(bridge->priv, enable);
}

struct prop_map {
	char *prop_name;
	u32 *prop_value;
	u32 prop_max;
};

static const struct fpga_bridge_ops altera_fpga2sdram_br_ops = {
	.enable_set = alt_fpga2sdram_enable_set,
};

static struct of_device_id altera_fpga_of_match[] = {
	{ .compatible = "altr,socfpga-fpga2sdram-bridge" },
	{},
};
MODULE_DEVICE_TABLE(of, altera_fpga_of_match);

static int alt_fpga_bridge_probe(struct device *dev)
{
	struct alt_fpga2sdram_data *priv;
	int ret = 0;

	priv = xzalloc(sizeof(*priv));
	if (!priv)
		return -ENOMEM;

	/* enable all ports for now */
	priv->mask = ALT_SDR_CTL_FPGAPORTRST_PORTRSTN_MSK;

	priv->dev = dev;

	ret = fpga_bridge_register(dev, F2S_BRIDGE_NAME,
				   &altera_fpga2sdram_br_ops, priv);
	if (ret)
		return ret;

	dev_info(dev, "driver initialized with handoff %08x\n", priv->mask);

	return ret;
}

static struct driver altera_fpga_driver = {
	.probe = alt_fpga_bridge_probe,
	.name = "altera-fpga2sdram-bridge",
	.of_compatible = DRV_OF_COMPAT(altera_fpga_of_match),
};
device_platform_driver(altera_fpga_driver);

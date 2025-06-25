// SPDX-License-Identifier: GPL-2.0-only
/*
 * Xilinx Zynq MPSoC PL loading
 *
 * Copyright (c) 2018 Thomas Haemmerle <thomas.haemmerle@wolfvision.net>
 *
 * based on U-Boot zynqmppl code
 *
 * (C) Copyright 2015 - 2016, Xilinx, Inc,
 * Michal Simek <michal.simek@xilinx.com>
 * Siva Durga Prasad <siva.durga.paladugu@xilinx.com> *
 */

#include <mach/zynqmp/zynqmp-pcap.h>
#include <mach/zynqmp/firmware-zynqmp.h>
#include <common.h>

struct zynqmp_private {
	const struct zynqmp_eemi_ops *eemi_ops;
};

int zynqmp_init(struct fpgamgr *mgr, struct device *dev)
{
	struct zynqmp_private *priv;
	u32 api_version;
	int ret;
	priv = xzalloc(sizeof(struct zynqmp_private));
	if (!priv)
		return -ENOMEM;

	priv->eemi_ops = zynqmp_pm_get_eemi_ops();

	ret = priv->eemi_ops->get_api_version(&api_version);
	if (ret) {
		dev_err(&mgr->dev, "could not get API version\n");
		goto out;
	}

	mgr->features = 0;

	if (api_version >= ZYNQMP_PM_VERSION(1, 1))
		mgr->features |= ZYNQMP_PM_VERSION_1_1_FEATURES;

	return 0;
out:
	free(priv);
	mgr->private = NULL;
	return ret;
}

int zynqmp_programmed_get(struct fpgamgr *mgr)
{
	struct zynqmp_private *priv = (struct zynqmp_private *) mgr->private;
	u32 status = 0x00;
	int ret = 0;

	ret = priv->eemi_ops->fpga_getstatus(&status);
	if (ret)
		return ret;

	return 0;
}

int zynqmp_fpga_load(struct fpgamgr *mgr, u64 addr, u32 buf_size, u8 flags)
{
	struct zynqmp_private *priv = (struct zynqmp_private *) mgr->private;

	return priv->eemi_ops->fpga_load((u64)addr, buf_size, flags);
}

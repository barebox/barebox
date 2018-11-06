/*
 * NAND Flash Controller Device Driver for DT
 *
 * Copyright © 2011, Picochip.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <init.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <io.h>
#include <of_mtd.h>
#include <errno.h>

#include <linux/clk.h>


#include "denali.h"

struct denali_dt {
	struct denali_nand_info	denali;
	struct clk		*clk;
};


static int denali_dt_probe(struct device_d *ofdev)
{
	struct resource *iores;
	struct denali_dt *dt;
	struct denali_nand_info *denali;
	int ret;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return 1;

	dt = kzalloc(sizeof(*dt), GFP_KERNEL);
	if (!dt)
		return -ENOMEM;
	denali = &dt->denali;

	denali->platform = DT;
	denali->dev = ofdev;

	iores = dev_request_mem_resource(ofdev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	denali->flash_mem = IOMEM(iores->start);

	iores = dev_request_mem_resource(ofdev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	denali->flash_reg = IOMEM(iores->start);

	dt->clk = clk_get(ofdev, NULL);
	if (IS_ERR(dt->clk)) {
		dev_err(ofdev, "no clk available\n");
		return PTR_ERR(dt->clk);
	}
	clk_enable(dt->clk);

	denali->have_hw_ecc_fixup = of_property_read_bool(ofdev->device_node,
		"have-hw-ecc-fixup");

	ret = denali_init(denali);
	if (ret)
		goto out_disable_clk;

	return 0;

out_disable_clk:
	clk_disable(dt->clk);

	return ret;
}

static __maybe_unused struct of_device_id denali_nand_compatible[] = {
	{
		.compatible = "altr,socfpga-denali-nand"
	}, {
		/* sentinel */
	}
};

static struct driver_d denali_dt_driver = {
	.name	= "denali-nand-dt",
	.probe		= denali_dt_probe,
	.of_compatible = DRV_OF_COMPAT(denali_nand_compatible)
};
device_platform_driver(denali_dt_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jamie Iles");
MODULE_DESCRIPTION("DT driver for Denali NAND controller");

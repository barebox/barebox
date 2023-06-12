/*
 * (C) Copyright 2014 Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <io.h>
#include <errno.h>
#include <linux/sizes.h>
#include <init.h>
#include <of.h>
#include <asm/barebox-arm.h>
#include <asm/memory.h>
#include <mach/omap/am33xx-silicon.h>
#include <mach/omap/emif4.h>

static int am33xx_scrm_probe(struct device *dev)
{
	return arm_add_mem_device("ram0", 0x80000000,
				  emif4_sdram_size(IOMEM(AM33XX_EMIF4_BASE)));
}

static __maybe_unused struct of_device_id am33xx_scrm_dt_ids[] = {
	{
		.compatible = "ti,am3-scm",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, am33xx_scrm_dt_ids);

static struct driver am33xx_scrm_driver = {
	.name   = "am33xx-scrm",
	.probe  = am33xx_scrm_probe,
	.of_compatible = DRV_OF_COMPAT(am33xx_scrm_dt_ids),
};

mem_platform_driver(am33xx_scrm_driver);

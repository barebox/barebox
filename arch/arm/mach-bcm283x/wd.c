/*
 * Copyright (C) 2017 Pengutronix, Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on code from  Carlo Caione <carlo@carlocaione.org>
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
#include <init.h>
#include <io.h>
#include <restart.h>

#include <mach/wd.h>

static void __iomem *wd_base;

#define RESET_TIMEOUT 10

static void __noreturn bcm2835_restart_soc(struct restart_handler *rst)
{
	uint32_t rstc;

	rstc = readl(wd_base + PM_RSTC);
	rstc &= ~PM_RSTC_WRCFG_SET;
	rstc |= PM_RSTC_WRCFG_FULL_RESET;
	writel(PM_PASSWORD | RESET_TIMEOUT, wd_base +  PM_WDOG);
	writel(PM_PASSWORD | rstc, wd_base + PM_RSTC);

	hang();
}

static int bcm2835_wd_probe(struct device_d *dev)
{
	struct resource *iores;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	wd_base = IOMEM(iores->start);

	restart_handler_register_fn(bcm2835_restart_soc);

	return 0;
}

static __maybe_unused struct of_device_id bcm2835_wd_dt_ids[] = {
	{
		.compatible = "brcm,bcm2835-pm-wdt",
	}, {
		/* sentinel */
	},
};

static struct driver_d bcm2835_wd_driver = {
	.name		= "bcm2835_wd",
	.of_compatible	= DRV_OF_COMPAT(bcm2835_wd_dt_ids),
	.probe		= bcm2835_wd_probe,
};

static int __init bcm2835_wd_init(void)
{
	return platform_driver_register(&bcm2835_wd_driver);
}
device_initcall(bcm2835_wd_init);

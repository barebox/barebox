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
#include <watchdog.h>

#define PM_RSTC		0x1c
#define PM_RSTS		0x20
#define PM_WDOG		0x24

#define PM_WDOG_RESET				0000000000
#define PM_PASSWORD				0x5a000000
#define PM_WDOG_TIME_SET			0x000fffff
#define PM_RSTC_WRCFG_CLR			0xffffffcf
#define PM_RSTC_WRCFG_SET			0x00000030
#define PM_RSTC_WRCFG_FULL_RESET		0x00000020
#define PM_RSTC_RESET				0x00000102

#define PM_RSTS_HADPOR_SET			0x00001000
#define PM_RSTS_HADSRH_SET			0x00000400
#define PM_RSTS_HADSRF_SET			0x00000200
#define PM_RSTS_HADSRQ_SET			0x00000100
#define PM_RSTS_HADWRH_SET			0x00000040
#define PM_RSTS_HADWRF_SET			0x00000020
#define PM_RSTS_HADWRQ_SET			0x00000010
#define PM_RSTS_HADDRH_SET			0x00000004
#define PM_RSTS_HADDRF_SET			0x00000002
#define PM_RSTS_HADDRQ_SET			0x00000001

#define SECS_TO_WDOG_TICKS(x) ((x) << 16)

struct bcm2835_wd {
	struct watchdog wd;
	void __iomem *base;
	struct device_d *dev;
	struct restart_handler restart;
};

#define RESET_TIMEOUT 10

static void __noreturn bcm2835_restart_soc(struct restart_handler *rst)
{
	struct bcm2835_wd *priv = container_of(rst, struct bcm2835_wd, restart);
	uint32_t rstc;

	rstc = readl(priv->base + PM_RSTC);
	rstc &= ~PM_RSTC_WRCFG_SET;
	rstc |= PM_RSTC_WRCFG_FULL_RESET;
	writel(PM_PASSWORD | RESET_TIMEOUT, priv->base +  PM_WDOG);
	writel(PM_PASSWORD | rstc, priv->base + PM_RSTC);

	hang();
}

static int bcm2835_wd_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct bcm2835_wd *priv = container_of(wd, struct bcm2835_wd, wd);
	u32 val;

	if (!timeout) {
		writel(PM_PASSWORD | PM_RSTC_RESET, priv->base + PM_RSTC);
		return 0;
	}

	writel(PM_PASSWORD | (SECS_TO_WDOG_TICKS(timeout) & PM_WDOG_TIME_SET),
	       priv->base + PM_WDOG);
	val = readl(priv->base + PM_RSTC);
	writel(PM_PASSWORD | (val & PM_RSTC_WRCFG_CLR) |
	       PM_RSTC_WRCFG_FULL_RESET, priv->base + PM_RSTC);

	return 0;
}

static int bcm2835_wd_probe(struct device_d *dev)
{
	struct resource *iores;
	struct bcm2835_wd *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "could not get memory region\n");
		return PTR_ERR(iores);
	}
	priv->base = IOMEM(iores->start);
	priv->wd.set_timeout = bcm2835_wd_set_timeout;
	priv->wd.dev = dev;
	priv->dev = dev;

	if (IS_ENABLED(CONFIG_WATCHDOG_BCM2835)) {
		ret = watchdog_register(&priv->wd);
		if (ret) {
			free(priv);
			return ret;
		}
	}

	priv->restart.name = "bcm2835_wd";
	priv->restart.restart = bcm2835_restart_soc;

	restart_handler_register(&priv->restart);

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

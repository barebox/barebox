// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2017 Pengutronix, Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on code from  Carlo Caione <carlo@carlocaione.org>
 */
#include <common.h>
#include <init.h>
#include <io.h>
#include <restart.h>
#include <watchdog.h>

#include <soc/bcm283x/wdt.h>

#define SECS_TO_WDOG_TICKS(x) ((x) << 16)

/* Largest value where SECS_TO_WDOG_TICKS doesn't overflow 20 bits
 * (PM_WDOG_TIME_SET) */
#define WDOG_SECS_MAX 15

struct bcm2835_wd {
	struct watchdog wd;
	void __iomem *base;
	struct device *dev;
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

static int bcm2835_wd_probe(struct device *dev)
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
	priv->wd.timeout_max = WDOG_SECS_MAX;
	priv->wd.hwdev = dev;
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
MODULE_DEVICE_TABLE(of, bcm2835_wd_dt_ids);

static struct driver bcm2835_wd_driver = {
	.name		= "bcm2835_wd",
	.of_compatible	= DRV_OF_COMPAT(bcm2835_wd_dt_ids),
	.probe		= bcm2835_wd_probe,
};

device_platform_driver(bcm2835_wd_driver);

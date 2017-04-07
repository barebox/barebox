/*
 * Watchdog driver for Marvell Armada XP.
 *
 * Copyright (C) 2017 Pengutronix, Uwe Kleine-König <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <of.h>
#include <watchdog.h>

#define CLKRATE 25000000

/* registers relative to timer_base (i.e. first reg property) */
#define TIMER_CTRL	0x00
#define TIMER_CTRL_WD_TIMER_25MHZ_EN	BIT(10)
#define TIMER_CTRL_WD_TIMER_EN		BIT(8)

#define TIMER_STATUS	0x04
#define TIMER_STATUS_WD_EXPIRED		BIT(31)

#define TIMER_WD_TIMER	0x34

/* registers relative to rstout_base (i.e. second reg property) */
#define WD_RSTOUTn_MASK	0x00
#define WD_RSTOUTn_MASK_GLOBAL_WD	BIT(8)

struct orion_wdt_ddata {
	struct watchdog wd;
	void __iomem *timer_base;
	void __iomem *rstout_base;
};

static int armada_xp_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct orion_wdt_ddata *ddata =
		container_of(wd, struct orion_wdt_ddata, wd);
	u32 ctrl;

	if (0xffffffff / CLKRATE < timeout)
		return -EINVAL;

	ctrl = readl(ddata->timer_base + TIMER_CTRL);

	if (timeout == 0) {
		/* disable timer */
		ctrl &= ~TIMER_CTRL_WD_TIMER_EN;
		writel(ctrl, ddata->timer_base + TIMER_CTRL);

		return 0;
	}

	/* setup duration */
	writel(CLKRATE * timeout, ddata->timer_base + TIMER_WD_TIMER);

	/* clear expiration status */
	writel(readl(ddata->timer_base + TIMER_STATUS) & ~TIMER_STATUS_WD_EXPIRED,
	       ddata->timer_base + TIMER_STATUS);

	/* assert reset on expiration */
	writel(WD_RSTOUTn_MASK_GLOBAL_WD, ddata->rstout_base + WD_RSTOUTn_MASK);

	/* enable */
	ctrl |= TIMER_CTRL_WD_TIMER_25MHZ_EN | TIMER_CTRL_WD_TIMER_EN;
	writel(ctrl, ddata->timer_base + TIMER_CTRL);

	return 0;
}

static int orion_wdt_probe(struct device_d *dev)
{
	struct orion_wdt_ddata* ddata;
	struct resource *res_timer, *res_rstout;

	ddata = xzalloc(sizeof(*ddata));

	ddata->wd.set_timeout = armada_xp_set_timeout;
	ddata->wd.name = "orion_wdt";
	ddata->wd.dev = dev;

	res_timer = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res_timer)) {
		dev_err(dev, "could not get timer memory region\n");
		return PTR_ERR(res_timer);
	}
	ddata->timer_base = IOMEM(res_timer->start);

	res_rstout = dev_request_mem_resource(dev, 1);
	if (IS_ERR(res_rstout)) {
		dev_err(dev, "could not get rstout memory region\n");
		release_region(res_timer);

		return PTR_ERR(res_rstout);
	}
	ddata->rstout_base = IOMEM(res_rstout->start);

	return watchdog_register(&ddata->wd);
}

static const struct of_device_id orion_wdt_of_match[] = {
	{
		.compatible = "marvell,armada-xp-wdt",
	}, { /* sentinel */ }
};

static struct driver_d orion_wdt_driver = {
	.probe = orion_wdt_probe,
	.name = "orion_wdt",
	.of_compatible = DRV_OF_COMPAT(orion_wdt_of_match),
};
device_platform_driver(orion_wdt_driver);

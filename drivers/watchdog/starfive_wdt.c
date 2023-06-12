// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Kalray Inc.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <watchdog.h>
#include <linux/reset.h>

#include <linux/clk.h>
#include <linux/err.h>

#define WDT_REG_RESET_EN	0x104
#define WDT_REG_TIMEOUT		0x108
#define WDT_REG_CONTROL		0x110
#define WDT_REG_UNLOCK		0x13c

#define WDT_UNLOCK_KEY		0x378f0765

#define WDT_TICKS_PER_SEC	50000000

struct starfive_wdt {
	u32 clk_rate;
	struct watchdog wdd;
	void __iomem *base;
	bool setup;
};

static int starfive_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct starfive_wdt *wd = container_of(wdd, struct starfive_wdt, wdd);

	writel(0, wd->base + WDT_REG_CONTROL);

	if (timeout > 0) {
		timeout *= wd->clk_rate;
		writel(timeout, wd->base + WDT_REG_TIMEOUT);
		writel(1, wd->base + WDT_REG_CONTROL);
	}

	return 0;
}

static int starfive_wdt_drv_probe(struct device *dev)
{
	struct starfive_wdt *wd;
	struct resource *iores;
	struct watchdog *wdd;
	struct clk_bulk_data clks[] = {
		{ .id = "bus" },
		{ .id = "core" },
	};
	int ret;

	ret = clk_bulk_get(dev, ARRAY_SIZE(clks), clks);
	if (ret)
		return ret;

	ret = clk_bulk_enable(ARRAY_SIZE(clks), clks);
	if (ret < 0)
		return ret;

	ret = device_reset_all(dev);
	if (ret)
		return ret;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	wd = xzalloc(sizeof(*wd));
	wd->base = IOMEM(iores->start);

	wd->clk_rate = WDT_TICKS_PER_SEC;

	writel(WDT_UNLOCK_KEY, wd->base + WDT_REG_UNLOCK);
	wd->base = IOMEM(iores->start);
	/* reset, not interrupt, on timer expiry */
	writel(1, wd->base + WDT_REG_RESET_EN);

	wdd = &wd->wdd;
	wdd->name = "starfive_wdt";
	wdd->hwdev = dev;
	wdd->set_timeout = starfive_wdt_set_timeout;
	wdd->timeout_max = U32_MAX / wd->clk_rate;

	wdd->running = readl(wd->base + WDT_REG_CONTROL) & 1 ?
		WDOG_HW_RUNNING : WDOG_HW_NOT_RUNNING;

	return watchdog_register(wdd);
}

static struct of_device_id starfive_wdt_of_match[] = {
	{ .compatible = "starfive,wdt", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, starfive_wdt_of_match);

static struct driver starfive_wdt_driver = {
	.name		= "starfive-wdt",
	.probe		= starfive_wdt_drv_probe,
	.of_compatible	= starfive_wdt_of_match,
};
device_platform_driver(starfive_wdt_driver);

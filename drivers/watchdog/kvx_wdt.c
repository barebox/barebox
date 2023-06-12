// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Kalray Inc.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <watchdog.h>

#include <linux/clk.h>
#include <linux/err.h>

#include <asm/sfr.h>

struct kvx_wdt {
	uint64_t clk_rate;
	struct watchdog wdd;
};

static void kvx_watchdog_disable(void)
{
	kvx_sfr_set_field(TCR, WUI, 0);
	kvx_sfr_set_field(TCR, WCE, 0);
}

static int kvx_wdt_set_timeout(struct watchdog *wdd, unsigned int timeout)
{
	struct kvx_wdt *wdt = container_of(wdd, struct kvx_wdt, wdd);
	uint64_t cycle_timeout = wdt->clk_rate * timeout;

	/* Disable watchdog */
	if (timeout == 0) {
		kvx_watchdog_disable();
		return 0;
	}

	kvx_sfr_set(WDV, cycle_timeout);
	kvx_sfr_set(WDR, 0);

	/* Start watchdog counting */
	kvx_sfr_set_field(TCR, WUI, 1);
	kvx_sfr_set_field(TCR, WCE, 1);

	return 0;
}

static int count;

static int kvx_wdt_drv_probe(struct device *dev)
{
	struct watchdog *wdd;
	struct clk *clk;
	struct kvx_wdt *kvx_wdt;

	if (count != 0) {
		dev_warn(dev, "Tried to register core watchdog twice\n");
		return -EINVAL;
	}
	count++;

	kvx_wdt = xzalloc(sizeof(*kvx_wdt));
	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	kvx_wdt->clk_rate = clk_get_rate(clk);
	clk_put(clk);

	wdd = &kvx_wdt->wdd;
	wdd->name = "kvx_wdt";
	wdd->hwdev = dev;
	wdd->set_timeout = kvx_wdt_set_timeout;

	/* Be sure that interrupt are disabled */
	kvx_sfr_set_field(TCR, WIE, 0);

	return watchdog_register(wdd);
}

static struct of_device_id kvx_wdt_of_match[] = {
	{ .compatible = "kalray,kvx-core-watchdog", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, kvx_wdt_of_match);

static struct driver kvx_wdt_driver = {
	.name		= "kvx-wdt",
	.probe		= kvx_wdt_drv_probe,
	.of_compatible	= DRV_OF_COMPAT(kvx_wdt_of_match),
};
device_platform_driver(kvx_wdt_driver);

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Kalray Inc.
 */

#include <common.h>
#include <clock.h>
#include <init.h>

#include <linux/clk.h>
#include <linux/err.h>

#include <asm/sfr.h>

static uint64_t kvx_pm_read(void)
{
	return kvx_sfr_get(PM0);
}

static struct clocksource kvx_clksrc = {
	.read	= kvx_pm_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.priority = 70,
};

static int kvx_timer_probe(struct device *dev)
{
	struct clk *clk;
	uint32_t clk_freq;

	/* Get clock frequency */
	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(dev, "Failed to get CPU clock: %ld\n", PTR_ERR(clk));
		return PTR_ERR(clk);
	}

	clk_freq = clk_get_rate(clk);
	clk_put(clk);

	/* Init clocksource */
	kvx_clksrc.mult = clocksource_hz2mult(clk_freq, kvx_clksrc.shift);

	return init_clock(&kvx_clksrc);
}

static struct of_device_id kvx_timer_dt_ids[] = {
	{ .compatible = "kalray,kvx-core-timer", },
	{ }
};
MODULE_DEVICE_TABLE(of, kvx_timer_dt_ids);

static struct driver kvx_timer_driver = {
	.name = "kvx-timer",
	.probe = kvx_timer_probe,
	.of_compatible = DRV_OF_COMPAT(kvx_timer_dt_ids),
};

postcore_platform_driver(kvx_timer_driver);

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2018 Sascha Hauer <s.hauer@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <linux/clk.h>
#include <io.h>
#include <asm/system.h>

static uint64_t arm_arch_clocksource_read(void)
{
	return get_cntpct();
}

static struct clocksource cs = {
	.read	= arm_arch_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
	.priority = 70,
};

static int arm_arch_timer_probe(struct device *dev)
{
	cs.mult = clocksource_hz2mult(get_cntfrq(), cs.shift);

	return init_clock(&cs);
}

static struct of_device_id arm_arch_timer_dt_ids[] = {
	{ .compatible = "arm,armv7-timer", },
	{ .compatible = "arm,armv8-timer", },
	{ }
};
MODULE_DEVICE_TABLE(of, arm_arch_timer_dt_ids);

static struct driver arm_arch_timer_driver = {
	.name = "arm-architected-timer",
	.probe = arm_arch_timer_probe,
	.of_compatible = DRV_OF_COMPAT(arm_arch_timer_dt_ids),
};
postcore_platform_driver(arm_arch_timer_driver);

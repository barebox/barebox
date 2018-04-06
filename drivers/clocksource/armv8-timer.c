/*
 * Copyright (C) 2018 Sascha Hauer <s.hauer@pengutronix.de>
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
#include <clock.h>
#include <linux/clk.h>
#include <io.h>
#include <asm/system.h>

uint64_t armv8_clocksource_read(void)
{
	unsigned long cntpct;

	isb();
	asm volatile("mrs %0, cntpct_el0" : "=r" (cntpct));

	return cntpct;
}

static struct clocksource cs = {
	.read	= armv8_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(64),
	.shift	= 0,
};

static int armv8_timer_probe(struct device_d *dev)
{
	unsigned long cntfrq;

	asm volatile("mrs %0, cntfrq_el0" : "=r" (cntfrq));

	cs.mult = clocksource_hz2mult(cntfrq, cs.shift);

	return init_clock(&cs);
}

static struct of_device_id armv8_timer_dt_ids[] = {
	{ .compatible = "arm,armv8-timer", },
	{ }
};

static struct driver_d armv8_timer_driver = {
	.name = "armv8-timer",
	.probe = armv8_timer_probe,
	.of_compatible = DRV_OF_COMPAT(armv8_timer_dt_ids),
};

static int armv8_timer_init(void)
{
	return platform_driver_register(&armv8_timer_driver);
}
postcore_initcall(armv8_timer_init);

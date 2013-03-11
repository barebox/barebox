/*
 * Copyright (C) 2013 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <clock.h>
#include <io.h>
#include <init.h>

#include <linux/clk.h>
#include <linux/err.h>

static __iomem void *clps711x_timer_base;

static uint64_t clps711x_cs_read(void)
{
	return ~readw(clps711x_timer_base);
}

static struct clocksource clps711x_cs = {
	.read = clps711x_cs_read,
	.mask = CLOCKSOURCE_MASK(16),
};

static int clps711x_cs_probe(struct device_d *dev)
{
	u32 rate;
	struct clk *timer_clk;

	timer_clk = clk_get(dev, NULL);
	if (IS_ERR(timer_clk))
		return PTR_ERR(timer_clk);

	rate = clk_get_rate(timer_clk);
	clps711x_timer_base = dev_request_mem_region(dev, 0);
	if (!clps711x_timer_base) {
		clk_put(timer_clk);
		return -ENOENT;
	}

	clocks_calc_mult_shift(&clps711x_cs.mult, &clps711x_cs.shift, rate,
			       NSEC_PER_SEC, 10);

	return init_clock(&clps711x_cs);
}

static struct driver_d clps711x_cs_driver = {
	.name = "clps711x-cs",
	.probe = clps711x_cs_probe,
};

static __init int clps711x_cs_init(void)
{
	return platform_driver_register(&clps711x_cs_driver);
}
coredevice_initcall(clps711x_cs_init);

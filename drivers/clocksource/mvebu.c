/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#define TIMER_CTRL_OFF          0x0000
#define  TIMER0_EN               0x0001
#define  TIMER0_RELOAD_EN        0x0002
#define  TIMER0_25MHZ            0x0800
#define  TIMER0_DIV(div)         ((div) << 19)
#define  TIMER1_EN               0x0004
#define  TIMER1_RELOAD_EN        0x0008
#define  TIMER1_25MHZ            0x1000
#define  TIMER1_DIV(div)         ((div) << 22)
#define TIMER_EVENTS_STATUS     0x0004
#define  TIMER0_CLR_MASK         (~0x1)
#define  TIMER1_CLR_MASK         (~0x100)
#define TIMER0_RELOAD_OFF       0x0010
#define TIMER0_VAL_OFF          0x0014
#define TIMER1_RELOAD_OFF       0x0018
#define TIMER1_VAL_OFF          0x001c

#define TIMER_DIVIDER_SHIFT     5

static __iomem void *timer_base;

uint64_t mvebu_clocksource_read(void)
{
	return 0 - __raw_readl(timer_base + TIMER0_VAL_OFF);
}

static struct clocksource cs = {
	.read	= mvebu_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static int mvebu_timer_probe(struct device_d *dev)
{
	struct clk *tclk;
	u32 val;

	timer_base = dev_request_mem_region(dev, 0);
	tclk = clk_get(dev, NULL);

	val = __raw_readl(timer_base + TIMER_CTRL_OFF);
	val &= ~TIMER0_25MHZ;
	__raw_writel(val, timer_base + TIMER_CTRL_OFF);

	__raw_writel(0xffffffff, timer_base + TIMER0_VAL_OFF);
	__raw_writel(0xffffffff, timer_base + TIMER0_RELOAD_OFF);

	val = __raw_readl(timer_base + TIMER_CTRL_OFF);
	val |= TIMER0_EN | TIMER0_RELOAD_EN | TIMER0_DIV(TIMER_DIVIDER_SHIFT);
	__raw_writel(val, timer_base + TIMER_CTRL_OFF);

	cs.mult = clocksource_hz2mult(clk_get_rate(tclk), cs.shift);

	init_clock(&cs);

	return 0;
}

static struct of_device_id mvebu_timer_dt_ids[] = {
	{ .compatible = "marvell,armada-370-xp-timer", },
	{ }
};

static struct driver_d mvebu_timer_driver = {
	.name = "mvebu-timer",
	.probe = mvebu_timer_probe,
	.of_compatible = DRV_OF_COMPAT(mvebu_timer_dt_ids),
};

static int mvebu_timer_init(void)
{
	return platform_driver_register(&mvebu_timer_driver);
}
postcore_initcall(mvebu_timer_init);

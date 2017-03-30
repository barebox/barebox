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

#define TIMER_CTRL_OFF		0x0000
#define  TIMER0_EN		BIT(0)
#define  TIMER0_RELOAD_EN	BIT(1)
#define  TIMER0_25MHZ		BIT(11)
#define  TIMER0_DIV(div)	((div) << 19)
#define  TIMER0_DIV_MASK	TIMER0_DIV(0x7)
#define  TIMER1_EN		BIT(2)
#define  TIMER1_RELOAD_EN	BIT(3)
#define  TIMER1_25MHZ		BIT(12)
#define  TIMER1_DIV(div)	((div) << 22)
#define  TIMER1_DIV_MASK	TIMER1_DIV(0x7)
#define TIMER_EVENTS_STATUS	0x0004
#define  TIMER0_CLR_MASK	(~BIT(0))
#define  TIMER1_CLR_MASK	(~BIT(9))
#define TIMER0_RELOAD_OFF	0x0010
#define TIMER0_VAL_OFF		0x0014
#define TIMER1_RELOAD_OFF	0x0018
#define TIMER1_VAL_OFF		0x001c

#define TIMER_DIVIDER_SHIFT	5
#define TIMER_DIVIDER		BIT(TIMER_DIVIDER_SHIFT)

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
	struct resource *iores;
	struct clk *clk;
	u32 rate, div, val;

	iores = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	timer_base = IOMEM(iores->start);

	val = __raw_readl(timer_base + TIMER_CTRL_OFF);
	val &= ~(TIMER0_25MHZ | TIMER0_DIV_MASK);
	if (of_device_is_compatible(dev->device_node,
				    "marvell,armada-370-timer")) {
		clk = clk_get(dev, NULL);
		div = TIMER_DIVIDER;
		val |= TIMER0_DIV(TIMER_DIVIDER_SHIFT);
		rate = clk_get_rate(clk) / TIMER_DIVIDER;
	} else {
		clk = clk_get(dev, "fixed");
		div = 1;
		val |= TIMER0_25MHZ;
		rate = clk_get_rate(clk);
	}
	__raw_writel(val, timer_base + TIMER_CTRL_OFF);

	__raw_writel(0xffffffff, timer_base + TIMER0_VAL_OFF);
	__raw_writel(0xffffffff, timer_base + TIMER0_RELOAD_OFF);

	val = __raw_readl(timer_base + TIMER_CTRL_OFF);
	val |= TIMER0_EN | TIMER0_RELOAD_EN;
	__raw_writel(val, timer_base + TIMER_CTRL_OFF);

	cs.mult = clocksource_hz2mult(rate, cs.shift);

	return init_clock(&cs);
}

static struct of_device_id mvebu_timer_dt_ids[] = {
	{ .compatible = "marvell,armada-370-timer", },
	{ .compatible = "marvell,armada-xp-timer", },
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

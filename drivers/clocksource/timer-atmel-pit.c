/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <mach/hardware.h>
#include <mach/at91_pit.h>
#include <mach/io.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>

#define PIT_CPIV(x)	((x) & AT91_PIT_CPIV)
#define pit_write(reg, val)	__raw_writel(val, pit_base + reg)
#define pit_read(reg)		__raw_readl(pit_base + reg)

static __iomem void *pit_base;

uint64_t at91sam9_clocksource_read(void)
{
	return pit_read(AT91_PIT_PIIR);
}

static struct clocksource cs = {
	.read	= at91sam9_clocksource_read,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 10,
};

static void at91_pit_stop(void)
{
	/* Disable timer and irqs */
	pit_write(AT91_PIT_MR, 0);

	/* Clear any pending interrupts, wait for PIT to stop counting */
	while (PIT_CPIV(pit_read(AT91_PIT_PIVR)) != 0);
}

static void at91sam926x_pit_reset(void)
{
	at91_pit_stop();

	/* Start PIT but don't enable IRQ */
	pit_write(AT91_PIT_MR, 0xfffff | AT91_PIT_PITEN);
}

static int at91_pit_probe(struct device_d *dev)
{
	struct clk *clk;
	u32 pit_rate;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(clk);
	if (ret < 0) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		clk_put(clk);
		return ret;
	}

	pit_base = dev_request_mem_region_err_null(dev, 0);
	if (!pit_base)
		return -ENOENT;

	pit_rate = clk_get_rate(clk) / 16;

	at91sam926x_pit_reset();

	cs.mult = clocksource_hz2mult(pit_rate, cs.shift);

	return init_clock(&cs);
}

static struct driver_d at91_pit_driver = {
	.name = "at91-pit",
	.probe = at91_pit_probe,
};

static int at91_pit_init(void)
{
	return platform_driver_register(&at91_pit_driver);
}
postcore_initcall(at91_pit_init);

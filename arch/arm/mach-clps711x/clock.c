/*
 * Copyright (C) 2012 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <clock.h>
#include <asm/io.h>
#include <linux/clkdev.h>

#include <mach/clps711x.h>

struct clk {
	unsigned long	rate;
};

static struct clk uart_clk, bus_clk;

uint64_t clocksource_read(void)
{
	return ~readw(TC2D);
}

static struct clocksource cs = {
	.read	= clocksource_read,
	.mask	= CLOCKSOURCE_MASK(16),
};

unsigned long clk_get_rate(struct clk *clk)
{
	return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

int clk_enable(struct clk *clk)
{
	/* Do nothing */
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	/* Do nothing */
}
EXPORT_SYMBOL(clk_disable);

static int clocks_init(void)
{
	int osc, ext, pll, cpu, timer;
	u32 tmp;

	osc = 3686400;
	ext = 13000000;

	tmp = readl(PLLR) >> 24;
	if (tmp)
		pll = (osc * tmp) / 2;
	else
		pll = 73728000; /* Default value for old CPUs */

	tmp = readl(SYSFLG2);
	if (tmp & SYSFLG2_CKMODE) {
		cpu = ext;
		bus_clk.rate = cpu;
	} else {
		cpu = pll;
		if (cpu >= 36864000)
			bus_clk.rate = cpu / 2;
		else
			bus_clk.rate = 36864000 / 2;
	}

	uart_clk.rate = bus_clk.rate / 10;

	if (tmp & SYSFLG2_CKMODE) {
		tmp = readw(SYSCON2);
		if (tmp & SYSCON2_OSTB)
			timer = ext / 26;
		else
			timer = 541440;
	} else
		timer = cpu / 144;

	tmp = readl(SYSCON1);
	tmp &= ~SYSCON1_TC2M;	/* Free running mode */
	tmp |= SYSCON1_TC2S;	/* High frequency source */
	writel(tmp, SYSCON1);

	clocks_calc_mult_shift(&cs.mult, &cs.shift, timer, NSEC_PER_SEC, 10);

	return init_clock(&cs);
}
core_initcall(clocks_init);

static struct clk_lookup clocks_lookups[] = {
	CLKDEV_CON_ID("bus", &bus_clk),
	CLKDEV_DEV_ID("clps711x_serial0", &uart_clk),
	CLKDEV_DEV_ID("clps711x_serial1", &uart_clk),
};

static int clkdev_init(void)
{
	clkdev_add_table(clocks_lookups, ARRAY_SIZE(clocks_lookups));

	return 0;
}
postcore_initcall(clkdev_init);

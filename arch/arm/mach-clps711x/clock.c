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
#include <sizes.h>
#include <asm/io.h>
#include <linux/clkdev.h>

#include <mach/clps711x.h>

#define CLPS711X_OSC_FREQ	3686400
#define CLPS711X_EXT_FREQ	13000000

static struct clk {
	unsigned long	rate;
} uart_clk, bus_clk, timer_clk;

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
	int pll, cpu;
	u32 tmp;

	tmp = readl(PLLR) >> 24;
	if (tmp)
		pll = (CLPS711X_OSC_FREQ * tmp) / 2;
	else
		pll = 73728000; /* Default value for old CPUs */

	tmp = readl(SYSFLG2);
	if (tmp & SYSFLG2_CKMODE) {
		cpu = CLPS711X_EXT_FREQ;
		bus_clk.rate = CLPS711X_EXT_FREQ;
	} else {
		cpu = pll;
		if (cpu >= 36864000)
			bus_clk.rate = cpu / 2;
		else
			bus_clk.rate = 36864000 / 2;
	}

	uart_clk.rate = DIV_ROUND_CLOSEST(bus_clk.rate, 10);

	if (tmp & SYSFLG2_CKMODE) {
		tmp = readw(SYSCON2);
		if (tmp & SYSCON2_OSTB)
			timer_clk.rate = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 26);
		else
			timer_clk.rate = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 24);
	} else
		timer_clk.rate = DIV_ROUND_CLOSEST(cpu, 144);

	tmp = readl(SYSCON1);
	tmp &= ~SYSCON1_TC2M;	/* Free running mode */
	tmp |= SYSCON1_TC2S;	/* High frequency source */
	writel(tmp, SYSCON1);

	return 0;
}
core_initcall(clocks_init);

static struct clk_lookup clocks_lookups[] = {
	CLKDEV_CON_ID("bus", &bus_clk),
	CLKDEV_DEV_ID("clps711x_serial0", &uart_clk),
	CLKDEV_DEV_ID("clps711x_serial1", &uart_clk),
	CLKDEV_DEV_ID("clps711x-cs", &timer_clk),
};

static int clkdev_init(void)
{
	clkdev_add_table(clocks_lookups, ARRAY_SIZE(clocks_lookups));

	return 0;
}
postcore_initcall(clkdev_init);

static const char *clps711x_clocksrc_name = "clps711x-cs";

static __init int clps711x_core_init(void)
{
	add_generic_device(clps711x_clocksrc_name, DEVICE_ID_SINGLE, NULL,
			   TC2D, SZ_2, IORESOURCE_MEM, NULL);
	return 0;
}
coredevice_initcall(clps711x_core_init);

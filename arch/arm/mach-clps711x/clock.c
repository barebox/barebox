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
#include <linux/clk.h>
#include <linux/clkdev.h>

#include <mach/clps711x.h>

#define CLPS711X_OSC_FREQ	3686400
#define CLPS711X_EXT_FREQ	13000000

enum clps711x_clks {
	dummy, cpu, bus, uart, timer_hf, timer_lf, tc1, tc2, clk_max
};

static struct {
	const char	*name;
	struct clk	*clk;
} clks[clk_max] = {
	{ "dummy", },
	{ "cpu", },
	{ "bus", },
	{ "uart", },
	{ "timer_hf", },
	{ "timer_lf", },
	{ "tc1", },
	{ "tc2", },
};

static const char *tc_sel_clks[] = {
	"timer_lf",
	"timer_hf",
};

static __init void clps711x_clk_register(enum clps711x_clks id)
{
	clk_register_clkdev(clks[id].clk, clks[id].name, NULL);
}

static __init int clps711x_clk_init(void)
{
	unsigned int f_cpu, f_bus, f_uart, f_timer_hf, f_timer_lf, pll;
	u32 tmp;

	tmp = readl(PLLR) >> 24;
	if (tmp)
		pll = (CLPS711X_OSC_FREQ * tmp) / 2;
	else
		pll = 73728000; /* Default value for old CPUs */

	tmp = readl(SYSFLG2);
	if (tmp & SYSFLG2_CKMODE) {
		f_cpu = CLPS711X_EXT_FREQ;
		f_bus = CLPS711X_EXT_FREQ;
	} else {
		f_cpu = pll;
		if (f_cpu >= 36864000)
			f_bus = f_cpu / 2;
		else
			f_bus = 36864000 / 2;
	}

	f_uart = f_bus / 10;

	if (tmp & SYSFLG2_CKMODE) {
		tmp = readw(SYSCON2);
		if (tmp & SYSCON2_OSTB)
			f_timer_hf = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 26);
		else
			f_timer_hf = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 24);
	} else
		f_timer_hf = DIV_ROUND_CLOSEST(f_cpu, 144);

	f_timer_lf = DIV_ROUND_CLOSEST(f_timer_hf, 256);

	/* Turn timers in free running mode */
	tmp = readl(SYSCON1);
	tmp &= ~(SYSCON1_TC1M | SYSCON1_TC2M);
	writel(tmp, SYSCON1);

	clks[dummy].clk = clk_fixed(clks[dummy].name, 0);
	clks[cpu].clk = clk_fixed(clks[cpu].name, f_cpu);
	clks[bus].clk = clk_fixed(clks[bus].name, f_bus);
	clks[uart].clk = clk_fixed(clks[uart].name, f_uart);
	clks[timer_hf].clk = clk_fixed(clks[timer_hf].name, f_timer_hf);
	clks[timer_lf].clk = clk_fixed(clks[timer_lf].name, f_timer_lf);
	clks[tc1].clk = clk_mux(clks[tc1].name, IOMEM(SYSCON1), 5, 1,
				tc_sel_clks, ARRAY_SIZE(tc_sel_clks));
	clks[tc2].clk = clk_mux(clks[tc2].name, IOMEM(SYSCON1), 7, 1,
				tc_sel_clks, ARRAY_SIZE(tc_sel_clks));

	clps711x_clk_register(dummy);
	clps711x_clk_register(cpu);
	clps711x_clk_register(bus);
	clps711x_clk_register(uart);
	clps711x_clk_register(timer_hf);
	clps711x_clk_register(timer_lf);
	clps711x_clk_register(tc1);
	clps711x_clk_register(tc2);

	return 0;
}
postcore_initcall(clps711x_clk_init);

static const char *clps711x_clocksrc_name = "clps711x-cs";

static __init int clps711x_core_init(void)
{
	/* Using TC2 in low frequency mode as clocksource */
	clk_set_parent(clks[tc2].clk, clks[timer_lf].clk);
	clk_add_alias(NULL, clps711x_clocksrc_name, "tc2", NULL);
	add_generic_device(clps711x_clocksrc_name, DEVICE_ID_SINGLE, NULL,
			   TC2D, SZ_2, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(clps711x_core_init);

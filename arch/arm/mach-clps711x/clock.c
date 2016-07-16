/*
 * Copyright (C) 2012-2016 Alexander Shiyan <shc_work@mail.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/sizes.h>
#include <dt-bindings/clock/clps711x-clock.h>

#include <mach/clps711x.h>

#define CLPS711X_OSC_FREQ	3686400
#define CLPS711X_EXT_FREQ	13000000

static struct clk *clks[CLPS711X_CLK_MAX];

static struct clk_div_table tdiv_tbl[] = {
	{ .val = 0, .div = 256, },
	{ .val = 1, .div = 1, },
};

static __init int clps711x_clk_init(void)
{
	unsigned int f_cpu, f_bus, f_uart, f_timer_ref, pll;
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
			f_timer_ref = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 26);
		else
			f_timer_ref = DIV_ROUND_CLOSEST(CLPS711X_EXT_FREQ, 24);
	} else
		f_timer_ref = DIV_ROUND_CLOSEST(f_cpu, 144);

	/* Turn timers in free running mode */
	tmp = readl(SYSCON1);
	tmp &= ~(SYSCON1_TC1M | SYSCON1_TC2M);
	writel(tmp, SYSCON1);

	clks[CLPS711X_CLK_DUMMY] = clk_fixed("dummy", 0);
	clks[CLPS711X_CLK_CPU] = clk_fixed("cpu", f_cpu);
	clks[CLPS711X_CLK_BUS] = clk_fixed("bus", f_bus);
	clks[CLPS711X_CLK_UART] = clk_fixed("uart", f_uart);
	clks[CLPS711X_CLK_TIMERREF] = clk_fixed("timer_ref", f_timer_ref);
	clks[CLPS711X_CLK_TIMER1] = clk_divider_table("timer1", "timer_ref",
		IOMEM(SYSCON1), 5, 1, tdiv_tbl, ARRAY_SIZE(tdiv_tbl));
	clks[CLPS711X_CLK_TIMER2] = clk_divider_table("timer2", "timer_ref",
		IOMEM(SYSCON1), 7, 1, tdiv_tbl, ARRAY_SIZE(tdiv_tbl));

	clkdev_add_physbase(clks[CLPS711X_CLK_UART], UARTDR1, NULL);
	clkdev_add_physbase(clks[CLPS711X_CLK_UART], UARTDR2, NULL);
	clkdev_add_physbase(clks[CLPS711X_CLK_TIMER2], TC2D, NULL);

	return 0;
}
postcore_initcall(clps711x_clk_init);

static __init int clps711x_core_init(void)
{
	add_generic_device("clps711x-cs", DEVICE_ID_SINGLE, NULL,
			   TC2D, SZ_2, IORESOURCE_MEM, NULL);

	return 0;
}
coredevice_initcall(clps711x_core_init);

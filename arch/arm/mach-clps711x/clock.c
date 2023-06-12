// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Alexander Shiyan <shc_work@mail.ru>

#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/sizes.h>
#include <dt-bindings/clock/clps711x-clock.h>

#include <mach/clps711x/clps711x.h>

#define CLPS711X_OSC_FREQ	3686400
#define CLPS711X_EXT_FREQ	13000000

static struct clk *clks[CLPS711X_CLK_MAX];
static struct clk_onecell_data clk_data;

static const struct clk_div_table tdiv_tbl[] = {
	{ .val = 0, .div = 256, },
	{ .val = 1, .div = 1, },
	{ }
};

static int clps711x_clk_probe(struct device *dev)
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

	f_uart = DIV_ROUND_CLOSEST(f_bus, 10);

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
	clks[CLPS711X_CLK_TIMER1] = clk_divider_table("timer1", "timer_ref", 0,
		IOMEM(SYSCON1), 5, 1, tdiv_tbl, 0);
	clks[CLPS711X_CLK_TIMER2] = clk_divider_table("timer2", "timer_ref", 0,
		IOMEM(SYSCON1), 7, 1, tdiv_tbl, 0);

	clk_data.clks = clks;
	clk_data.clk_num = CLPS711X_CLK_MAX;
	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &clk_data);

	return 0;
}

static const struct of_device_id __maybe_unused clps711x_clk_dt_ids[] = {
	{ .compatible = "cirrus,ep7209-clk", },
	{ }
};
MODULE_DEVICE_TABLE(of, clps711x_clk_dt_ids);

static struct driver clps711x_clk_driver = {
	.probe = clps711x_clk_probe,
	.name = "clps711x-clk",
	.of_compatible = DRV_OF_COMPAT(clps711x_clk_dt_ids),
};
postcore_platform_driver(clps711x_clk_driver);

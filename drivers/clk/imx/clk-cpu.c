/*
 * Copyright (c) 2014 Lucas Stach <l.stach@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <malloc.h>
#include <clock.h>

#include "clk.h"

struct clk_cpu {
	struct clk	clk;
	struct clk	*div;
	struct clk	*mux;
	struct clk	*pll;
	struct clk	*step;
};

static inline struct clk_cpu *to_clk_cpu(struct clk *clk)
{
	return container_of(clk, struct clk_cpu, clk);
}

static unsigned long clk_cpu_recalc_rate(struct clk *clk,
					 unsigned long parent_rate)
{
	struct clk_cpu *cpu = to_clk_cpu(clk);

	return clk_get_rate(cpu->div);
}

static long clk_cpu_round_rate(struct clk *clk, unsigned long rate,
			       unsigned long *prate)
{
	struct clk_cpu *cpu = to_clk_cpu(clk);

	return clk_round_rate(cpu->pll, rate);
}

static int clk_cpu_set_rate(struct clk *clk, unsigned long rate,
			    unsigned long parent_rate)
{
	struct clk_cpu *cpu = to_clk_cpu(clk);
	int ret;

	/* switch to PLL bypass clock */
	ret = clk_set_parent(cpu->mux, cpu->step);
	if (ret)
		return ret;

	/* reprogram PLL */
	ret = clk_set_rate(cpu->pll, rate);
	if (ret) {
		clk_set_parent(cpu->mux, cpu->pll);
		return ret;
	}
	/* switch back to PLL clock */
	clk_set_parent(cpu->mux, cpu->pll);

	/* Ensure the divider is what we expect */
	clk_set_rate(cpu->div, rate);

	return 0;
}

static const struct clk_ops clk_cpu_ops = {
	.recalc_rate	= clk_cpu_recalc_rate,
	.round_rate	= clk_cpu_round_rate,
	.set_rate	= clk_cpu_set_rate,
};

struct imx_clk_cpu {
	struct clk_cpu cpu;
	const char *parent_name;
};

struct clk *imx_clk_cpu(const char *name, const char *parent_name,
		struct clk *div, struct clk *mux, struct clk *pll,
		struct clk *step)
{
	struct imx_clk_cpu *icpu;
	struct clk_cpu *cpu;
	int ret;

	icpu = xzalloc(sizeof(*icpu));
	icpu->parent_name = parent_name;
	cpu = &icpu->cpu;

	cpu->div = div;
	cpu->mux = mux;
	cpu->pll = pll;
	cpu->step = step;

	cpu->clk.name = name;
	cpu->clk.ops = &clk_cpu_ops;
	cpu->clk.flags = 0;
	cpu->clk.parent_names = &icpu->parent_name;
	cpu->clk.num_parents = 1;

	ret = clk_register(&cpu->clk);
	if (ret)
		free(cpu);

	return &cpu->clk;
}

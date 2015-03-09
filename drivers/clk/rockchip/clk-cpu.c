/*
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * based on clk/samsung/clk-cpu.c
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Author: Thomas Abraham <thomas.ab@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * A CPU clock is defined as a clock supplied to a CPU or a group of CPUs.
 * The CPU clock is typically derived from a hierarchy of clock
 * blocks which includes mux and divider blocks. There are a number of other
 * auxiliary clocks supplied to the CPU domain such as the debug blocks and AXI
 * clock for CPU domain. The rates of these auxiliary clocks are related to the
 * CPU clock rate and this relation is usually specified in the hardware manual
 * of the SoC or supplied after the SoC characterization.
 *
 * The below implementation of the CPU clock allows the rate changes of the CPU
 * clock and the corresponding rate changes of the auxillary clocks of the CPU
 * domain. The platform clock driver provides a clock register configuration
 * for each configurable rate which is then used to program the clock hardware
 * registers to acheive a fast co-oridinated rate change for all the CPU domain
 * clocks.
 *
 * On a rate change request for the CPU clock, the rate change is propagated
 * upto the PLL supplying the clock to the CPU domain clock blocks. While the
 * CPU domain PLL is reconfigured, the CPU domain clocks are driven using an
 * alternate clock source. If required, the alternate clock source is divided
 * down in order to keep the output clock rate within the previous OPP limits.
 */

#include <common.h>
#include <of.h>
#include <malloc.h>
#include <io.h>
#include <xfuncs.h>
#include "clk.h"
#include <linux/barebox-wrapper.h>

/**
 * struct rockchip_cpuclk: information about clock supplied to a CPU core.
 * @hw:		handle between ccf and cpu clock.
 * @alt_parent:	alternate parent clock to use when switching the speed
 *		of the primary parent clock.
 * @reg_base:	base register for cpu-clock values.
 * @rate_count:	number of rates in the rate_table
 * @rate_table:	pll-rates and their associated dividers
 * @reg_data:	cpu-specific register settings
 */
struct rockchip_cpuclk {
	struct clk				hw;

	struct clk				*alt_parent;
	void __iomem				*reg_base;
	unsigned int				rate_count;
	struct rockchip_cpuclk_rate_table	*rate_table;
	const struct rockchip_cpuclk_reg_data	*reg_data;
};

#define to_rockchip_cpuclk_hw(hw) container_of(hw, struct rockchip_cpuclk, hw)

static unsigned long rockchip_cpuclk_recalc_rate(struct clk *hw,
					unsigned long parent_rate)
{
	struct rockchip_cpuclk *cpuclk = to_rockchip_cpuclk_hw(hw);
	const struct rockchip_cpuclk_reg_data *reg_data = cpuclk->reg_data;
	u32 clksel0 = readl(cpuclk->reg_base + reg_data->core_reg);

	clksel0 >>= reg_data->div_core_shift;
	clksel0 &= reg_data->div_core_mask;
	return parent_rate / (clksel0 + 1);
}

static const struct clk_ops rockchip_cpuclk_ops = {
	.recalc_rate = rockchip_cpuclk_recalc_rate,
};

struct clk *rockchip_clk_register_cpuclk(const char *name,
			const char **parent_names, u8 num_parents,
			const struct rockchip_cpuclk_reg_data *reg_data,
			const struct rockchip_cpuclk_rate_table *rates,
			int nrates, void __iomem *reg_base)
{
	struct rockchip_cpuclk *cpuclk;
	struct clk *clk;
	int ret;

	if (num_parents != 2) {
		pr_err("%s: needs two parent clocks\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	cpuclk = kzalloc(sizeof(*cpuclk), GFP_KERNEL);
	if (!cpuclk)
		return ERR_PTR(-ENOMEM);

	cpuclk->hw.name = name;
	cpuclk->hw.parent_names = &parent_names[0];
	cpuclk->hw.num_parents = 1;
	cpuclk->hw.ops = &rockchip_cpuclk_ops;

	/* only allow rate changes when we have a rate table */
	cpuclk->hw.flags = (nrates > 0) ? CLK_SET_RATE_PARENT : 0;

	cpuclk->reg_base = reg_base;
	cpuclk->reg_data = reg_data;

	cpuclk->alt_parent = __clk_lookup(parent_names[1]);
	if (!cpuclk->alt_parent) {
		pr_err("%s: could not lookup alternate parent\n",
		       __func__);
		ret = -EINVAL;
		goto free_cpuclk;
	}

	ret = clk_enable(cpuclk->alt_parent);
	if (ret) {
		pr_err("%s: could not enable alternate parent\n",
		       __func__);
		goto free_cpuclk;
	}

	clk = __clk_lookup(parent_names[0]);
	if (!clk) {
		pr_err("%s: could not lookup parent clock %s\n",
		       __func__, parent_names[0]);
		ret = -EINVAL;
		goto free_cpuclk;
	}

	if (nrates > 0) {
		cpuclk->rate_count = nrates;
		cpuclk->rate_table = xmemdup(rates,
					     sizeof(*rates) * nrates
					     );
		if (!cpuclk->rate_table) {
			pr_err("%s: could not allocate memory for cpuclk rates\n",
			       __func__);
			ret = -ENOMEM;
			goto free_cpuclk;
		}
	}

	ret = clk_register(&cpuclk->hw);
	if (ret) {
		pr_err("%s: could not register cpuclk %s\n", __func__,	name);
		goto free_rate_table;
	}

	return &cpuclk->hw;

free_rate_table:
	kfree(cpuclk->rate_table);
free_cpuclk:
	kfree(cpuclk);
	return ERR_PTR(ret);
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014 MundoReader S.L.
 * Author: Heiko Stuebner <heiko@sntech.de>
 *
 * based on clk/samsung/clk-cpu.c
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 * Author: Thomas Abraham <thomas.ab@samsung.com>
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
#include <linux/barebox-wrapper.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include "clk.h"

/**
 * struct rockchip_cpuclk: information about clock supplied to a CPU core.
 * @hw:		handle between ccf and cpu clock.
 * @alt_parent:	alternate parent clock to use when switching the speed
 *		of the primary parent clock.
 * @reg_base:	base register for cpu-clock values.
 * @clk_nb:	clock notifier registered for changes in clock speed of the
 *		primary parent clock.
 * @rate_count:	number of rates in the rate_table
 * @rate_table:	pll-rates and their associated dividers
 * @reg_data:	cpu-specific register settings
 * @lock:	clock lock
 */
struct rockchip_cpuclk {
	struct clk_hw				hw;
	struct clk				*alt_parent;
	void __iomem				*reg_base;
	unsigned int				rate_count;
	struct rockchip_cpuclk_rate_table	*rate_table;
	const struct rockchip_cpuclk_reg_data	*reg_data;
	spinlock_t				*lock;
};

#define to_rockchip_cpuclk_hw(hw) container_of(hw, struct rockchip_cpuclk, hw)

static unsigned long rockchip_cpuclk_recalc_rate(struct clk_hw *hw,
					unsigned long parent_rate)
{
	struct rockchip_cpuclk *cpuclk = to_rockchip_cpuclk_hw(hw);
	const struct rockchip_cpuclk_reg_data *reg_data = cpuclk->reg_data;
	u32 clksel0 = readl_relaxed(cpuclk->reg_base + reg_data->core_reg[0]);

	clksel0 >>= reg_data->div_core_shift[0];
	clksel0 &= reg_data->div_core_mask[0];
	return parent_rate / (clksel0 + 1);
}

static const struct clk_ops rockchip_cpuclk_ops = {
	.recalc_rate = rockchip_cpuclk_recalc_rate,
};

struct clk *rockchip_clk_register_cpuclk(const char *name,
			const char *const *parent_names, u8 num_parents,
			const struct rockchip_cpuclk_reg_data *reg_data,
			const struct rockchip_cpuclk_rate_table *rates,
			int nrates, void __iomem *reg_base, spinlock_t *lock)
{
	struct rockchip_cpuclk *cpuclk;
	struct clk_init_data init;
	struct clk *clk, *cclk;
	int ret;

	if (num_parents < 2) {
		pr_err("%s: needs at least two parent clocks\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	cpuclk = kzalloc(sizeof(*cpuclk), GFP_KERNEL);
	if (!cpuclk)
		return ERR_PTR(-ENOMEM);

	init.name = name;
	init.parent_names = &parent_names[reg_data->mux_core_main];
	init.num_parents = 1;
	init.ops = &rockchip_cpuclk_ops;

	/* only allow rate changes when we have a rate table */
	init.flags = (nrates > 0) ? CLK_SET_RATE_PARENT : 0;

	/* disallow automatic parent changes by ccf */
	init.flags |= CLK_SET_RATE_NO_REPARENT;

	init.flags |= CLK_GET_RATE_NOCACHE;

	cpuclk->reg_base = reg_base;
	cpuclk->lock = lock;
	cpuclk->reg_data = reg_data;
	cpuclk->hw.init = &init;

	cpuclk->alt_parent = __clk_lookup(parent_names[reg_data->mux_core_alt]);
	if (!cpuclk->alt_parent) {
		pr_err("%s: could not lookup alternate parent: (%d)\n",
		       __func__, reg_data->mux_core_alt);
		ret = -EINVAL;
		goto free_cpuclk;
	}

	ret = clk_enable(cpuclk->alt_parent);
	if (ret) {
		pr_err("%s: could not enable alternate parent\n",
		       __func__);
		goto free_cpuclk;
	}

	clk = __clk_lookup(parent_names[reg_data->mux_core_main]);
	if (!clk) {
		pr_err("%s: could not lookup parent clock: (%d) %s\n",
		       __func__, reg_data->mux_core_main,
		       parent_names[reg_data->mux_core_main]);
		ret = -EINVAL;
		goto free_alt_parent;
	}

	if (nrates > 0) {
		cpuclk->rate_count = nrates;
		cpuclk->rate_table = kmemdup(rates,
					     sizeof(*rates) * nrates,
					     GFP_KERNEL);
		if (!cpuclk->rate_table) {
			ret = -ENOMEM;
			goto unregister_notifier;
		}
	}

	cclk = clk_register(NULL, &cpuclk->hw);
	if (IS_ERR(cclk)) {
		pr_err("%s: could not register cpuclk %s\n", __func__,	name);
		ret = PTR_ERR(cclk);
		goto free_rate_table;
	}

	return cclk;

free_rate_table:
	kfree(cpuclk->rate_table);
unregister_notifier:
free_alt_parent:
	clk_disable(cpuclk->alt_parent);
free_cpuclk:
	kfree(cpuclk);
	return ERR_PTR(ret);
}

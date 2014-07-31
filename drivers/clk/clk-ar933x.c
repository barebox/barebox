/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <mach/ath79.h>
#include <dt-bindings/clock/ar933x-clk.h>

static struct clk *clks[AR933X_CLK_END];
static struct clk_onecell_data clk_data;

struct clk_ar933x {
	struct clk clk;
	void __iomem *base;
	u32 div_shift;
	u32 div_mask;
	const char *parent;
};

static unsigned long clk_ar933x_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	struct clk_ar933x *f = container_of(clk, struct clk_ar933x, clk);
	unsigned long rate;
	unsigned long freq;
	u32 clock_ctrl;
	u32 cpu_config;
	u32 t;

	clock_ctrl = __raw_readl(f->base + AR933X_PLL_CLOCK_CTRL_REG);

	if (clock_ctrl & AR933X_PLL_CLOCK_CTRL_BYPASS) {
		rate = parent_rate;
	} else {
		cpu_config = __raw_readl(f->base + AR933X_PLL_CPU_CONFIG_REG);

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_REFDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_REFDIV_MASK;
		freq = parent_rate / t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_NINT_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_NINT_MASK;
		freq *= t;

		t = (cpu_config >> AR933X_PLL_CPU_CONFIG_OUTDIV_SHIFT) &
		    AR933X_PLL_CPU_CONFIG_OUTDIV_MASK;
		if (t == 0)
			t = 1;

		freq >>= t;

		t = ((clock_ctrl >> f->div_shift) & f->div_mask) + 1;
		rate = freq / t;
	}

	return rate;
}

struct clk_ops clk_ar933x_ops = {
	.recalc_rate = clk_ar933x_recalc_rate,
};

static struct clk *clk_ar933x(const char *name, const char *parent,
	void __iomem *base, u32 div_shift, u32 div_mask)
{
	struct clk_ar933x *f = xzalloc(sizeof(*f));

	f->parent = parent;
	f->base = base;
	f->div_shift = div_shift;
	f->div_mask = div_mask;

	f->clk.ops = &clk_ar933x_ops;
	f->clk.name = name;
	f->clk.parent_names = &f->parent;
	f->clk.num_parents = 1;

	clk_register(&f->clk);

	return &f->clk;
}

static void ar933x_ref_clk_init(void __iomem *base)
{
	u32 t;
	unsigned long ref_rate;

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	if (t & AR933X_BOOTSTRAP_REF_CLK_40)
		ref_rate = (40 * 1000 * 1000);
	else
		ref_rate = (25 * 1000 * 1000);

	clks[AR933X_CLK_REF] = clk_fixed("ref", ref_rate);
}

static void ar933x_pll_init(void __iomem *base)
{
	clks[AR933X_CLK_UART] = clk_fixed_factor("uart", "ref", 1, 1,
			CLK_SET_RATE_PARENT);

	clks[AR933X_CLK_CPU] = clk_ar933x("cpu", "ref", base,
		AR933X_PLL_CLOCK_CTRL_CPU_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_CPU_DIV_MASK);

	clks[AR933X_CLK_DDR] = clk_ar933x("ddr", "ref", base,
		AR933X_PLL_CLOCK_CTRL_DDR_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_DDR_DIV_MASK);

	clks[AR933X_CLK_AHB] = clk_ar933x("ahb", "ref", base,
		AR933X_PLL_CLOCK_CTRL_AHB_DIV_SHIFT,
		AR933X_PLL_CLOCK_CTRL_AHB_DIV_MASK);

	clks[AR933X_CLK_WDT] = clk_fixed_factor("wdt", "ahb", 1, 1,
			CLK_SET_RATE_PARENT);
}

static int ar933x_clk_probe(struct device_d *dev)
{
	void __iomem *base;

	base = dev_request_mem_region(dev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	ar933x_ref_clk_init(base);
	ar933x_pll_init(base);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->device_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id ar933x_clk_dt_ids[] = {
	{
		.compatible = "qca,ar933x-clk",
	}, {
		/* sentinel */
	}
};

static struct driver_d ar933x_clk_driver = {
	.probe	= ar933x_clk_probe,
	.name	= "ar933x_clk",
	.of_compatible = DRV_OF_COMPAT(ar933x_clk_dt_ids),
};

static int ar933x_clk_init(void)
{
	return platform_driver_register(&ar933x_clk_driver);
}
postcore_initcall(ar933x_clk_init);

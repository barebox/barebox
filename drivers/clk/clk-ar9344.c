// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2017 Oleksij Rempel <linux@rempel-privat.de>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <mach/ath79.h>
#include <dt-bindings/clock/ath79-clk.h>

#define AR9344_CPU_PLL_CONFIG			0x00
#define AR9344_DDR_PLL_CONFIG			0x04
#define AR9344_OUTDIV_M				0x3
#define AR9344_OUTDIV_S				19
#define AR9344_REFDIV_M				0x1f
#define AR9344_REFDIV_S				12
#define AR9344_NINT_M				0x3f
#define AR9344_NINT_S				6
#define AR9344_NFRAC_M				0x3f
#define AR9344_NFRAC_S				0


#define AR9344_CPU_DDR_CLOCK_CONTROL		0x08
#define AR9344_CPU_FROM_CPUPLL			BIT(20)
#define AR9344_CPU_PLL_BYPASS			BIT(2)
#define AR9344_CPU_POST_DIV_M			0x1f
#define AR9344_CPU_POST_DIV_S			5

static struct clk *clks[ATH79_CLK_END];
static struct clk_onecell_data clk_data;

struct clk_ar9344 {
	struct clk_hw hw;
	void __iomem *base;
	u32 div_shift;
	u32 div_mask;
	const char *parent;
};

static unsigned long clk_ar9344_recalc_rate(struct clk_hw *hw,
	unsigned long parent_rate)
{
	struct clk_ar9344 *f = container_of(hw, struct clk_ar9344, hw);
	int outdiv, refdiv, nint, nfrac;
	int cpu_post_div;
	u32 clock_ctrl;
	u32 val;

	clock_ctrl = __raw_readl(f->base + AR9344_CPU_DDR_CLOCK_CONTROL);
	cpu_post_div = ((clock_ctrl >> AR9344_CPU_POST_DIV_S)
			& AR9344_CPU_POST_DIV_M) + 1;
	if (clock_ctrl & AR9344_CPU_PLL_BYPASS) {
		return parent_rate;
	} else if (clock_ctrl & AR9344_CPU_FROM_CPUPLL) {
		val = __raw_readl(f->base + AR9344_CPU_PLL_CONFIG);
	} else {
		val = __raw_readl(f->base + AR9344_DDR_PLL_CONFIG);
	}

	outdiv = (val >> AR9344_OUTDIV_S) & AR9344_OUTDIV_M;
	refdiv = (val >> AR9344_REFDIV_S) & AR9344_REFDIV_M;
	nint = (val >> AR9344_NINT_S) & AR9344_NINT_M;
	nfrac = (val >> AR9344_NFRAC_S) & AR9344_NFRAC_M;

	return (parent_rate * (nint + (nfrac >> 9))) / (refdiv * (1 << outdiv));
}

struct clk_ops clk_ar9344_ops = {
	.recalc_rate = clk_ar9344_recalc_rate,
};

static struct clk *clk_ar9344(const char *name, const char *parent,
	void __iomem *base)
{
	struct clk_ar9344 *f = xzalloc(sizeof(*f));

	f->parent = parent;
	f->base = base;
	f->div_shift = 0;
	f->div_mask = 0;

	f->hw.clk.ops = &clk_ar9344_ops;
	f->hw.clk.name = name;
	f->hw.clk.parent_names = &f->parent;
	f->hw.clk.num_parents = 1;

	bclk_register(&f->hw.clk);

	return &f->hw.clk;
}

static void ar9344_pll_init(void __iomem *base)
{
	clks[ATH79_CLK_CPU] = clk_ar9344("cpu", "ref", base);
}

static int ar9344_clk_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	ar9344_pll_init(base);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get,
			    &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id ar9344_clk_dt_ids[] = {
	{
		.compatible = "qca,ar9344-pll",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ar9344_clk_dt_ids);

static struct driver ar9344_clk_driver = {
	.probe	= ar9344_clk_probe,
	.name	= "ar9344_clk",
	.of_compatible = DRV_OF_COMPAT(ar9344_clk_dt_ids),
};

postcore_platform_driver(ar9344_clk_driver);

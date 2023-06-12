// SPDX-License-Identifier: GPL-2.0-only
/*
 * Based on the ath79 clock code by Antony Pavlov <antonynpavlov@gmail.com>
 *  Barebox drivers/clk/clk-ar933x.c
 *
 * Copyright (C) 2020 Du Huanpeng <u74147@gmail.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include <dt-bindings/clock/ls1b-clk.h>

#define LS1B_CPU_DIV_SHIFT	20
#define LS1B_CPU_DIV_WIDTH	4

#define LS1B_DDR_DIV_SHIFT	14
#define LS1B_DDR_DIV_WIDTH	4

#define LS1B_DC_DIV_SHIFT	26
#define LS1B_DC_DIV_WIDTH	4

#define LS1B_CLK_APB_MULT	1
#define LS1B_CLK_APB_DIV2	2

/* register offset */
#define PLL_FREQ	0
#define PLL_DIV_PARAM	4

static struct clk *clks[LS1B_CLK_END];
static struct clk_onecell_data clk_data;

struct clk_ls1b200 {
	struct clk_hw hw;
	void __iomem *base;
	int div_shift;
	int div_mask;
	const char *parent;
};

static unsigned long clk_ls1b200_recalc_rate(struct clk_hw *hw, unsigned long parent_rate)
{
	int n;
	unsigned long rate;
	int pll_freq;
	struct clk_ls1b200 *ls1bclk;

	ls1bclk = container_of(hw, struct clk_ls1b200, hw);
	pll_freq = __raw_readl(ls1bclk->base);

	n  = 12 * 1024;
	n += (pll_freq & 0x3F) * 1024;
	n += (pll_freq >> 8) & 0x3FF;

	rate = parent_rate / 2 / 1024;
	/* avoid overflow. */
	rate *= n;

	return rate;
}

struct clk_ops clk_ls1b200_ops = {
	.recalc_rate = clk_ls1b200_recalc_rate,
};

static struct clk *clk_ls1b200(const char *name, const char *parent,
	void __iomem *base, int div_shift, int div_mask)
{
	struct clk_ls1b200 *f = xzalloc(sizeof(struct clk_ls1b200));

	f->parent = parent;
	f->base = base;
	f->div_shift = div_shift;
	f->div_mask = div_mask;

	f->hw.clk.ops = &clk_ls1b200_ops;
	f->hw.clk.name = name;
	f->hw.clk.parent_names = &f->parent;
	f->hw.clk.num_parents = 1;

	bclk_register(&f->hw.clk);

	return &f->hw.clk;
}

static const char * const cpu_mux[] = {"cpu_div", "oscillator", };
static const char * const ddr_mux[] = {"ddr_div", "oscillator", };
static const char * const dc_mux[]  = {"dc_div", "oscillator", };

static void ls1b200_pll_init(void __iomem *base)
{
	clks[LS1B_CLK_PLL] = clk_ls1b200("pll", "oscillator", base + PLL_FREQ, 0, 0);

	clks[LS1B_CLK_CPU_DIV] = clk_divider("cpu_div", "pll", 0,
		base + PLL_DIV_PARAM, LS1B_CPU_DIV_SHIFT, LS1B_CPU_DIV_WIDTH, CLK_DIVIDER_ONE_BASED);
	clks[LS1B_CLK_CPU_MUX] = clk_mux("cpu_mux", 0, base + PLL_DIV_PARAM,
		8, 1, cpu_mux,  ARRAY_SIZE(cpu_mux), 0);

	clks[LS1B_CLK_DDR_DIV] = clk_divider("ddr_div", "pll", 0,
		base + PLL_DIV_PARAM, LS1B_DDR_DIV_SHIFT, LS1B_DDR_DIV_WIDTH, CLK_DIVIDER_ONE_BASED);
	clks[LS1B_CLK_DDR_MUX] = clk_mux("ddr_mux", 0, base + PLL_DIV_PARAM,
		10, 1, ddr_mux,  ARRAY_SIZE(ddr_mux), 0);
	clks[LS1B_CLK_APB_DIV] = clk_fixed_factor("apb_div", "ddr_mux", LS1B_CLK_APB_MULT, LS1B_CLK_APB_DIV2, 0);

	clks[LS1B_CLK_DIV4] = clk_fixed_factor("dc_div4", "pll", 1, 4, 0);

	clks[LS1B_CLK_DC_DIV] = clk_divider("dc_div", "dc_div4", 0,
		base + PLL_DIV_PARAM, LS1B_DC_DIV_SHIFT, LS1B_DC_DIV_WIDTH, CLK_DIVIDER_ONE_BASED);
	clks[LS1B_CLK_DC_MUX] = clk_mux("dc_mux", 0, base + PLL_DIV_PARAM,
		10, 1, dc_mux,  ARRAY_SIZE(dc_mux), 0);
}

static int ls1b200_clk_probe(struct device *dev)
{
	struct resource *iores;
	void __iomem *base;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	/* now got the controller base address */
	ls1b200_pll_init(base);

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &clk_data);

	return 0;
}

static __maybe_unused struct of_device_id ls1b200_clk_dt_ids[] = {
	{
		.compatible = "loongson,ls1b-pll",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, ls1b200_clk_dt_ids);

static struct driver ls1b200_clk_driver = {
	.probe	= ls1b200_clk_probe,
	.name	= "ls1b-clk",
	.of_compatible = DRV_OF_COMPAT(ls1b200_clk_dt_ids),
};

postcore_platform_driver(ls1b200_clk_driver);

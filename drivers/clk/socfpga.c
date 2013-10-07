/*
 *  Copyright (C) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <malloc.h>
#include <linux/clkdev.h>
#include <linux/err.h>

/* Clock Manager offsets */
#define CLKMGR_CTRL	0x0
#define CLKMGR_BYPASS	0x4
#define CLKMGR_L4SRC	0x70
#define CLKMGR_PERPLL_SRC	0xAC

/* Clock bypass bits */
#define MAINPLL_BYPASS		(1<<0)
#define SDRAMPLL_BYPASS		(1<<1)
#define SDRAMPLL_SRC_BYPASS	(1<<2)
#define PERPLL_BYPASS		(1<<3)
#define PERPLL_SRC_BYPASS	(1<<4)

#define SOCFPGA_PLL_BG_PWRDWN		0
#define SOCFPGA_PLL_EXT_ENA		1
#define SOCFPGA_PLL_PWR_DOWN		2
#define SOCFPGA_PLL_DIVF_MASK		0x0000FFF8
#define SOCFPGA_PLL_DIVF_SHIFT	3
#define SOCFPGA_PLL_DIVQ_MASK		0x003F0000
#define SOCFPGA_PLL_DIVQ_SHIFT	16
#define SOCFGPA_MAX_PARENTS	3

#define SOCFPGA_DB_CLK			"gpio_db_clk"

#define div_mask(width)	((1 << (width)) - 1)
#define streq(a, b) (strcmp((a), (b)) == 0)

static void __iomem *clk_mgr_base_addr;

char *of_clk_get_parent_name(struct device_node *np, unsigned int index)
{
	struct of_phandle_args clkspec;
	const char *clk_name;
	int rc;

	rc = of_parse_phandle_with_args(np, "clocks", "#clock-cells", index,
			&clkspec);
	if (rc)
		return NULL;

	if (of_property_read_string_index(clkspec.np, "clock-output-names",
				clkspec.args_count ? clkspec.args[0] : 0,
				&clk_name) < 0)
		clk_name = clkspec.np->name;

	return xstrdup(clk_name);
}

static struct clk *socfpga_fixed_clk(struct device_node *node)
{
	uint32_t f = 0;

	of_property_read_u32(node, "clock-frequency", &f);

	return clk_fixed(node->name, f);
}

struct clk_pll {
	struct clk clk;
	const char *parent;
	unsigned regofs;
};

static unsigned long clk_pll_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_pll *pll = container_of(clk, struct clk_pll, clk);
	unsigned long divf, divq, vco_freq, reg;
	unsigned long bypass;

	reg = readl(clk_mgr_base_addr + pll->regofs);

	bypass = readl(clk_mgr_base_addr + CLKMGR_BYPASS);
	if (bypass & MAINPLL_BYPASS)
		return parent_rate;

	divf = (reg & SOCFPGA_PLL_DIVF_MASK) >> SOCFPGA_PLL_DIVF_SHIFT;
	divq = (reg & SOCFPGA_PLL_DIVQ_MASK) >> SOCFPGA_PLL_DIVQ_SHIFT;
	vco_freq = parent_rate * (divf + 1);

	return vco_freq / (1 + divq);
}

static struct clk_ops clk_pll_ops = {
	.recalc_rate = clk_pll_recalc_rate,
};

static struct clk *socfpga_pll_clk(struct device_node *node)
{
	struct clk_pll *pll;
	int ret;

	pll = xzalloc(sizeof(*pll));

	pll->parent = of_clk_get_parent_name(node, 0);
	if (!pll->parent)
		return ERR_PTR(-EINVAL);

	pll->clk.parent_names = &pll->parent;
	pll->clk.num_parents = 1;
	pll->clk.name = xstrdup(node->name);
	pll->clk.ops = &clk_pll_ops;

	of_property_read_u32(node, "reg", &pll->regofs);

	ret = clk_register(&pll->clk);
	if (ret) {
		free(pll);
		return ERR_PTR(ret);
	}

	return &pll->clk;
}

struct clk_periph {
	struct clk clk;
	const char *parent;
	unsigned regofs;
	unsigned int fixed_div;
};

static unsigned long clk_periph_recalc_rate(struct clk *clk,
		unsigned long parent_rate)
{
	struct clk_periph *periph = container_of(clk, struct clk_periph, clk);
	u32 div;

	if (periph->fixed_div)
		div = periph->fixed_div;
	else
		div = ((readl(clk_mgr_base_addr + periph->regofs) & 0x1ff) + 1);

	return parent_rate / div;
}

static struct clk_ops clk_periph_ops = {
	.recalc_rate = clk_periph_recalc_rate,
};

static struct clk *socfpga_periph_clk(struct device_node *node)
{
	struct clk_periph *periph;
	int ret;

	periph = xzalloc(sizeof(*periph));

	periph->parent = of_clk_get_parent_name(node, 0);
	if (!periph->parent)
		return ERR_PTR(-EINVAL);

	periph->clk.parent_names = &periph->parent;
	periph->clk.num_parents = 1;
	periph->clk.name = xstrdup(node->name);
	periph->clk.ops = &clk_periph_ops;

	of_property_read_u32(node, "reg", &periph->regofs);
	of_property_read_u32(node, "fixed-divider", &periph->fixed_div);

	ret = clk_register(&periph->clk);
	if (ret) {
		free(periph);
		return ERR_PTR(ret);
	}

	return &periph->clk;
}

struct clk_socfpga {
	struct clk clk;
	const char *parent;
	void __iomem *reg;
	void __iomem *div_reg;
	void __iomem *parent_reg;
	unsigned int fixed_div;
	unsigned int bit_idx;
	unsigned int shift;
	unsigned int width;
	unsigned int parent_shift;
	unsigned int parent_width;
	const char *parent_names[SOCFGPA_MAX_PARENTS];
};

static int clk_socfpga_enable(struct clk *clk)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);
	u32 val;

	val = readl(cs->reg);
	val |= 1 << cs->shift;
	writel(val, cs->reg);

	return 0;
}

static void clk_socfpga_disable(struct clk *clk)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);
	u32 val;

	val = readl(cs->reg);
	val &= ~(1 << cs->shift);
	writel(val, cs->reg);
}

static int clk_socfpga_is_enabled(struct clk *clk)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);
	u32 val;

	val = readl(cs->reg);

	if (val & (1 << cs->shift))
		return 1;
	else
		return 0;
}

static unsigned long clk_socfpga_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);
	u32 div = 1, val;

	if (cs->fixed_div) {
		div = cs->fixed_div;
	} else if (cs->div_reg) {
		val = readl(cs->div_reg) >> cs->shift;
		val &= div_mask(cs->width);
		if (streq(clk->name, SOCFPGA_DB_CLK))
			div = val + 1;
		else
			div = (1 << val);
	}

	return parent_rate / div;
}

static int clk_socfpga_get_parent(struct clk *clk)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);

	return readl(cs->parent_reg) >> cs->parent_shift &
		((1 << cs->parent_width) - 1);
}

static int clk_socfpga_set_parent(struct clk *clk, u8 parent)
{
	struct clk_socfpga *cs = container_of(clk, struct clk_socfpga, clk);
	uint32_t val;

	val = readl(cs->parent_reg);
	val &= ~(((1 << cs->parent_width) - 1) << cs->parent_shift);
	val |= parent << cs->parent_shift;
	writel(val, cs->parent_reg);

	return 0;
}

static struct clk_ops clk_socfpga_ops = {
	.recalc_rate = clk_socfpga_recalc_rate,
	.enable = clk_socfpga_enable,
	.disable = clk_socfpga_disable,
	.is_enabled = clk_socfpga_is_enabled,
	.get_parent = clk_socfpga_get_parent,
	.set_parent = clk_socfpga_set_parent,
};

static struct clk *socfpga_gate_clk(struct device_node *node)
{
	u32 clk_gate[2];
	u32 div_reg[3];
	u32 parent_reg[3];
	u32 fixed_div;
	struct clk_socfpga *cs;
	int ret;
	int i = 0;

	cs = xzalloc(sizeof(*cs));

	ret = of_property_read_u32_array(node, "clk-gate", clk_gate, 2);
	if (ret)
		clk_gate[0] = 0;

	if (clk_gate[0]) {
		cs->reg = clk_mgr_base_addr + clk_gate[0];
		cs->bit_idx = clk_gate[1];
	}

	ret = of_property_read_u32(node, "fixed-divider", &fixed_div);
	if (ret)
		cs->fixed_div = 0;
	else
		cs->fixed_div = fixed_div;

	ret = of_property_read_u32_array(node, "div-reg", div_reg, 3);
	if (!ret) {
		cs->div_reg = clk_mgr_base_addr + div_reg[0];
		cs->shift = div_reg[1];
		cs->width = div_reg[2];
	}

	ret = of_property_read_u32_array(node, "parent-reg", parent_reg, 3);
	if (!ret) {
		cs->parent_reg = clk_mgr_base_addr + parent_reg[0];
		cs->parent_shift = parent_reg[1];
		cs->parent_width = parent_reg[2];
	}

	for (i = 0; i < SOCFGPA_MAX_PARENTS; i++) {
		cs->parent_names[i] = of_clk_get_parent_name(node, i);
		if (!cs->parent_names[i])
			break;
	}

	cs->clk.parent_names = cs->parent_names;
	cs->clk.num_parents = i;
	cs->clk.name = xstrdup(node->name);
	cs->clk.ops = &clk_socfpga_ops;

	ret = clk_register(&cs->clk);
	if (ret) {
		free(cs);
		return ERR_PTR(ret);
	}

	return &cs->clk;
}

static void socfpga_register_clocks(struct device_d *dev, struct device_node *node)
{
	struct device_node *child;
	struct clk *clk;

	for_each_child_of_node(node, child) {
		socfpga_register_clocks(dev, child);
	}

	if (of_device_is_compatible(node, "fixed-clock"))
		clk = socfpga_fixed_clk(node);
	else if (of_device_is_compatible(node, "altr,socfpga-pll-clock"))
		clk = socfpga_pll_clk(node);
	else if (of_device_is_compatible(node, "altr,socfpga-perip-clk"))
		clk = socfpga_periph_clk(node);
	else if (of_device_is_compatible(node, "altr,socfpga-gate-clk"))
		clk = socfpga_gate_clk(node);
	else
		return;

	of_clk_add_provider(node, of_clk_src_simple_get, clk);
}

static int socfpga_ccm_probe(struct device_d *dev)
{
	void __iomem *regs;
	struct device_node *clknode;

	regs = dev_request_mem_region(dev, 0);
	if (!regs)
		return -EBUSY;

	clk_mgr_base_addr = regs;

	clknode = of_get_child_by_name(dev->device_node, "clocks");
	if (!clknode)
		return -EINVAL;

	socfpga_register_clocks(dev, clknode);

	return 0;
}

static __maybe_unused struct of_device_id socfpga_ccm_dt_ids[] = {
	{
		.compatible = "altr,clk-mgr",
	}, {
		/* sentinel */
	}
};

static struct driver_d socfpga_ccm_driver = {
	.probe	= socfpga_ccm_probe,
	.name	= "socfpga-ccm",
	.of_compatible = DRV_OF_COMPAT(socfpga_ccm_dt_ids),
};

static int socfpga_ccm_init(void)
{
	return platform_driver_register(&socfpga_ccm_driver);
}
core_initcall(socfpga_ccm_init);

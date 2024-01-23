// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * clock driver for Freescale QorIQ SoCs.
 */

#define pr_fmt(fmt) "clk-qoric: " fmt

#include <linux/clk.h>
#include <linux/clkdev.h>
#include <io.h>
#include <linux/kernel.h>
#include <of_address.h>
#include <of.h>
#include <linux/math64.h>

#define PLL_DIV1	0
#define PLL_DIV2	1
#define PLL_DIV3	2
#define PLL_DIV4	3

#define PLATFORM_PLL	0
#define CGA_PLL1	1
#define CGA_PLL2	2
#define CGA_PLL3	3
#define CGA_PLL4	4	/* only on clockgen-1.0, which lacks CGB */
#define CGB_PLL1	4
#define CGB_PLL2	5
#define MAX_PLL_DIV	32

struct clockgen_pll_div {
	struct clk_hw *hw;
	char name[32];
};

struct clockgen_pll {
	struct clockgen_pll_div div[MAX_PLL_DIV];
};

#define CLKSEL_VALID	1

struct clockgen_sourceinfo {
	u32 flags;	/* CLKSEL_xxx */
	int pll;	/* CGx_PLLn */
	int div;	/* PLL_DIVn */
};

#define NUM_MUX_PARENTS	16

struct clockgen_muxinfo {
	struct clockgen_sourceinfo clksel[NUM_MUX_PARENTS];
};

#define NUM_HWACCEL	5
#define NUM_CMUX	8

struct clockgen;

#define CG_PLL_8BIT		2	/* PLLCnGSR[CFG] is 8 bits, not 6 */
#define CG_VER3			4	/* version 3 cg: reg layout different */
#define CG_LITTLE_ENDIAN	8

struct clockgen_chipinfo {
	const char *compat;
	const struct clockgen_muxinfo *cmux_groups[2];
	const struct clockgen_muxinfo *hwaccel[NUM_HWACCEL];
	void (*init_periph)(struct clockgen *cg);
	int cmux_to_group[NUM_CMUX]; /* -1 terminates if fewer than NUM_CMUX */
	u32 pll_mask;	/* 1 << n bit set if PLL n is valid */
	u32 flags;	/* CG_xxx */
};

struct clockgen {
	struct device_node *node;
	void __iomem *regs;
	struct clockgen_chipinfo info; /* mutable copy */
	struct clk *sysclk, *coreclk;
	struct clockgen_pll pll[6];
	struct clk *cmux[NUM_CMUX];
	struct clk *hwaccel[NUM_HWACCEL];
	struct clk *fman[2];
};

static struct clockgen clockgen;

static void cg_out(struct clockgen *cg, u32 val, u32 __iomem *reg)
{
	if (cg->info.flags & CG_LITTLE_ENDIAN)
		iowrite32(val, reg);
	else
		iowrite32be(val, reg);
}

static u32 cg_in(struct clockgen *cg, u32 __iomem *reg)
{
	u32 val;

	if (cg->info.flags & CG_LITTLE_ENDIAN)
		val = ioread32(reg);
	else
		val = ioread32be(reg);

	return val;
}

static const struct clockgen_muxinfo t1023_cmux = {
	{
		[0] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		[1] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
	}
};

static const struct clockgen_muxinfo t1040_cmux = {
	{
		[0] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		[1] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		[4] = { CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		[5] = { CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
	}
};

static const struct clockgen_muxinfo clockgen2_cmux_cga12 = {
	{
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV4 },
	},
};

static const struct clockgen_muxinfo clockgen2_cmux_cgb = {
	{
		{ CLKSEL_VALID, CGB_PLL1, PLL_DIV1 },
		{ CLKSEL_VALID, CGB_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGB_PLL1, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGB_PLL2, PLL_DIV1 },
		{ CLKSEL_VALID, CGB_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGB_PLL2, PLL_DIV4 },
	},
};

static const struct clockgen_muxinfo ls1028a_hwa1 = {
	{
		{ CLKSEL_VALID, PLATFORM_PLL, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1028a_hwa2 = {
	{
		{ CLKSEL_VALID, PLATFORM_PLL, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1028a_hwa3 = {
	{
		{ CLKSEL_VALID, PLATFORM_PLL, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1028a_hwa4 = {
	{
		{ CLKSEL_VALID, PLATFORM_PLL, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV4 },
		{},
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1043a_hwa1 = {
	{
		{},
		{},
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
		{},
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1043a_hwa2 = {
	{
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1046a_hwa1 = {
	{
		{},
		{},
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV3 },
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV4 },
		{ CLKSEL_VALID, PLATFORM_PLL, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
	},
};

static const struct clockgen_muxinfo ls1046a_hwa2 = {
	{
		{},
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV1 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV2 },
		{ CLKSEL_VALID, CGA_PLL2, PLL_DIV3 },
		{},
		{},
		{ CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
	},
};

static const struct clockgen_muxinfo ls1012a_cmux = {
	{
		[0] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV1 },
		{},
		[2] = { CLKSEL_VALID, CGA_PLL1, PLL_DIV2 },
	}
};

static void __init t2080_init_periph(struct clockgen *cg)
{
	cg->fman[0] = cg->hwaccel[0];
}

static const struct clockgen_chipinfo chipinfo_ls1012a = {
	.compat = "fsl,ls1012a-clockgen",
	.cmux_groups = { &ls1012a_cmux },
	.cmux_to_group = { 0, -1 },
	.pll_mask = 0x03,
};

static const struct clockgen_chipinfo chipinfo_ls1021a = {
	.compat = "fsl,ls1021a-clockgen",
	.cmux_groups = { &t1023_cmux },
	.cmux_to_group = { 0, -1 },
	.pll_mask = 0x03,
};

static const struct clockgen_chipinfo chipinfo_ls1028a = {
	.compat = "fsl,ls1028a-clockgen",
	.cmux_groups = { &clockgen2_cmux_cga12 },
	.hwaccel = { &ls1028a_hwa1, &ls1028a_hwa2, &ls1028a_hwa3, &ls1028a_hwa4 },
	.cmux_to_group = { 0, 0, 0, 0, -1 },
	.pll_mask = 0x07,
	.flags = CG_VER3 | CG_LITTLE_ENDIAN,
};

static const struct clockgen_chipinfo chipinfo_ls1043a = {
	.compat = "fsl,ls1043a-clockgen",
	.init_periph = t2080_init_periph,
	.cmux_groups = { &t1040_cmux },
	.hwaccel = { &ls1043a_hwa1, &ls1043a_hwa2 },
	.cmux_to_group = { 0, -1 },
	.pll_mask = 0x07,
	.flags = CG_PLL_8BIT,
};

static const struct clockgen_chipinfo chipinfo_ls1046a = {
	.compat = "fsl,ls1046a-clockgen",
	.init_periph = t2080_init_periph,
	.cmux_groups = { &t1040_cmux },
	.hwaccel = { &ls1046a_hwa1, &ls1046a_hwa2 },
	.cmux_to_group = { 0, -1 },
	.pll_mask = 0x07,
	.flags = CG_PLL_8BIT,
};

static const struct clockgen_chipinfo chipinfo_ls1088a = {
	.compat = "fsl,ls1088a-clockgen",
	.cmux_groups = { &clockgen2_cmux_cga12 },
	.cmux_to_group = { 0, 0, -1 },
	.pll_mask = 0x07,
	.flags = CG_VER3 | CG_LITTLE_ENDIAN,
};

static const struct clockgen_chipinfo chipinfo_ls2080a = {
	.compat = "fsl,ls2080a-clockgen",
	.cmux_groups = { &clockgen2_cmux_cga12, &clockgen2_cmux_cgb },
	.cmux_to_group = { 0, 0, 1, 1, -1 },
	.pll_mask = 0x37,
	.flags = CG_VER3 | CG_LITTLE_ENDIAN,
};

struct mux_hwclock {
	struct clk_hw hw;
	struct clockgen *cg;
	const struct clockgen_muxinfo *info;
	u32 __iomem *reg;
	int num_parents;
};

#define to_mux_hwclock(p)	container_of(p, struct mux_hwclock, hw)
#define CLKSEL_MASK		0x78000000
#define	CLKSEL_SHIFT		27

static int mux_set_parent(struct clk_hw *hw, u8 idx)
{
	struct mux_hwclock *hwc = to_mux_hwclock(hw);

	if (idx >= hwc->num_parents)
		return -EINVAL;

	cg_out(hwc->cg, (idx << CLKSEL_SHIFT) & CLKSEL_MASK, hwc->reg);

	return 0;
}

static int mux_get_parent(struct clk_hw *hw)
{
	struct mux_hwclock *hwc = to_mux_hwclock(hw);

	return (cg_in(hwc->cg, hwc->reg) & CLKSEL_MASK) >> CLKSEL_SHIFT;
}

static const struct clk_ops cmux_ops = {
	.get_parent = mux_get_parent,
	.set_parent = mux_set_parent,
};

/*
 * Don't allow setting for now, as the clock options haven't been
 * sanitized for additional restrictions.
 */
static const struct clk_ops hwaccel_ops = {
	.get_parent = mux_get_parent,
};

static const struct clockgen_pll_div *get_pll_div(struct clockgen *cg,
						  struct mux_hwclock *hwc,
						  int idx)
{
	const struct clockgen_sourceinfo *clksel = &hwc->info->clksel[idx];
	int pll, div;

	if (!(clksel->flags & CLKSEL_VALID))
		return NULL;

	pll = clksel->pll;
	div = clksel->div;

	return &cg->pll[pll].div[div];
}

static struct clk * __init create_mux_common(struct clockgen *cg,
					     struct mux_hwclock *hwc,
					     const struct clk_ops *ops,
					     const char *fmt, int idx)
{
	struct clk_hw *hw = &hwc->hw;
	const struct clockgen_pll_div *div;
	const char **parent_names;
	int i, ret;

	parent_names = xzalloc(sizeof(char *) * NUM_MUX_PARENTS);

	for (i = 0; i < NUM_MUX_PARENTS; i++) {
		div = get_pll_div(cg, hwc, i);
		if (!div)
			continue;

		parent_names[i] = div->name;
	}

	hw->clk.name = xasprintf(fmt, idx);;
	hw->clk.ops = ops;
	hw->clk.parent_names = parent_names;
	hw->clk.num_parents = hwc->num_parents = i;
	hwc->cg = cg;

	ret = bclk_register(&hw->clk);
	if (ret) {
		pr_err("%s: Couldn't register %s: %d\n", __func__, clk_hw_get_name(hw), ret);
		kfree(hwc);
		return NULL;
	}

	return &hw->clk;
}

static struct clk * __init create_one_cmux(struct clockgen *cg, int idx)
{
	struct mux_hwclock *hwc;
	const struct clockgen_pll_div *div;
	u32 clksel;

	hwc = xzalloc(sizeof(*hwc));

	if (cg->info.flags & CG_VER3)
		hwc->reg = cg->regs + 0x70000 + 0x20 * idx;
	else
		hwc->reg = cg->regs + 0x20 * idx;

	hwc->info = cg->info.cmux_groups[cg->info.cmux_to_group[idx]];

	/*
	 * Find the rate for the default clksel, and treat it as the
	 * maximum rated core frequency.  If this is an incorrect
	 * assumption, certain clock options (possibly including the
	 * default clksel) may be inappropriately excluded on certain
	 * chips.
	 */
	clksel = (cg_in(cg, hwc->reg) & CLKSEL_MASK) >> CLKSEL_SHIFT;
	div = get_pll_div(cg, hwc, clksel);
	if (!div) {
		kfree(hwc);
		return NULL;
	}

	return create_mux_common(cg, hwc, &cmux_ops, "cg-cmux%d", idx);
}

static struct clk * __init create_one_hwaccel(struct clockgen *cg, int idx)
{
	struct mux_hwclock *hwc;

	hwc = xzalloc(sizeof(*hwc));

	hwc->reg = cg->regs + 0x20 * idx + 0x10;
	hwc->info = cg->info.hwaccel[idx];

	return create_mux_common(cg, hwc, &hwaccel_ops, "cg-hwaccel%d", idx);
}

static void __init create_muxes(struct clockgen *cg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cg->cmux); i++) {
		if (cg->info.cmux_to_group[i] < 0)
			break;
		if (cg->info.cmux_to_group[i] >=
		    ARRAY_SIZE(cg->info.cmux_groups)) {
			continue;
		}

		cg->cmux[i] = create_one_cmux(cg, i);
	}

	for (i = 0; i < ARRAY_SIZE(cg->hwaccel); i++) {
		if (!cg->info.hwaccel[i])
			continue;

		cg->hwaccel[i] = create_one_hwaccel(cg, i);
	}
}

#define PLL_KILL BIT(31)

static void __init create_one_pll(struct clockgen *cg, int idx)
{
	u32 __iomem *reg;
	u32 mult;
	struct clockgen_pll *pll = &cg->pll[idx];
	const char *input = cg->sysclk->name;
	int i;

	if (!(cg->info.pll_mask & (1 << idx)))
		return;

	if (cg->coreclk && idx != PLATFORM_PLL) {
		if (IS_ERR(cg->coreclk))
			return;

		input = cg->coreclk->name;
	}

	if (cg->info.flags & CG_VER3) {
		switch (idx) {
		case PLATFORM_PLL:
			reg = cg->regs + 0x60080;
			break;
		case CGA_PLL1:
			reg = cg->regs + 0x80;
			break;
		case CGA_PLL2:
			reg = cg->regs + 0xa0;
			break;
		case CGB_PLL1:
			reg = cg->regs + 0x10080;
			break;
		case CGB_PLL2:
			reg = cg->regs + 0x100a0;
			break;
		default:
			pr_warn("index %d\n", idx);
			return;
		}
	} else {
		if (idx == PLATFORM_PLL)
			reg = cg->regs + 0xc00;
		else
			reg = cg->regs + 0x800 + 0x20 * (idx - 1);
	}

	/* Get the multiple of PLL */
	mult = cg_in(cg, reg);

	/* Check if this PLL is disabled */
	if (mult & PLL_KILL) {
		pr_debug("%s(): pll %p disabled\n", __func__, reg);
		return;
	}

	if ((cg->info.flags & CG_VER3) ||
	    ((cg->info.flags & CG_PLL_8BIT) && idx != PLATFORM_PLL))
		mult = (mult & GENMASK(8, 1)) >> 1;
	else
		mult = (mult & GENMASK(6, 1)) >> 1;

	for (i = 0; i < ARRAY_SIZE(pll->div); i++) {
		struct clk *clk;

		/*
		 * For platform PLL, there are 8 divider clocks.
		 * For core PLL, there are 4 divider clocks at most.
		 */
		if (idx != PLATFORM_PLL && i >= 4)
			break;

		snprintf(pll->div[i].name, sizeof(pll->div[i].name),
			 "cg-pll%d-div%d", idx, i + 1);

		clk = clk_fixed_factor(pll->div[i].name, input, mult, i + 1, 0);
		if (IS_ERR(clk)) {
			pr_err("%s: %s: register failed %ld\n",
			       __func__, pll->div[i].name, PTR_ERR(clk));
			continue;
		}

		pll->div[i].hw = clk_to_clk_hw(clk);
	}
}

static void __init create_plls(struct clockgen *cg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cg->pll); i++)
		create_one_pll(cg, i);
}

static struct clk *clockgen_clk_get(struct of_phandle_args *clkspec, void *data)
{
	struct clockgen *cg = data;
	struct clk *clk;
	struct clockgen_pll *pll;
	u32 type, idx;

	if (clkspec->args_count < 2) {
		pr_err("%s: insufficient phandle args\n", __func__);
		return ERR_PTR(-EINVAL);
	}

	type = clkspec->args[0];
	idx = clkspec->args[1];

	switch (type) {
	case 0:
		if (idx != 0)
			goto bad_args;
		clk = cg->sysclk;
		break;
	case 1:
		if (idx >= ARRAY_SIZE(cg->cmux))
			goto bad_args;
		clk = cg->cmux[idx];
		break;
	case 2:
		if (idx >= ARRAY_SIZE(cg->hwaccel))
			goto bad_args;
		clk = cg->hwaccel[idx];
		break;
	case 3:
		if (idx >= ARRAY_SIZE(cg->fman))
			goto bad_args;
		clk = cg->fman[idx];
		break;
	case 4:
		pll = &cg->pll[PLATFORM_PLL];
		if (idx >= ARRAY_SIZE(pll->div))
			goto bad_args;
		clk = clk_hw_to_clk(pll->div[idx].hw);
		break;
	case 5:
		if (idx != 0)
			goto bad_args;
		clk = cg->coreclk;
		if (IS_ERR(clk))
			clk = NULL;
		break;
	default:
		goto bad_args;
	}

	if (!clk)
		return ERR_PTR(-ENOENT);
	return clk;

bad_args:
	pr_err("%s: Bad phandle args %u %u\n", __func__, type, idx);
	return ERR_PTR(-EINVAL);
}

static void __init clockgen_init(struct device_node *np,
				 const struct clockgen_chipinfo *chipinfo)
{
	int ret;

	clockgen.node = np;
	clockgen.regs = of_iomap(np, 0);
	if (!clockgen.regs) {
		pr_err("of_iomap failed for %pOF\n", np);
		return;
	}

	clockgen.info = *chipinfo;

	clockgen.sysclk = of_clk_get(clockgen.node, 0);
	if (IS_ERR(clockgen.sysclk)) {
		pr_err("sysclk not found: %pe\n", clockgen.sysclk);
		return;
	}

	clockgen.coreclk = of_clk_get(clockgen.node, 1);
	if (IS_ERR(clockgen.coreclk))
		clockgen.coreclk = NULL;

	create_plls(&clockgen);
	create_muxes(&clockgen);

	if (clockgen.info.init_periph)
		clockgen.info.init_periph(&clockgen);

	ret = of_clk_add_provider(np, clockgen_clk_get, &clockgen);
	if (ret) {
		pr_err("Couldn't register clk provider for node %pOF: %d\n",
		       np, ret);
	}

	return;
}

static void __maybe_unused clockgen_init_ls1012a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1012a);
}

static void __maybe_unused clockgen_init_ls1021a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1021a);
}

static void __maybe_unused clockgen_init_ls1028a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1028a);
}

static void __maybe_unused clockgen_init_ls1043a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1043a);
}

static void __maybe_unused clockgen_init_ls1046a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1046a);
}

static void __maybe_unused clockgen_init_ls1088a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls1088a);
}

static void __maybe_unused clockgen_init_ls2080a(struct device_node *np)
{
	clockgen_init(np, &chipinfo_ls2080a);
}

#ifdef CONFIG_ARCH_LS1012
CLK_OF_DECLARE(qoriq_clockgen_ls1012a, "fsl,ls1012a-clockgen", clockgen_init_ls1012a);
#endif
#ifdef CONFIG_ARCH_LS1021
CLK_OF_DECLARE(qoriq_clockgen_ls1021a, "fsl,ls1021a-clockgen", clockgen_init_ls1021a);
#endif
#ifdef CONFIG_ARCH_LS1028
CLK_OF_DECLARE(qoriq_clockgen_ls1021a, "fsl,ls1028a-clockgen", clockgen_init_ls1028a);
#endif
#ifdef CONFIG_ARCH_LS1043
CLK_OF_DECLARE(qoriq_clockgen_ls1043a, "fsl,ls1043a-clockgen", clockgen_init_ls1043a);
#endif
#ifdef CONFIG_ARCH_LS1046
CLK_OF_DECLARE(qoriq_clockgen_ls1046a, "fsl,ls1046a-clockgen", clockgen_init_ls1046a);
#endif
#ifdef CONFIG_ARCH_LS1088
CLK_OF_DECLARE(qoriq_clockgen_ls1088a, "fsl,ls1088a-clockgen", clockgen_init_ls1088a);
#endif
#ifdef CONFIG_ARCH_LS2080
CLK_OF_DECLARE(qoriq_clockgen_ls2080a, "fsl,ls2080a-clockgen", clockgen_init_ls2080a);
#endif

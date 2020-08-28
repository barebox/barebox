// SPDX-License-Identifier: GPL-2.0-only
/*
 * MVEBU Core divider clock
 *
 * Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * Based on Linux driver
 *   Copyright (C) 2013 Marvell
 *   Ezequiel Garcia <ezequiel.garcia@free-electrons.com>
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clkdev.h>

/*
 * This structure describes the hardware details (bit offset and mask)
 * to configure one particular core divider clock. Those hardware
 * details may differ from one SoC to another. This structure is
 * therefore typically instantiated statically to describe the
 * hardware details.
 */
struct clk_corediv_desc {
	unsigned int mask;
	unsigned int offset;
	unsigned int fieldbit;
};

/*
 * This structure describes the hardware details to configure the core
 * divider clocks on a given SoC. Amongst others, it points to the
 * array of core divider clock descriptors for this SoC, as well as
 * the corresponding operations to manipulate them.
 */
struct clk_corediv_soc_desc {
	const struct clk_corediv_desc *descs;
	unsigned int ndescs;
	const struct clk_ops ops;
	u32 ratio_reload;
	u32 enable_bit_offset;
	u32 ratio_offset;
};

/*
 * This structure represents one core divider clock for the clock
 * framework, and is dynamically allocated for each core divider clock
 * existing in the current SoC.
 */
struct clk_corediv {
	struct clk clk;
	void __iomem *reg;
	const struct clk_corediv_desc *desc;
	const struct clk_corediv_soc_desc *soc_desc;
};

static struct clk_onecell_data clk_data;

/*
 * Description of the core divider clocks available. For now, we
 * support only NAND, and it is available at the same register
 * locations regardless of the SoC.
 */
static const struct clk_corediv_desc mvebu_corediv_desc[] = {
	{ .mask = 0x3f, .offset = 8, .fieldbit = 1 }, /* NAND clock */
};

#define CORE_CLK_DIV_RATIO_MASK	0xff

#define to_corediv_clk(p) container_of(p, struct clk_corediv, clk)

static int clk_corediv_is_enabled(struct clk *clk)
{
	struct clk_corediv *corediv = to_corediv_clk(clk);
	const struct clk_corediv_soc_desc *soc_desc = corediv->soc_desc;
	const struct clk_corediv_desc *desc = corediv->desc;
	u32 enable_mask = BIT(desc->fieldbit) << soc_desc->enable_bit_offset;

	return !!(readl(corediv->reg) & enable_mask);
}

static int clk_corediv_enable(struct clk *clk)
{
	struct clk_corediv *corediv = to_corediv_clk(clk);
	const struct clk_corediv_soc_desc *soc_desc = corediv->soc_desc;
	const struct clk_corediv_desc *desc = corediv->desc;
	u32 reg;

	reg = readl(corediv->reg);
	reg |= (BIT(desc->fieldbit) << soc_desc->enable_bit_offset);
	writel(reg, corediv->reg);

	return 0;
}

static void clk_corediv_disable(struct clk *clk)
{
	struct clk_corediv *corediv = to_corediv_clk(clk);
	const struct clk_corediv_soc_desc *soc_desc = corediv->soc_desc;
	const struct clk_corediv_desc *desc = corediv->desc;
	u32 reg;

	reg = readl(corediv->reg);
	reg &= ~(BIT(desc->fieldbit) << soc_desc->enable_bit_offset);
	writel(reg, corediv->reg);
}

static unsigned long clk_corediv_recalc_rate(struct clk *clk,
					     unsigned long parent_rate)
{
	struct clk_corediv *corediv = to_corediv_clk(clk);
	const struct clk_corediv_soc_desc *soc_desc = corediv->soc_desc;
	const struct clk_corediv_desc *desc = corediv->desc;
	u32 reg, div;

	reg = readl(corediv->reg + soc_desc->ratio_offset);
	div = (reg >> desc->offset) & desc->mask;
	return parent_rate / div;
}

static long clk_corediv_round_rate(struct clk *clk, unsigned long rate,
				   unsigned long *parent_rate)
{
	/* Valid ratio are 1:4, 1:5, 1:6 and 1:8 */
	u32 div;

	div = *parent_rate / rate;
	if (div < 4)
		div = 4;
	else if (div > 6)
		div = 8;

	return *parent_rate / div;
}

static int clk_corediv_set_rate(struct clk *clk, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk_corediv *corediv = to_corediv_clk(clk);
	const struct clk_corediv_soc_desc *soc_desc = corediv->soc_desc;
	const struct clk_corediv_desc *desc = corediv->desc;
	u32 reg, div;

	div = parent_rate / rate;

	/* Write new divider to the divider ratio register */
	reg = readl(corediv->reg + soc_desc->ratio_offset);
	reg &= ~(desc->mask << desc->offset);
	reg |= (div & desc->mask) << desc->offset;
	writel(reg, corediv->reg + soc_desc->ratio_offset);

	/* Set reload-force for this clock */
	reg = readl(corediv->reg) | BIT(desc->fieldbit);
	writel(reg, corediv->reg);

	/* Now trigger the clock update */
	reg = readl(corediv->reg) | soc_desc->ratio_reload;
	writel(reg, corediv->reg);

	/*
	 * Wait for clocks to settle down, and then clear all the
	 * ratios request and the reload request.
	 */
	udelay(1000);
	reg &= ~(CORE_CLK_DIV_RATIO_MASK | soc_desc->ratio_reload);
	writel(reg, corediv->reg);
	udelay(1000);

	return 0;
}

static const struct clk_corediv_soc_desc armada370_corediv_soc = {
	.descs = mvebu_corediv_desc,
	.ndescs = ARRAY_SIZE(mvebu_corediv_desc),
	.ops = {
		.enable = clk_corediv_enable,
		.disable = clk_corediv_disable,
		.is_enabled = clk_corediv_is_enabled,
		.recalc_rate = clk_corediv_recalc_rate,
		.round_rate = clk_corediv_round_rate,
		.set_rate = clk_corediv_set_rate,
	},
	.ratio_reload = BIT(8),
	.enable_bit_offset = 24,
	.ratio_offset = 0x8,
};

static struct of_device_id mvebu_corediv_clk_ids[] = {
	{ .compatible = "marvell,armada-370-corediv-clock",
	  .data = &armada370_corediv_soc },
	{ }
};

static int mvebu_corediv_clk_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	const struct of_device_id *match;
	const struct clk_corediv_soc_desc *soc_desc;
	struct clk_corediv *corediv;
	struct clk *parent;
	void __iomem *base;
	int n;

	match = of_match_node(mvebu_corediv_clk_ids, np);
	if (!match)
		return -EINVAL;
	soc_desc = (const struct clk_corediv_soc_desc *)match->data;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	parent = of_clk_get(np, 0);
	if (IS_ERR(parent))
		return PTR_ERR(parent);

	clk_data.clk_num = soc_desc->ndescs;
	clk_data.clks = xzalloc(clk_data.clk_num * sizeof(*clk_data.clks));
	corediv = xzalloc(clk_data.clk_num * sizeof(*corediv));

	for (n = 0; n < clk_data.clk_num; n++) {
		const char *clk_name;
		struct clk *clk = &corediv->clk;

		if (of_property_read_string_index(np,
				"clock-output-names", n, &clk_name)) {
			dev_warn(dev, "missing clock output name %d\n", n);
			continue;
		}

		clk->ops = &soc_desc->ops;
		clk->name = clk_name;
		clk->flags = 0;
		clk->parent_names = &parent->name;
		clk->num_parents = 1;
		corediv->soc_desc = soc_desc;
		corediv->desc = &soc_desc->descs[n];
		corediv->reg = base;
		clk_data.clks[n] = clk;
		WARN_ON(IS_ERR_VALUE(clk_register(clk)));
	}

	return of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}

static struct driver_d mvebu_corediv_clk_driver = {
	.probe	= mvebu_corediv_clk_probe,
	.name	= "mvebu-corediv-clk",
	.of_compatible = DRV_OF_COMPAT(mvebu_corediv_clk_ids),
};

postcore_platform_driver(mvebu_corediv_clk_driver);

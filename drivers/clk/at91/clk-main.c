// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */
#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include "pmc.h"

#define SLOW_CLOCK_FREQ		32768
#define MAINF_DIV		16
#define MAINFRDY_TIMEOUT	(((MAINF_DIV + 1) * SECOND) / \
				 SLOW_CLOCK_FREQ)
#define MAINF_LOOP_MIN_WAIT	(SECOND / SLOW_CLOCK_FREQ)
#define MAINF_LOOP_MAX_WAIT	MAINFRDY_TIMEOUT

#define MOR_KEY_MASK		(0xff << 16)

#define clk_main_parent_select(s)	(((s) & \
					(AT91_PMC_MOSCEN | \
					AT91_PMC_OSCBYPASS)) ? 1 : 0)

struct clk_main_osc {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent;
};

#define to_clk_main_osc(_hw) container_of(_hw, struct clk_main_osc, hw)

struct clk_main_rc_osc {
	struct clk_hw hw;
	struct regmap *regmap;
	unsigned long frequency;
};

#define to_clk_main_rc_osc(_hw) container_of(_hw, struct clk_main_rc_osc, hw)

struct clk_rm9200_main {
	struct clk_hw hw;
	struct regmap *regmap;
	const char *parent;
};

#define to_clk_rm9200_main(_hw) container_of(_hw, struct clk_rm9200_main, hw)

struct clk_sam9x5_main {
	struct clk_hw hw;
	struct regmap *regmap;
	u8 parent;
};

#define to_clk_sam9x5_main(_hw) container_of(_hw, struct clk_sam9x5_main, hw)

static inline bool clk_main_osc_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_MOSCS;
}

static int clk_main_osc_enable(struct clk_hw *hw)
{
	struct clk_main_osc *osc = to_clk_main_osc(hw);
	struct regmap *regmap = osc->regmap;
	u32 tmp;

	regmap_read(regmap, AT91_CKGR_MOR, &tmp);
	tmp &= ~MOR_KEY_MASK;

	if (tmp & AT91_PMC_OSCBYPASS)
		return 0;

	if (!(tmp & AT91_PMC_MOSCEN)) {
		tmp |= AT91_PMC_MOSCEN | AT91_PMC_KEY;
		regmap_write(regmap, AT91_CKGR_MOR, tmp);
	}

	while (!clk_main_osc_ready(regmap))
		barrier();

	return 0;
}

static void clk_main_osc_disable(struct clk_hw *hw)
{
	struct clk_main_osc *osc = to_clk_main_osc(hw);
	struct regmap *regmap = osc->regmap;
	u32 tmp;

	regmap_read(regmap, AT91_CKGR_MOR, &tmp);
	if (tmp & AT91_PMC_OSCBYPASS)
		return;

	if (!(tmp & AT91_PMC_MOSCEN))
		return;

	tmp &= ~(AT91_PMC_KEY | AT91_PMC_MOSCEN);
	regmap_write(regmap, AT91_CKGR_MOR, tmp | AT91_PMC_KEY);
}

static int clk_main_osc_is_enabled(struct clk_hw *hw)
{
	struct clk_main_osc *osc = to_clk_main_osc(hw);
	struct regmap *regmap = osc->regmap;
	u32 tmp, status;

	regmap_read(regmap, AT91_CKGR_MOR, &tmp);
	if (tmp & AT91_PMC_OSCBYPASS)
		return 1;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return (status & AT91_PMC_MOSCS) && clk_main_parent_select(tmp);
}

static const struct clk_ops main_osc_ops = {
	.enable = clk_main_osc_enable,
	.disable = clk_main_osc_disable,
	.is_enabled = clk_main_osc_is_enabled,
};

struct clk * __init
at91_clk_register_main_osc(struct regmap *regmap,
			   const char *name,
			   const char *parent_name,
			   bool bypass)
{
	struct clk_main_osc *osc;
	int ret;

	if (!name || !parent_name)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));

	osc->parent = parent_name;
	osc->hw.clk.name = name;
	osc->hw.clk.ops = &main_osc_ops;
	osc->hw.clk.parent_names = &osc->parent;
	osc->hw.clk.num_parents = 1;
	osc->regmap = regmap;

	if (bypass)
		regmap_write_bits(regmap,
				  AT91_CKGR_MOR, MOR_KEY_MASK |
				  AT91_PMC_MOSCEN,
				  AT91_PMC_OSCBYPASS | AT91_PMC_KEY);

	ret = bclk_register(&osc->hw.clk);
	if (ret) {
		free(osc);
		return ERR_PTR(ret);
	}

	return &osc->hw.clk;
}

static bool clk_main_rc_osc_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_MOSCRCS;
}

static int clk_main_rc_osc_enable(struct clk_hw *hw)
{
	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(hw);
	struct regmap *regmap = osc->regmap;
	unsigned int mor;

	regmap_read(regmap, AT91_CKGR_MOR, &mor);

	if (!(mor & AT91_PMC_MOSCRCEN))
		regmap_write_bits(regmap, AT91_CKGR_MOR,
				  MOR_KEY_MASK | AT91_PMC_MOSCRCEN,
				  AT91_PMC_MOSCRCEN | AT91_PMC_KEY);

	while (!clk_main_rc_osc_ready(regmap))
		barrier();

	return 0;
}

static void clk_main_rc_osc_disable(struct clk_hw *hw)
{
	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(hw);
	struct regmap *regmap = osc->regmap;
	unsigned int mor;

	regmap_read(regmap, AT91_CKGR_MOR, &mor);

	if (!(mor & AT91_PMC_MOSCRCEN))
		return;

	regmap_write_bits(regmap, AT91_CKGR_MOR,
			  MOR_KEY_MASK | AT91_PMC_MOSCRCEN, AT91_PMC_KEY);
}

static int clk_main_rc_osc_is_enabled(struct clk_hw *hw)
{
	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(hw);
	struct regmap *regmap = osc->regmap;
	unsigned int mor, status;

	regmap_read(regmap, AT91_CKGR_MOR, &mor);
	regmap_read(regmap, AT91_PMC_SR, &status);

	return (mor & AT91_PMC_MOSCRCEN) && (status & AT91_PMC_MOSCRCS);
}

static unsigned long clk_main_rc_osc_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_main_rc_osc *osc = to_clk_main_rc_osc(hw);

	return osc->frequency;
}

static const struct clk_ops main_rc_osc_ops = {
	.enable = clk_main_rc_osc_enable,
	.disable = clk_main_rc_osc_disable,
	.is_enabled = clk_main_rc_osc_is_enabled,
	.recalc_rate = clk_main_rc_osc_recalc_rate,
};

struct clk * __init
at91_clk_register_main_rc_osc(struct regmap *regmap,
			      const char *name,
			      u32 frequency, u32 accuracy)
{
	int ret;
	struct clk_main_rc_osc *osc;

	if (!name || !frequency)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));

	osc->hw.clk.name = name;
	osc->hw.clk.ops = &main_rc_osc_ops;
	osc->hw.clk.parent_names = NULL;
	osc->hw.clk.num_parents = 0;

	osc->regmap = regmap;
	osc->frequency = frequency;

	ret = bclk_register(&osc->hw.clk);
	if (ret) {
		kfree(osc);
		return ERR_PTR(ret);
	}

	return &osc->hw.clk;
}

static int clk_main_probe_frequency(struct regmap *regmap)
{
	unsigned int mcfr;
	uint64_t start = get_time_ns();

	do {
		regmap_read(regmap, AT91_CKGR_MCFR, &mcfr);
		if (mcfr & AT91_PMC_MAINRDY)
			return 0;
	} while (!is_timeout(start, MAINFRDY_TIMEOUT *  USECOND));

	return -ETIMEDOUT;
}

static unsigned long clk_main_recalc_rate(struct regmap *regmap,
					  unsigned long parent_rate)
{
	unsigned int mcfr;

	if (parent_rate)
		return parent_rate;

	pr_warn("Main crystal frequency not set, using approximate value\n");
	regmap_read(regmap, AT91_CKGR_MCFR, &mcfr);
	if (!(mcfr & AT91_PMC_MAINRDY))
		return 0;

	return ((mcfr & AT91_PMC_MAINF) * SLOW_CLOCK_FREQ) / MAINF_DIV;
}

static int clk_rm9200_main_enable(struct clk_hw *hw)
{
	struct clk_rm9200_main *clkmain = to_clk_rm9200_main(hw);

	return clk_main_probe_frequency(clkmain->regmap);
}

static int clk_rm9200_main_is_enabled(struct clk_hw *hw)
{
	struct clk_rm9200_main *clkmain = to_clk_rm9200_main(hw);
	unsigned int status;

	regmap_read(clkmain->regmap, AT91_CKGR_MCFR, &status);

	return status & AT91_PMC_MAINRDY ? 1 : 0;
}

static unsigned long clk_rm9200_main_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_rm9200_main *clkmain = to_clk_rm9200_main(hw);

	return clk_main_recalc_rate(clkmain->regmap, parent_rate);
}

static const struct clk_ops rm9200_main_ops = {
	.enable = clk_rm9200_main_enable,
	.is_enabled = clk_rm9200_main_is_enabled,
	.recalc_rate = clk_rm9200_main_recalc_rate,
};

struct clk * __init
at91_clk_register_rm9200_main(struct regmap *regmap,
			      const char *name,
			      const char *parent_name)
{
	int ret;
	struct clk_rm9200_main *clkmain;

	if (!name)
		return ERR_PTR(-EINVAL);

	if (!parent_name)
		return ERR_PTR(-EINVAL);

	clkmain = xzalloc(sizeof(*clkmain));

	clkmain->parent = parent_name;
	clkmain->hw.clk.name = name;
	clkmain->hw.clk.ops = &rm9200_main_ops;
	clkmain->hw.clk.parent_names = &clkmain->parent;
	clkmain->hw.clk.num_parents = 1;
	clkmain->regmap = regmap;

	ret = bclk_register(&clkmain->hw.clk);
	if (ret) {
		kfree(clkmain);
		return ERR_PTR(ret);
	}

	return &clkmain->hw.clk;
}

static inline bool clk_sam9x5_main_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_MOSCSELS ? 1 : 0;
}

static int clk_sam9x5_main_enable(struct clk_hw *hw)
{
	struct clk_sam9x5_main *clkmain = to_clk_sam9x5_main(hw);
	struct regmap *regmap = clkmain->regmap;

	while (!clk_sam9x5_main_ready(regmap))
		barrier();

	return clk_main_probe_frequency(regmap);
}

static int clk_sam9x5_main_is_enabled(struct clk_hw *hw)
{
	struct clk_sam9x5_main *clkmain = to_clk_sam9x5_main(hw);

	return clk_sam9x5_main_ready(clkmain->regmap);
}

static unsigned long clk_sam9x5_main_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_sam9x5_main *clkmain = to_clk_sam9x5_main(hw);

	return clk_main_recalc_rate(clkmain->regmap, parent_rate);
}

static int clk_sam9x5_main_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_sam9x5_main *clkmain = to_clk_sam9x5_main(hw);
	struct regmap *regmap = clkmain->regmap;
	unsigned int tmp;

	if (index > 1)
		return -EINVAL;

	regmap_read(regmap, AT91_CKGR_MOR, &tmp);
	tmp &= ~MOR_KEY_MASK;

	if (index && !(tmp & AT91_PMC_MOSCSEL))
		regmap_write(regmap, AT91_CKGR_MOR, tmp | AT91_PMC_MOSCSEL);
	else if (!index && (tmp & AT91_PMC_MOSCSEL))
		regmap_write(regmap, AT91_CKGR_MOR, tmp & ~AT91_PMC_MOSCSEL);

	while (!clk_sam9x5_main_ready(regmap))
		barrier();

	return 0;
}

static int clk_sam9x5_main_get_parent(struct clk_hw *hw)
{
	struct clk_sam9x5_main *clkmain = to_clk_sam9x5_main(hw);
	unsigned int status;

	regmap_read(clkmain->regmap, AT91_CKGR_MOR, &status);

	return clk_main_parent_select(status);
}

static const struct clk_ops sam9x5_main_ops = {
	.enable = clk_sam9x5_main_enable,
	.is_enabled = clk_sam9x5_main_is_enabled,
	.recalc_rate = clk_sam9x5_main_recalc_rate,
	.set_parent = clk_sam9x5_main_set_parent,
	.get_parent = clk_sam9x5_main_get_parent,
};

struct clk * __init
at91_clk_register_sam9x5_main(struct regmap *regmap,
			      const char *name,
			      const char **parent_names,
			      int num_parents)
{
	int ret;
	unsigned int status;
	size_t parents_array_size;
	struct clk_sam9x5_main *clkmain;

	if (!name)
		return ERR_PTR(-EINVAL);

	if (!parent_names || !num_parents)
		return ERR_PTR(-EINVAL);

	clkmain = xzalloc(sizeof(*clkmain));

	clkmain->hw.clk.name = name;
	clkmain->hw.clk.ops = &sam9x5_main_ops;
	parents_array_size = num_parents * sizeof (clkmain->hw.clk.parent_names[0]);
	clkmain->hw.clk.parent_names = xmemdup(parent_names, parents_array_size);
	clkmain->hw.clk.num_parents = num_parents;

	/* init.flags = CLK_SET_PARENT_GATE; */

	clkmain->regmap = regmap;
	regmap_read(clkmain->regmap, AT91_CKGR_MOR, &status);
	clkmain->parent = clk_main_parent_select(status);

	ret = bclk_register(&clkmain->hw.clk);
	if (ret) {
		kfree(clkmain);
		return ERR_PTR(ret);
	}

	return &clkmain->hw.clk;
}

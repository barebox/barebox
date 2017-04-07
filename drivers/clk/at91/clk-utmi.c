/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define UTMI_FIXED_MUL		40

struct clk_utmi {
	struct clk clk;
	struct regmap *regmap;
	const char *parent;
};

#define to_clk_utmi(clk) container_of(clk, struct clk_utmi, clk)

static inline bool clk_utmi_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_LOCKU;
}

static int clk_utmi_enable(struct clk *clk)
{
	struct clk_utmi *utmi = to_clk_utmi(clk);
	unsigned int uckr = AT91_PMC_UPLLEN | AT91_PMC_UPLLCOUNT |
			    AT91_PMC_BIASEN;

	regmap_write_bits(utmi->regmap, AT91_CKGR_UCKR, uckr, uckr);

	while (!clk_utmi_ready(utmi->regmap))
		barrier();

	return 0;
}

static int clk_utmi_is_enabled(struct clk *clk)
{
	struct clk_utmi *utmi = to_clk_utmi(clk);

	return clk_utmi_ready(utmi->regmap);
}

static void clk_utmi_disable(struct clk *clk)
{
	struct clk_utmi *utmi = to_clk_utmi(clk);

	regmap_write_bits(utmi->regmap, AT91_CKGR_UCKR, AT91_PMC_UPLLEN, 0);
}

static unsigned long clk_utmi_recalc_rate(struct clk *clk,
					  unsigned long parent_rate)
{
	/* UTMI clk is a fixed clk multiplier */
	return parent_rate * UTMI_FIXED_MUL;
}

static const struct clk_ops utmi_ops = {
	.enable = clk_utmi_enable,
	.disable = clk_utmi_disable,
	.is_enabled = clk_utmi_is_enabled,
	.recalc_rate = clk_utmi_recalc_rate,
};

static struct clk * __init
at91_clk_register_utmi(struct regmap *regmap,
		       const char *name, const char *parent_name)
{
	int ret;
	struct clk_utmi *utmi;

	utmi = xzalloc(sizeof(*utmi));

	utmi->clk.name = name;
	utmi->clk.ops = &utmi_ops;

	if (parent_name) {
		utmi->parent = parent_name;
		utmi->clk.parent_names = &utmi->parent;
		utmi->clk.num_parents = 1;
	}

	/* utmi->clk.flags = CLK_SET_RATE_GATE; */

	utmi->regmap = regmap;

	ret = clk_register(&utmi->clk);
	if (ret) {
		kfree(utmi);
		return ERR_PTR(ret);
	}

	return &utmi->clk;
}
#if defined(CONFIG_OFTREE) && defined(CONFIG_COMMON_CLK_OF_PROVIDER)
static void __init of_at91sam9x5_clk_utmi_setup(struct device_node *np)
{
	struct clk *clk;
	const char *parent_name;
	const char *name = np->name;
	struct regmap *regmap;

	parent_name = of_clk_get_parent_name(np, 0);

	of_property_read_string(np, "clock-output-names", &name);

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return;

	clk = at91_clk_register_utmi(regmap, name, parent_name);
	if (IS_ERR(clk))
		return;

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
	return;
}
CLK_OF_DECLARE(at91sam9x5_clk_utmi, "atmel,at91sam9x5-clk-utmi",
	       of_at91sam9x5_clk_utmi_setup);
#endif

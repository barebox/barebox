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
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define SYSTEM_MAX_ID		31

#define SYSTEM_MAX_NAME_SZ	32

#define to_clk_system(clk) container_of(clk, struct clk_system, clk)
struct clk_system {
	struct clk clk;
	struct regmap *regmap;
	u8 id;
	const char *parent_name;
};

static inline int is_pck(int id)
{
	return (id >= 8) && (id <= 15);
}

static inline bool clk_system_ready(struct regmap *regmap, int id)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & (1 << id) ? 1 : 0;
}

static int clk_system_enable(struct clk *clk)
{
	struct clk_system *sys = to_clk_system(clk);

	regmap_write(sys->regmap, AT91_PMC_SCER, 1 << sys->id);

	if (!is_pck(sys->id))
		return 0;

	while (!clk_system_ready(sys->regmap, sys->id))
		barrier();

	return 0;
}

static void clk_system_disable(struct clk *clk)
{
	struct clk_system *sys = to_clk_system(clk);

	regmap_write(sys->regmap, AT91_PMC_SCDR, 1 << sys->id);
}

static int clk_system_is_enabled(struct clk *clk)
{
	struct clk_system *sys = to_clk_system(clk);
	unsigned int status;

	regmap_read(sys->regmap, AT91_PMC_SCSR, &status);

	if (!(status & (1 << sys->id)))
		return 0;

	if (!is_pck(sys->id))
		return 1;

	regmap_read(sys->regmap, AT91_PMC_SR, &status);

	return status & (1 << sys->id) ? 1 : 0;
}

static const struct clk_ops system_ops = {
	.enable = clk_system_enable,
	.disable = clk_system_disable,
	.is_enabled = clk_system_is_enabled,
};

static struct clk *
at91_clk_register_system(struct regmap *regmap, const char *name,
			 const char *parent_name, u8 id)
{
	struct clk_system *sys;
	int ret;

	if (!parent_name || id > SYSTEM_MAX_ID)
		return ERR_PTR(-EINVAL);

	sys = xzalloc(sizeof(*sys));
	sys->clk.name = name;
	sys->clk.ops = &system_ops;
	sys->parent_name = parent_name;
	sys->clk.parent_names = &sys->parent_name;
	sys->clk.num_parents = 1;
	/* init.flags = CLK_SET_RATE_PARENT; */
	sys->id = id;
	sys->regmap = regmap;

	ret = clk_register(&sys->clk);
	if (ret) {
		kfree(sys);
		return ERR_PTR(ret);
	}

	return &sys->clk;
}

static int of_at91rm9200_clk_sys_setup(struct device_node *np)
{
	int num;
	u32 id;
	struct clk *clk;
	const char *name;
	struct device_node *sysclknp;
	const char *parent_name;
	struct regmap *regmap;

	num = of_get_child_count(np);
	if (num > (SYSTEM_MAX_ID + 1))
		return -EINVAL;

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	for_each_child_of_node(np, sysclknp) {
		if (of_property_read_u32(sysclknp, "reg", &id))
			continue;

		if (of_property_read_string(np, "clock-output-names", &name))
			name = sysclknp->name;

		parent_name = of_clk_get_parent_name(sysclknp, 0);

		clk = at91_clk_register_system(regmap, name, parent_name, id);
		if (IS_ERR(clk))
			continue;

		of_clk_add_provider(sysclknp, of_clk_src_simple_get, clk);
	}

	return 0;
}
CLK_OF_DECLARE(at91rm9200_clk_sys, "atmel,at91rm9200-clk-system",
	       of_at91rm9200_clk_sys_setup);

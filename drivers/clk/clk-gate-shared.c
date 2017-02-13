/*
 * clk-gate-shared.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2017 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_gate_shared {
	struct clk clk;
	const char *parent;
	const char *companion_gate;
	struct clk *companion_clk;
};

#define to_clk_gate_shared(_clk) container_of(_clk, struct clk_gate_shared, clk)

static struct clk *lookup_companion(struct clk_gate_shared *g)
{
	if (IS_ERR(g->companion_clk))
		g->companion_clk = clk_lookup(g->companion_gate);

	if (IS_ERR(g->companion_clk))
		return NULL;

	return g->companion_clk;
}

static int clk_gate_shared_enable(struct clk *clk)
{
	struct clk_gate_shared *g = to_clk_gate_shared(clk);

	return clk_enable(lookup_companion(g));
}

static void clk_gate_shared_disable(struct clk *clk)
{
	struct clk_gate_shared *g = to_clk_gate_shared(clk);

	clk_disable(lookup_companion(g));
}

static int clk_gate_shared_is_enabled(struct clk *clk)
{
	struct clk_gate_shared *g = to_clk_gate_shared(clk);

	return clk_is_enabled(lookup_companion(g));
}

static struct clk_ops clk_gate_shared_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gate_shared_enable,
	.disable = clk_gate_shared_disable,
	.is_enabled = clk_gate_shared_is_enabled,
};

struct clk *clk_gate_shared_alloc(const char *name, const char *parent, const char *companion,
			   unsigned flags)
{
	struct clk_gate_shared *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->companion_gate = companion;
	g->companion_clk = ERR_PTR(-EINVAL);
	g->clk.ops = &clk_gate_shared_ops;
	g->clk.name = name;
	g->clk.flags = flags;
	g->clk.parent_names = &g->parent;
	g->clk.num_parents = 1;

	return &g->clk;
}

void clk_gate_shared_free(struct clk *clk)
{
	struct clk_gate_shared *g = to_clk_gate_shared(clk);

	free(g);
}

/*
 * clk_gate_shared - register a gate controlled by another gate
 * @name: The name of the new clock gate
 * @parent: The parent name of the new clock
 * companion: The hardware gate this clock is controlled by
 * @flags: common CLK_* flags
 *
 * This gate clock is used when a single software control knob controls multiple
 * gates in hardware. The first gate is then registered as the real hardware gate,
 * the others are registered with this function. This gate has no hardware control
 * itself, but only enables/disabled its companion hardware gate.
 */
struct clk *clk_gate_shared(const char *name, const char *parent, const char *companion,
			    unsigned flags)
{
	struct clk *clk;
	int ret;

	clk = clk_gate_shared_alloc(name , parent, companion, flags);

	ret = clk_register(clk);
	if (ret) {
		clk_gate_shared_free(clk);
		return ERR_PTR(ret);
	}

	return clk;
}

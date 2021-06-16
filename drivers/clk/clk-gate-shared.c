// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk-gate-shared.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2017 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 */
#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

struct clk_gate_shared {
	struct clk_hw hw;
	const char *parent;
	const char *companion_gate;
	struct clk *companion_clk;
};

#define to_clk_gate_shared(_hw) container_of(_hw, struct clk_gate_shared, hw)

static struct clk *lookup_companion(struct clk_gate_shared *g)
{
	if (IS_ERR(g->companion_clk))
		g->companion_clk = clk_lookup(g->companion_gate);

	if (IS_ERR(g->companion_clk))
		return NULL;

	return g->companion_clk;
}

static int clk_gate_shared_enable(struct clk_hw *hw)
{
	struct clk_gate_shared *g = to_clk_gate_shared(hw);

	return clk_enable(lookup_companion(g));
}

static void clk_gate_shared_disable(struct clk_hw *hw)
{
	struct clk_gate_shared *g = to_clk_gate_shared(hw);

	clk_disable(lookup_companion(g));
}

static int clk_gate_shared_is_enabled(struct clk_hw *hw)
{
	struct clk_gate_shared *g = to_clk_gate_shared(hw);

	return clk_is_enabled(lookup_companion(g));
}

static struct clk_ops clk_gate_shared_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.enable = clk_gate_shared_enable,
	.disable = clk_gate_shared_disable,
	.is_enabled = clk_gate_shared_is_enabled,
};

static struct clk *clk_gate_shared_alloc(const char *name, const char *parent,
				const char *companion, unsigned flags)
{
	struct clk_gate_shared *g = xzalloc(sizeof(*g));

	g->parent = parent;
	g->companion_gate = companion;
	g->companion_clk = ERR_PTR(-EINVAL);
	g->hw.clk.ops = &clk_gate_shared_ops;
	g->hw.clk.name = name;
	g->hw.clk.flags = flags;
	g->hw.clk.parent_names = &g->parent;
	g->hw.clk.num_parents = 1;

	return &g->hw.clk;
}

static void clk_gate_shared_free(struct clk *clk)
{
	struct clk_hw *hw = clk_to_clk_hw(clk);
	struct clk_gate_shared *g = to_clk_gate_shared(hw);

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

	ret = bclk_register(clk);
	if (ret) {
		clk_gate_shared_free(clk);
		return ERR_PTR(ret);
	}

	return clk;
}

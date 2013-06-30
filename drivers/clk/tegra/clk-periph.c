/*
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * Based on the Linux Tegra clock code
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

#include "clk.h"

#define to_clk_periph(_hw) container_of(_hw, struct tegra_clk_periph, hw)

static int clk_periph_get_parent(struct clk *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->mux->ops->get_parent(periph->mux);
}

static int clk_periph_set_parent(struct clk *hw, u8 index)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->mux->ops->set_parent(periph->mux, index);
}

static unsigned long clk_periph_recalc_rate(struct clk *hw,
					    unsigned long parent_rate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->recalc_rate(periph->div, parent_rate);
}

static long clk_periph_round_rate(struct clk *hw, unsigned long rate,
				  unsigned long *prate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->round_rate(periph->div, rate, prate);
}

static int clk_periph_set_rate(struct clk *hw, unsigned long rate,
			       unsigned long parent_rate)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->div->ops->set_rate(periph->div, rate, parent_rate);
}

static int clk_periph_is_enabled(struct clk *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);

	return periph->gate->ops->is_enabled(periph->gate);
}

static int clk_periph_enable(struct clk *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);
	u32 reg;

	reg = readl(periph->rst_reg);
	reg |= (1 << periph->rst_shift);
	writel(reg, periph->rst_reg);

	periph->gate->ops->enable(periph->gate);

	udelay(2);

	reg = readl(periph->rst_reg);
	reg &= ~(1 << periph->rst_shift);
	writel(reg, periph->rst_reg);

	return 0;
}

static void clk_periph_disable(struct clk *hw)
{
	struct tegra_clk_periph *periph = to_clk_periph(hw);
	u32 reg;

	reg = readl(periph->rst_reg);
	reg |= (1 << periph->rst_shift);
	writel(reg, periph->rst_reg);

	udelay(2);

	periph->gate->ops->disable(periph->gate);
}

const struct clk_ops tegra_clk_periph_ops = {
	.get_parent = clk_periph_get_parent,
	.set_parent = clk_periph_set_parent,
	.recalc_rate = clk_periph_recalc_rate,
	.round_rate = clk_periph_round_rate,
	.set_rate = clk_periph_set_rate,
	.is_enabled = clk_periph_is_enabled,
	.enable = clk_periph_enable,
	.disable = clk_periph_disable,
};

const struct clk_ops tegra_clk_periph_nodiv_ops = {
	.get_parent = clk_periph_get_parent,
	.set_parent = clk_periph_set_parent,
	.is_enabled = clk_periph_is_enabled,
	.enable = clk_periph_enable,
	.disable = clk_periph_disable,
};

struct clk *_tegra_clk_register_periph(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags,
		bool has_div)
{
	struct tegra_clk_periph *periph;
	int ret;

	periph = kzalloc(sizeof(*periph), GFP_KERNEL);
	if (!periph) {
		pr_err("%s: could not allocate peripheral clk\n",
		       __func__);
		goto out_periph;
	}

	periph->mux = clk_mux_alloc(NULL, clk_base + reg_offset, 30, 2,
				    parent_names, num_parents);
	if (!periph->mux)
		goto out_mux;

	periph->gate = clk_gate_alloc(NULL, NULL, clk_base + 0x10 +
				      ((id >> 3) & 0xc), id & 0x1f);
	if (!periph->gate)
		goto out_gate;

	if (has_div) {
		periph->div = tegra_clk_divider_alloc(NULL, NULL, clk_base +
				reg_offset, 0, TEGRA_DIVIDER_ROUND_UP, 0, 8, 1);
		if (!periph->div)
			goto out_div;
	}

	periph->hw.name = name;
	periph->hw.ops = has_div ? &tegra_clk_periph_ops :
				   &tegra_clk_periph_nodiv_ops;
	periph->hw.parent_names = parent_names;
	periph->hw.num_parents = num_parents;
	periph->flags = flags;
	periph->rst_reg = clk_base + 0x4 + ((id >> 3) & 0xc);
	periph->rst_shift = id & 0x1f;

	ret = clk_register(&periph->hw);
	if (ret)
		goto out_register;

	return &periph->hw;

out_register:
	tegra_clk_divider_free(periph->div);
out_div:
	clk_gate_free(periph->gate);
out_gate:
	clk_mux_free(periph->mux);
out_mux:
	kfree(periph);
out_periph:
	return NULL;
}

struct clk *tegra_clk_register_periph_nodiv(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags)
{
	return _tegra_clk_register_periph(name, parent_names, num_parents,
					  clk_base, reg_offset, id, flags,
					  false);
}

struct clk *tegra_clk_register_periph(const char *name,
		const char **parent_names, int num_parents,
		void __iomem *clk_base, u32 reg_offset, u8 id, u8 flags)
{
	return _tegra_clk_register_periph(name, parent_names, num_parents,
					  clk_base, reg_offset, id, flags,
					  true);
}

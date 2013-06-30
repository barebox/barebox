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
#include <linux/clk.h>

#include "clk.h"

void tegra_init_from_table(struct tegra_clk_init_table *tbl,
				  struct clk *clks[], int clk_max)
{
	struct clk *clk;

	for (; tbl->clk_id < clk_max; tbl++) {
		clk = clks[tbl->clk_id];
		if (!clk)
			return;

		if (tbl->parent_id < clk_max) {
			struct clk *parent = clks[tbl->parent_id];
			if (clk_set_parent(clk, parent)) {
				pr_err("%s: Failed to set parent %s of %s\n",
				       __func__, parent->name, clk->name);
				WARN_ON(1);
			}
		}

		if (tbl->rate)
			if (clk_set_rate(clk, tbl->rate)) {
				pr_err("%s: Failed to set rate %lu of %s\n",
				       __func__, tbl->rate, clk->name);
				WARN_ON(1);
			}

		if (tbl->state)
			if (clk_enable(clk)) {
				pr_err("%s: Failed to enable %s\n", __func__,
				       clk->name);
				WARN_ON(1);
			}
	}
}

/*
 * Copyright (C) 2015 Altera Corporation. All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <regmap.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <mach/arria10-regs.h>
#include <mach/arria10-system-manager.h>

#include "clk.h"

#define to_socfpga_gate_clk(p) container_of(p, struct socfpga_gate_clk, clk)

/* SDMMC Group for System Manager defines */
#define SYSMGR_SDMMCGRP_CTRL_OFFSET	0x28

static unsigned long socfpga_gate_clk_recalc_rate(struct clk *clk,
	unsigned long parent_rate)
{
	struct socfpga_gate_clk *socfpgaclk = to_socfpga_gate_clk(clk);
	u32 div = 1, val;

	if (socfpgaclk->fixed_div)
		div = socfpgaclk->fixed_div;
	else if (socfpgaclk->div_reg) {
		val = readl(socfpgaclk->div_reg) >> socfpgaclk->shift;
		val &= GENMASK(socfpgaclk->width - 1, 0);
		div = (1 << val);
	}

	return parent_rate / div;
}

static int socfpga_clk_prepare(struct clk *clk)
{
	struct socfpga_gate_clk *socfpgaclk = to_socfpga_gate_clk(clk);
	int i;
	u32 hs_timing;
	u32 clk_phase[2];

	if (socfpgaclk->clk_phase[0] || socfpgaclk->clk_phase[1]) {
		for (i = 0; i < ARRAY_SIZE(clk_phase); i++) {
			switch (socfpgaclk->clk_phase[i]) {
			case 0:
				clk_phase[i] = 0;
				break;
			case 45:
				clk_phase[i] = 1;
				break;
			case 90:
				clk_phase[i] = 2;
				break;
			case 135:
				clk_phase[i] = 3;
				break;
			case 180:
				clk_phase[i] = 4;
				break;
			case 225:
				clk_phase[i] = 5;
				break;
			case 270:
				clk_phase[i] = 6;
				break;
			case 315:
				clk_phase[i] = 7;
				break;
			default:
				clk_phase[i] = 0;
				break;
			}
		}

		hs_timing = SYSMGR_SDMMC_CTRL_SET(clk_phase[0], clk_phase[1]);
		writel(hs_timing, ARRIA10_SYSMGR_SDMMC);
	}
	return 0;
}

static int clk_socfpga_enable(struct clk *clk)
{
	struct socfpga_gate_clk *socfpga_clk = to_socfpga_gate_clk(clk);
	u32 val;

	socfpga_clk_prepare(clk);

	val = readl(socfpga_clk->reg);
	val |= 1 << socfpga_clk->bit_idx;
	writel(val, socfpga_clk->reg);

	return 0;
}

static void clk_socfpga_disable(struct clk *clk)
{
	struct socfpga_gate_clk *socfpga_clk = to_socfpga_gate_clk(clk);
	u32 val;

	val = readl(socfpga_clk->reg);
	val &= ~(1 << socfpga_clk->shift);
	writel(val, socfpga_clk->reg);
}

static struct clk_ops gateclk_ops = {
	.recalc_rate = socfpga_gate_clk_recalc_rate,
};

static struct clk *__socfpga_gate_init(struct device_node *node,
	const struct clk_ops *ops)
{
	u32 clk_gate[2];
	u32 div_reg[3];
	u32 clk_phase[2];
	u32 fixed_div;
	struct socfpga_gate_clk *socfpga_clk;
	const char *clk_name = node->name;
	int rc;
	int i;

	socfpga_clk = xzalloc(sizeof(*socfpga_clk));

	rc = of_property_read_u32_array(node, "clk-gate", clk_gate, 2);
	if (rc)
		clk_gate[0] = 0;

	if (clk_gate[0]) {
		socfpga_clk->reg = clk_mgr_base_addr + clk_gate[0];
		socfpga_clk->bit_idx = clk_gate[1];

		gateclk_ops.enable = clk_socfpga_enable;
		gateclk_ops.disable = clk_socfpga_disable;
	}

	rc = of_property_read_u32(node, "fixed-divider", &fixed_div);
	if (rc)
		socfpga_clk->fixed_div = 0;
	else
		socfpga_clk->fixed_div = fixed_div;

	rc = of_property_read_u32_array(node, "div-reg", div_reg, 3);
	if (!rc) {
		socfpga_clk->div_reg = clk_mgr_base_addr + div_reg[0];
		socfpga_clk->shift = div_reg[1];
		socfpga_clk->width = div_reg[2];
	} else {
		socfpga_clk->div_reg = NULL;
	}

	rc = of_property_read_u32_array(node, "clk-phase", clk_phase, 2);
	if (!rc) {
		socfpga_clk->clk_phase[0] = clk_phase[0];
		socfpga_clk->clk_phase[1] = clk_phase[1];
	}

	of_property_read_string(node, "clock-output-names", &clk_name);

	socfpga_clk->clk.name = xstrdup(clk_name);
	socfpga_clk->clk.ops = ops;

	for (i = 0; i < SOCFPGA_MAX_PARENTS; i++) {
		socfpga_clk->parent_names[i] = of_clk_get_parent_name(node, i);
		if (!socfpga_clk->parent_names[i])
			break;
	}

	socfpga_clk->clk.num_parents = i;
	socfpga_clk->clk.parent_names = socfpga_clk->parent_names;

	rc = clk_register(&socfpga_clk->clk);
	if (rc) {
		free(socfpga_clk);
		return ERR_PTR(rc);
	}

	return &socfpga_clk->clk;
}

struct clk *socfpga_a10_gate_init(struct device_node *node)
{
	return __socfpga_gate_init(node, &gateclk_ops);
}

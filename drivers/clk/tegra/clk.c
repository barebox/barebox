/*
 * Copyright (C) 2013-2014 Lucas Stach <l.stach@pengutronix.de>
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
#include <linux/reset-controller.h>
#include <mach/lowlevel.h>

#include "clk.h"

#define CLK_OUT_ENB_L			0x010
#define CLK_OUT_ENB_H			0x014
#define CLK_OUT_ENB_U			0x018
#define CLK_OUT_ENB_V			0x360
#define CLK_OUT_ENB_W			0x364
#define CLK_OUT_ENB_X			0x280
#define CLK_OUT_ENB_SET_L		0x320
#define CLK_OUT_ENB_CLR_L		0x324
#define CLK_OUT_ENB_SET_H		0x328
#define CLK_OUT_ENB_CLR_H		0x32c
#define CLK_OUT_ENB_SET_U		0x330
#define CLK_OUT_ENB_CLR_U		0x334
#define CLK_OUT_ENB_SET_V		0x440
#define CLK_OUT_ENB_CLR_V		0x444
#define CLK_OUT_ENB_SET_W		0x448
#define CLK_OUT_ENB_CLR_W		0x44c
#define CLK_OUT_ENB_SET_X		0x284
#define CLK_OUT_ENB_CLR_X		0x288

#define RST_DEVICES_L			0x004
#define RST_DEVICES_H			0x008
#define RST_DEVICES_U			0x00C
#define RST_DFLL_DVCO			0x2F4
#define RST_DEVICES_V			0x358
#define RST_DEVICES_W			0x35C
#define RST_DEVICES_X			0x28C
#define RST_DEVICES_SET_L		0x300
#define RST_DEVICES_CLR_L		0x304
#define RST_DEVICES_SET_H		0x308
#define RST_DEVICES_CLR_H		0x30c
#define RST_DEVICES_SET_U		0x310
#define RST_DEVICES_CLR_U		0x314
#define RST_DEVICES_SET_V		0x430
#define RST_DEVICES_CLR_V		0x434
#define RST_DEVICES_SET_W		0x438
#define RST_DEVICES_CLR_W		0x43c
#define RST_DEVICES_SET_X		0x290
#define RST_DEVICES_CLR_X		0x294

static struct tegra_clk_periph_regs periph_regs[] = {
	[0] = {
		.enb_reg = CLK_OUT_ENB_L,
		.enb_set_reg = CLK_OUT_ENB_SET_L,
		.enb_clr_reg = CLK_OUT_ENB_CLR_L,
		.rst_reg = RST_DEVICES_L,
		.rst_set_reg = RST_DEVICES_SET_L,
		.rst_clr_reg = RST_DEVICES_CLR_L,
	},
	[1] = {
		.enb_reg = CLK_OUT_ENB_H,
		.enb_set_reg = CLK_OUT_ENB_SET_H,
		.enb_clr_reg = CLK_OUT_ENB_CLR_H,
		.rst_reg = RST_DEVICES_H,
		.rst_set_reg = RST_DEVICES_SET_H,
		.rst_clr_reg = RST_DEVICES_CLR_H,
	},
	[2] = {
		.enb_reg = CLK_OUT_ENB_U,
		.enb_set_reg = CLK_OUT_ENB_SET_U,
		.enb_clr_reg = CLK_OUT_ENB_CLR_U,
		.rst_reg = RST_DEVICES_U,
		.rst_set_reg = RST_DEVICES_SET_U,
		.rst_clr_reg = RST_DEVICES_CLR_U,
	},
	[3] = {
		.enb_reg = CLK_OUT_ENB_V,
		.enb_set_reg = CLK_OUT_ENB_SET_V,
		.enb_clr_reg = CLK_OUT_ENB_CLR_V,
		.rst_reg = RST_DEVICES_V,
		.rst_set_reg = RST_DEVICES_SET_V,
		.rst_clr_reg = RST_DEVICES_CLR_V,
	},
	[4] = {
		.enb_reg = CLK_OUT_ENB_W,
		.enb_set_reg = CLK_OUT_ENB_SET_W,
		.enb_clr_reg = CLK_OUT_ENB_CLR_W,
		.rst_reg = RST_DEVICES_W,
		.rst_set_reg = RST_DEVICES_SET_W,
		.rst_clr_reg = RST_DEVICES_CLR_W,
	},
	[5] = {
		.enb_reg = CLK_OUT_ENB_X,
		.enb_set_reg = CLK_OUT_ENB_SET_X,
		.enb_clr_reg = CLK_OUT_ENB_CLR_X,
		.rst_reg = RST_DEVICES_X,
		.rst_set_reg = RST_DEVICES_SET_X,
		.rst_clr_reg = RST_DEVICES_CLR_X,
	},
};

static void __iomem *car_base;

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

static int tegra_clk_rst_assert(struct reset_controller_dev *rcdev,
                                unsigned long id)
{
	tegra_read_chipid();

	writel(BIT(id % 32), car_base + periph_regs[id / 32].rst_set_reg);

	return 0;
}

static int tegra_clk_rst_deassert(struct reset_controller_dev *rcdev,
                                  unsigned long id)
{
	writel(BIT(id % 32), car_base + periph_regs[id / 32].rst_clr_reg);

	return 0;
}

static struct reset_control_ops rst_ops = {
	.assert = tegra_clk_rst_assert,
	.deassert = tegra_clk_rst_deassert,
};

static struct reset_controller_dev rst_ctlr = {
	.ops = &rst_ops,
	.of_reset_n_cells = 1,
};

void tegra_clk_init_rst_controller(void __iomem *base, struct device_node *np,
                                   unsigned int num)
{
	car_base = base;

	rst_ctlr.of_node = np;
	rst_ctlr.nr_resets = num;
	reset_controller_register(&rst_ctlr);
}

void tegra_clk_reset_uarts(void) {
	int i;
	int console_device_ids[] = {6, 7, 55, 65, 66};

	for (i = 0; i < ARRAY_SIZE(console_device_ids); i++) {
		rst_ops.assert(&rst_ctlr, console_device_ids[i]);
		udelay(2);
		rst_ops.deassert(&rst_ctlr, console_device_ids[i]);
	}
};

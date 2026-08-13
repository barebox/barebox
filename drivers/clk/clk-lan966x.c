// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/clk/clk-lan966x.c?id=b7e5ac83cb16f7ffd11dc23736f84276602100ed
/*
 * Microchip LAN966x SoC Clock driver.
 *
 * Copyright (C) 2021 Microchip Technology, Inc. and its subsidiaries
 *
 * Author: Kavyasree Kotagiri <kavyasree.kotagiri@microchip.com>
 *
 * Ported from Linux drivers/clk/clk-lan966x.c. Structure and identifiers
 * are kept aligned with Linux so future fixes can be backported with
 * minimal context churn.
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <of.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/spinlock.h>

#define GCK_ENA         BIT(0)
#define GCK_SRC_SEL     GENMASK(9, 8)
#define GCK_PRESCALER   GENMASK(23, 16)

#define DIV_MAX		255

static const char * const lan966x_clk_names[] = {
	"qspi0", "qspi1", "qspi2", "sdmmc0",
	"pi", "mcan0", "mcan1", "flexcom0",
	"flexcom1", "flexcom2", "flexcom3",
	"flexcom4", "timer1", "usb_refclk",
};

static const char * const lan969x_clk_names[] = {
	"qspi0", "qspi2", "sdmmc0", "sdmmc1",
	"mcan0", "mcan1", "flexcom0",
	"flexcom1", "flexcom2", "flexcom3",
	"timer1", "usb_refclk",
};

struct lan966x_gck {
	struct clk_hw hw;
	void __iomem *reg;
};
#define to_lan966x_gck(hw) container_of(hw, struct lan966x_gck, hw)

static const struct clk_parent_data lan966x_gck_pdata[] = {
	{ .fw_name = "cpu", .name = "cpu-clk", },
	{ .fw_name = "ddr", .name = "ddr-clk", },
	{ .fw_name = "sys", .name = "fx100-clk", },
};

static struct clk_init_data init = {
	.parent_data = lan966x_gck_pdata,
	.num_parents = ARRAY_SIZE(lan966x_gck_pdata),
};

struct clk_gate_soc_desc {
	const char *name;
	int bit_idx;
};

static const struct clk_gate_soc_desc lan966x_clk_gate_desc[] = {
	{ "uhphs", 11 },
	{ "udphs", 10 },
	{ "mcramc", 9 },
	{ "hmatrix", 8 },
	{ }
};

static const struct clk_gate_soc_desc lan969x_clk_gate_desc[] = {
	{ "usb_drd", 10 },
	{ "mcramc", 9 },
	{ "hmatrix", 8 },
	{ }
};

struct lan966x_match_data {
	char *name;
	const char * const *clk_name;
	const struct clk_gate_soc_desc *clk_gate_desc;
	u8 num_generic_clks;
	u8 num_total_clks;
};

static struct lan966x_match_data lan966x_desc = {
	.name = "lan966x",
	.clk_name = lan966x_clk_names,
	.clk_gate_desc = lan966x_clk_gate_desc,
	.num_total_clks = 18,
	.num_generic_clks = 14,
};

static struct lan966x_match_data lan969x_desc = {
	.name = "lan969x",
	.clk_name = lan969x_clk_names,
	.clk_gate_desc = lan969x_clk_gate_desc,
	.num_total_clks = 15,
	.num_generic_clks = 12,
};

static DEFINE_SPINLOCK(clk_gate_lock);
static void __iomem *base;

static int lan966x_gck_enable(struct clk_hw *hw)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 val = readl(gck->reg);

	val |= GCK_ENA;
	writel(val, gck->reg);

	return 0;
}

static void lan966x_gck_disable(struct clk_hw *hw)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 val = readl(gck->reg);

	val &= ~GCK_ENA;
	writel(val, gck->reg);
}

static int lan966x_gck_set_rate(struct clk_hw *hw,
				unsigned long rate,
				unsigned long parent_rate)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 div, val = readl(gck->reg);

	if (rate == 0 || parent_rate == 0)
		return -EINVAL;

	/* Set Prescalar */
	div = parent_rate / rate;
	val &= ~GCK_PRESCALER;
	val |= FIELD_PREP(GCK_PRESCALER, (div - 1));
	writel(val, gck->reg);

	return 0;
}

static unsigned long lan966x_gck_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 div, val = readl(gck->reg);

	div = FIELD_GET(GCK_PRESCALER, val);

	return parent_rate / (div + 1);
}

/*
 * Linux uses .determine_rate, which barebox does not have. round_rate is
 * called against the already-selected parent, so we just clamp the divider.
 * Source selection happens via .set_parent / .get_parent.
 */
static long lan966x_gck_round_rate(struct clk_hw *hw, unsigned long rate,
				   unsigned long *parent_rate)
{
	unsigned long div;

	if (!rate || !*parent_rate)
		return 0;

	div = DIV_ROUND_CLOSEST(*parent_rate, rate);
	if (div > DIV_MAX + 1)
		div = DIV_MAX + 1;
	if (div < 1)
		div = 1;

	return *parent_rate / div;
}

static int lan966x_gck_get_parent(struct clk_hw *hw)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 val = readl(gck->reg);

	return FIELD_GET(GCK_SRC_SEL, val);
}

static int lan966x_gck_set_parent(struct clk_hw *hw, u8 index)
{
	struct lan966x_gck *gck = to_lan966x_gck(hw);
	u32 val = readl(gck->reg);

	val &= ~GCK_SRC_SEL;
	val |= FIELD_PREP(GCK_SRC_SEL, index);
	writel(val, gck->reg);

	return 0;
}

static const struct clk_ops lan966x_gck_ops = {
	.enable		= lan966x_gck_enable,
	.disable	= lan966x_gck_disable,
	.set_rate	= lan966x_gck_set_rate,
	.recalc_rate	= lan966x_gck_recalc_rate,
	.round_rate	= lan966x_gck_round_rate,
	.set_parent	= lan966x_gck_set_parent,
	.get_parent	= lan966x_gck_get_parent,
};

static struct clk_hw *lan966x_gck_clk_register(struct device *dev, int i)
{
	struct lan966x_gck *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->reg = base + (i * 4);
	priv->hw.init = &init;
	ret = clk_hw_register(dev, &priv->hw);
	if (ret) {
		free(priv);
		return ERR_PTR(ret);
	}

	return &priv->hw;
};

static int lan966x_gate_clk_register(struct device *dev,
				     const struct lan966x_match_data *data,
				     struct clk_onecell_data *clk_data,
				     void __iomem *gate_base)
{
	struct clk_hw *hw;
	int i;

	for (i = data->num_generic_clks; i < data->num_total_clks; ++i) {
		int idx = i - data->num_generic_clks;
		const struct clk_gate_soc_desc *desc;

		desc = &data->clk_gate_desc[idx];

		hw = clk_hw_register_gate(dev, desc->name,
					  data->name, 0, gate_base,
					  desc->bit_idx,
					  0, &clk_gate_lock);
		if (IS_ERR(hw)) {
			dev_err(dev, "failed to register %s clock\n",
				desc->name);
			return PTR_ERR(hw);
		}
		clk_data->clks[i] = clk_hw_to_clk(hw);
	}

	return 0;
}

static int lan966x_clk_probe(struct device *dev)
{
	const struct lan966x_match_data *data;
	struct clk_onecell_data *clk_data;
	struct resource *iores;
	int i, ret;

	data = device_get_match_data(dev);
	if (!data)
		return -EINVAL;

	clk_data = xzalloc(sizeof(*clk_data));
	clk_data->clks = xzalloc(data->num_total_clks *
				 sizeof(*clk_data->clks));
	clk_data->clk_num = data->num_total_clks;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	base = IOMEM(iores->start);

	init.ops = &lan966x_gck_ops;

	for (i = 0; i < data->num_generic_clks; i++) {
		struct clk_hw *hw;

		init.name = data->clk_name[i];
		hw = lan966x_gck_clk_register(dev, i);
		if (IS_ERR(hw)) {
			dev_err(dev, "failed to register %s clock\n",
				init.name);
			return PTR_ERR(hw);
		}
		clk_data->clks[i] = clk_hw_to_clk(hw);
	}

	iores = dev_request_mem_resource(dev, 1);
	if (!IS_ERR(iores)) {
		void __iomem *gate_base = IOMEM(iores->start);

		ret = lan966x_gate_clk_register(dev, data, clk_data, gate_base);
		if (ret)
			return ret;
	}

	return of_clk_add_provider(dev->of_node, of_clk_src_onecell_get,
				   clk_data);
}

static const struct of_device_id lan966x_clk_dt_ids[] = {
	{ .compatible = "microchip,lan966x-gck", .data = &lan966x_desc },
	{ .compatible = "microchip,lan9691-gck", .data = &lan969x_desc },
	{ }
};
MODULE_DEVICE_TABLE(of, lan966x_clk_dt_ids);

static struct driver lan966x_clk_driver = {
	.name = "lan966x-clk",
	.probe = lan966x_clk_probe,
	.of_compatible = DRV_OF_COMPAT(lan966x_clk_dt_ids),
};
postcore_platform_driver(lan966x_clk_driver);

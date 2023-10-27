// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Copyright (C) 2018 Microchip Technology Inc,
 *                     Codrin Ciubotariu <codrin.ciubotariu@microchip.com>
 *
 *
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/overflow.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include <soc/at91/atmel-sfr.h>

#include "pmc.h"

struct clk_i2s_mux {
	struct clk_hw hw;
	struct regmap *regmap;
	u8 bus_id;
	const char *parent_names[];
};

#define to_clk_i2s_mux(_hw) container_of(_hw, struct clk_i2s_mux, hw)

static int clk_i2s_mux_get_parent(struct clk_hw *hw)
{
	struct clk_i2s_mux *mux = to_clk_i2s_mux(hw);
	u32 val;

	regmap_read(mux->regmap, AT91_SFR_I2SCLKSEL, &val);

	return (val & BIT(mux->bus_id)) >> mux->bus_id;
}

static int clk_i2s_mux_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_i2s_mux *mux = to_clk_i2s_mux(hw);

	return regmap_update_bits(mux->regmap, AT91_SFR_I2SCLKSEL,
				  BIT(mux->bus_id), index << mux->bus_id);
}

static const struct clk_ops clk_i2s_mux_ops = {
	.set_rate = clk_parent_set_rate,
	.round_rate = clk_parent_round_rate,
	.get_parent = clk_i2s_mux_get_parent,
	.set_parent = clk_i2s_mux_set_parent,
};

struct clk * __init
at91_clk_i2s_mux_register(struct regmap *regmap, const char *name,
			  const char * const *parent_names,
			  unsigned int num_parents, u8 bus_id)
{
	struct clk_i2s_mux *i2s_ck;
	int ret;

	i2s_ck = kzalloc(struct_size(i2s_ck, parent_names, num_parents), GFP_KERNEL);
	if (!i2s_ck)
		return ERR_PTR(-ENOMEM);

	i2s_ck->hw.clk.name = name;
	i2s_ck->hw.clk.ops = &clk_i2s_mux_ops;
	memcpy(i2s_ck->parent_names, parent_names,
	       num_parents * sizeof(i2s_ck->parent_names[0]));
	i2s_ck->hw.clk.parent_names = &i2s_ck->parent_names[0];
	i2s_ck->hw.clk.num_parents = num_parents;

	i2s_ck->bus_id = bus_id;
	i2s_ck->regmap = regmap;

	ret = bclk_register(&i2s_ck->hw.clk);
	if (ret) {
		kfree(i2s_ck);
		return ERR_PTR(ret);
	}

	return &i2s_ck->hw.clk;
}

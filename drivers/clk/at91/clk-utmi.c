// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>

#include <soc/at91/atmel-sfr.h>

#include "pmc.h"

/*
 * The purpose of this clock is to generate a 480 MHz signal. A different
 * rate can't be configured.
 */
#define UTMI_RATE      480000000

struct clk_utmi {
	struct clk_hw hw;
	const char *parent;
	struct regmap *regmap_pmc;
	struct regmap *regmap_sfr;
};

#define to_clk_utmi(_hw) container_of(_hw, struct clk_utmi, hw)

static inline bool clk_utmi_ready(struct regmap *regmap)
{
	unsigned int status;

	regmap_read(regmap, AT91_PMC_SR, &status);

	return status & AT91_PMC_LOCKU;
}

static int clk_utmi_enable(struct clk_hw *hw)
{
	struct clk *hw_parent;
	struct clk_utmi *utmi = to_clk_utmi(hw);
	unsigned int uckr = AT91_PMC_UPLLEN | AT91_PMC_UPLLCOUNT |
			    AT91_PMC_BIASEN;
	unsigned int utmi_ref_clk_freq;
	unsigned long parent_rate;

	/*
	 * If mainck rate is different from 12 MHz, we have to configure the
	 * FREQ field of the SFR_UTMICKTRIM register to generate properly
	 * the utmi clock.
	 */
	hw_parent = clk_get_parent(clk_hw_to_clk(hw));
	parent_rate = clk_get_rate(hw_parent);

	switch (parent_rate) {
	case 12000000:
		utmi_ref_clk_freq = 0;
		break;
	case 16000000:
		utmi_ref_clk_freq = 1;
		break;
	case 24000000:
		utmi_ref_clk_freq = 2;
		break;
	/*
	 * Not supported on SAMA5D2 but it's not an issue since MAINCK
	 * maximum value is 24 MHz.
	 */
	case 48000000:
		utmi_ref_clk_freq = 3;
		break;
	default:
		pr_err("UTMICK: unsupported mainck rate\n");
		return -EINVAL;
	}


	if (utmi->regmap_sfr) {
		regmap_write_bits(utmi->regmap_sfr, AT91_SFR_UTMICKTRIM,
				  AT91_UTMICKTRIM_FREQ, utmi_ref_clk_freq);
	} else if (utmi_ref_clk_freq) {
		pr_err("UTMICK: sfr node required\n");
		return -EINVAL;
	}
	regmap_write_bits(utmi->regmap_pmc, AT91_CKGR_UCKR, uckr, uckr);


	while (!clk_utmi_ready(utmi->regmap_pmc))
		barrier();

	return 0;
}

static int clk_utmi_is_enabled(struct clk_hw *hw)
{
	struct clk_utmi *utmi = to_clk_utmi(hw);

	return clk_utmi_ready(utmi->regmap_pmc);
}

static void clk_utmi_disable(struct clk_hw *hw)
{
	struct clk_utmi *utmi = to_clk_utmi(hw);

	regmap_write_bits(utmi->regmap_pmc, AT91_CKGR_UCKR,
			  AT91_PMC_UPLLEN, 0);
}

static unsigned long clk_utmi_recalc_rate(struct clk_hw *hw,
					  unsigned long parent_rate)
{
	/* UTMI clk rate is fixed */
	return UTMI_RATE;
}

static const struct clk_ops utmi_ops = {
	.enable = clk_utmi_enable,
	.disable = clk_utmi_disable,
	.is_enabled = clk_utmi_is_enabled,
	.recalc_rate = clk_utmi_recalc_rate,
};

struct clk * __init
at91_clk_register_utmi(struct regmap *regmap_pmc, struct regmap *regmap_sfr,
		       const char *name, const char *parent_name)
{
	int ret;
	struct clk_utmi *utmi;

	utmi = xzalloc(sizeof(*utmi));

	utmi->hw.clk.name = name;
	utmi->hw.clk.ops = &utmi_ops;

	if (parent_name) {
		utmi->parent = parent_name;
		utmi->hw.clk.parent_names = &utmi->parent;
		utmi->hw.clk.num_parents = 1;
	}

	/* utmi->clk.flags = CLK_SET_RATE_GATE; */

	utmi->regmap_pmc = regmap_pmc;
	utmi->regmap_sfr = regmap_sfr;

	ret = bclk_register(&utmi->hw.clk);
	if (ret) {
		kfree(utmi);
		return ERR_PTR(ret);
	}

	return &utmi->hw.clk;
}

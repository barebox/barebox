// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * drivers/clk/at91/sckc.c
 *
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <of_address.h>
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <linux/overflow.h>
#include <mfd/syscon.h>
#include <linux/regmap.h>



#define SLOW_CLOCK_FREQ		32768
#define SLOWCK_SW_CYCLES	5
#define SLOWCK_SW_TIME_USEC	((SLOWCK_SW_CYCLES * USEC_PER_SEC) / \
				 SLOW_CLOCK_FREQ)

#define	AT91_SCKC_CR			0x00

struct clk_slow_bits {
	u32 cr_rcen;
	u32 cr_osc32en;
	u32 cr_osc32byp;
	u32 cr_oscsel;
};

struct clk_slow_osc {
	struct clk_hw hw;
	void __iomem *sckcr;
	const struct clk_slow_bits *bits;
	unsigned long startup_usec;
	const char *parent_name;
};

#define to_clk_slow_osc(_hw) container_of(_hw, struct clk_slow_osc, hw)

struct clk_sama5d4_slow_osc {
	struct clk_hw hw;
	void __iomem *sckcr;
	const struct clk_slow_bits *bits;
	unsigned long startup_usec;
	bool prepared;
	const char *parent_name;
};

#define to_clk_sama5d4_slow_osc(_hw) container_of(_hw, struct clk_sama5d4_slow_osc, hw)

struct clk_slow_rc_osc {
	struct clk_hw hw;
	void __iomem *sckcr;
	const struct clk_slow_bits *bits;
	unsigned long frequency;
	unsigned long startup_usec;
	const char *parent_name;
};

#define to_clk_slow_rc_osc(_hw) container_of(_hw, struct clk_slow_rc_osc, hw)

struct clk_sam9x5_slow {
	struct clk_hw hw;
	void __iomem *sckcr;
	const struct clk_slow_bits *bits;
	u8 parent;
	const char *parent_names[];
};

#define to_clk_sam9x5_slow(_hw) container_of(_hw, struct clk_sam9x5_slow, hw)

static int clk_slow_osc_enable(struct clk_hw *hw)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(hw);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & (osc->bits->cr_osc32byp | osc->bits->cr_osc32en))
		return 0;

	writel(tmp | osc->bits->cr_osc32en, sckcr);

	udelay(osc->startup_usec);

	return 0;
}

static void clk_slow_osc_disable(struct clk_hw *hw)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(hw);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & osc->bits->cr_osc32byp)
		return;

	writel(tmp & ~osc->bits->cr_osc32en, sckcr);
}

static int clk_slow_osc_is_enabled(struct clk_hw *hw)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(hw);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & osc->bits->cr_osc32byp)
		return 1;

	return !!(tmp & osc->bits->cr_osc32en);
}

static const struct clk_ops slow_osc_ops = {
	.enable = clk_slow_osc_enable,
	.disable = clk_slow_osc_disable,
	.is_enabled = clk_slow_osc_is_enabled,
};

static struct clk * __init
at91_clk_register_slow_osc(void __iomem *sckcr,
			   const char *name,
			   const char *parent_name,
			   unsigned long startup,
			   bool bypass,
			   const struct clk_slow_bits *bits)
{
	int ret;
	struct clk_slow_osc *osc;

	if (!sckcr || !name || !parent_name)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));

	osc->hw.clk.name = name;
	osc->hw.clk.ops = &slow_osc_ops;
	osc->parent_name = parent_name;
	osc->hw.clk.parent_names = &osc->parent_name;
	osc->hw.clk.num_parents = 1;
	/* osc->clk.flags = CLK_IGNORE_UNUSED; */

	osc->sckcr = sckcr;
	osc->startup_usec = startup;
	osc->bits = bits;

	if (bypass)
		writel((readl(sckcr) & ~osc->bits->cr_osc32en) |
					osc->bits->cr_osc32byp, sckcr);

	ret = bclk_register(&osc->hw.clk);
	if (ret) {
		kfree(osc);
		return ERR_PTR(ret);
	}

	return &osc->hw.clk;
}

static void at91_clk_unregister_slow_osc(struct clk *clk)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(clk_to_clk_hw(clk));

	clk_unregister(clk);
	kfree(osc);
}

static unsigned long clk_slow_rc_osc_recalc_rate(struct clk_hw *hw,
						 unsigned long parent_rate)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(hw);

	return osc->frequency;
}

static int clk_slow_rc_osc_enable(struct clk_hw *hw)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(hw);
	void __iomem *sckcr = osc->sckcr;

	writel(readl(sckcr) | osc->bits->cr_rcen, sckcr);

	udelay(osc->startup_usec);

	return 0;
}

static void clk_slow_rc_osc_disable(struct clk_hw *hw)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(hw);
	void __iomem *sckcr = osc->sckcr;

	writel(readl(sckcr) & ~osc->bits->cr_rcen, sckcr);
}

static int clk_slow_rc_osc_is_enabled(struct clk_hw *hw)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(hw);

	return !!(readl(osc->sckcr) & osc->bits->cr_rcen);
}

static const struct clk_ops slow_rc_osc_ops = {
	.enable = clk_slow_rc_osc_enable,
	.disable = clk_slow_rc_osc_disable,
	.is_enabled = clk_slow_rc_osc_is_enabled,
	.recalc_rate = clk_slow_rc_osc_recalc_rate,
};

static struct clk * __init
at91_clk_register_slow_rc_osc(void __iomem *sckcr,
			      const char *name,
			      unsigned long frequency,
			      unsigned long accuracy,
			      unsigned long startup,
			      const struct clk_slow_bits *bits)
{
	struct clk_slow_rc_osc *osc;
	int ret;

	if (!sckcr || !name)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));
	osc->hw.clk.name = name;
	osc->hw.clk.ops = &slow_rc_osc_ops;
	osc->hw.clk.parent_names = NULL;
	osc->hw.clk.num_parents = 0;
	/* init.flags = CLK_IGNORE_UNUSED; */

	osc->sckcr = sckcr;
	osc->bits = bits;
	osc->frequency = frequency;
	osc->startup_usec = startup;

	ret = bclk_register(&osc->hw.clk);
	if (ret) {
		kfree(osc);
		return ERR_PTR(ret);
	}

	return &osc->hw.clk;
}

static void at91_clk_unregister_slow_rc_osc(struct clk *clk)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(clk_to_clk_hw(clk));

	clk_unregister(clk);
	kfree(osc);
}

static int clk_sam9x5_slow_set_parent(struct clk_hw *hw, u8 index)
{
	struct clk_sam9x5_slow *slowck = to_clk_sam9x5_slow(hw);
	void __iomem *sckcr = slowck->sckcr;
	u32 tmp;

	if (index > 1)
		return -EINVAL;

	tmp = readl(sckcr);

	if ((!index && !(tmp & slowck->bits->cr_oscsel)) ||
	    (index && (tmp & slowck->bits->cr_oscsel)))
		return 0;

	if (index)
		tmp |= slowck->bits->cr_oscsel;
	else
		tmp &= ~slowck->bits->cr_oscsel;

	writel(tmp, sckcr);

	udelay(SLOWCK_SW_TIME_USEC);

	return 0;
}

static int clk_sam9x5_slow_get_parent(struct clk_hw *hw)
{
	struct clk_sam9x5_slow *slowck = to_clk_sam9x5_slow(hw);

	return !!(readl(slowck->sckcr) & slowck->bits->cr_oscsel);
}

static const struct clk_ops sam9x5_slow_ops = {
	.set_parent = clk_sam9x5_slow_set_parent,
	.get_parent = clk_sam9x5_slow_get_parent,
};

static struct clk * __init
at91_clk_register_sam9x5_slow(void __iomem *sckcr,
			      const char *name,
			      const char **parent_names,
			      int num_parents,
			      const struct clk_slow_bits *bits)
{
	struct clk_sam9x5_slow *slowck;
	int ret;

	if (!sckcr || !name || !parent_names || !num_parents)
		return ERR_PTR(-EINVAL);

	slowck = xzalloc(struct_size(slowck, parent_names, num_parents));
	slowck->hw.clk.name = name;
	slowck->hw.clk.ops = &sam9x5_slow_ops;

	memcpy(slowck->parent_names, parent_names,
	       num_parents * sizeof(slowck->parent_names[0]));
	slowck->hw.clk.parent_names = slowck->parent_names;
	slowck->hw.clk.num_parents = num_parents;
	slowck->sckcr = sckcr;
	slowck->bits = bits;
	slowck->parent = !!(readl(sckcr) & slowck->bits->cr_oscsel);

	ret = bclk_register(&slowck->hw.clk);
	if (ret) {
		kfree(slowck);
		return ERR_PTR(ret);
	}

	return &slowck->hw.clk;
}

static void at91_clk_unregister_sam9x5_slow(struct clk *clk)
{
	struct clk_sam9x5_slow *slowck = to_clk_sam9x5_slow(clk_to_clk_hw(clk));

	clk_unregister(clk);
	kfree(slowck);
}

static void __init at91sam9x5_sckc_register(struct device_node *np,
					    unsigned int rc_osc_startup_us,
					    const struct clk_slow_bits *bits)
{
	const char *parent_names[2] = { "slow_rc_osc", "slow_osc" };
	void __iomem *regbase = of_iomap(np, 0);
	struct device_node *child = NULL;
	const char *xtal_name;
	struct clk *slow_rc, *slow_osc, *slowck;
	bool bypass;
	int ret;

	if (!regbase)
		return;

	slow_rc = at91_clk_register_slow_rc_osc(regbase, parent_names[0],
						32768, 50000000,
						rc_osc_startup_us, bits);
	if (IS_ERR(slow_rc))
		return;

	xtal_name = of_clk_get_parent_name(np, 0);
	if (!xtal_name) {
		/* DT backward compatibility */
		child = of_get_compatible_child(np, "atmel,at91sam9x5-clk-slow-osc");
		if (!child)
			goto unregister_slow_rc;

		xtal_name = of_clk_get_parent_name(child, 0);
		bypass = of_property_read_bool(child, "atmel,osc-bypass");

		child =  of_get_compatible_child(np, "atmel,at91sam9x5-clk-slow");
	} else {
		bypass = of_property_read_bool(np, "atmel,osc-bypass");
	}

	if (!xtal_name)
		goto unregister_slow_rc;

	slow_osc = at91_clk_register_slow_osc(regbase, parent_names[1],
					      xtal_name, 1200000, bypass, bits);
	if (IS_ERR(slow_osc))
		goto unregister_slow_rc;

	slowck = at91_clk_register_sam9x5_slow(regbase, "slowck", parent_names,
					       2, bits);
	if (IS_ERR(slowck))
		goto unregister_slow_osc;

	/* DT backward compatibility */
	if (child)
		ret = of_clk_add_provider(child, of_clk_src_simple_get,
					     slowck);
	else
		ret = of_clk_add_provider(np, of_clk_src_simple_get, slowck);

	if (WARN_ON(ret))
		goto unregister_slowck;

	return;

unregister_slowck:
	at91_clk_unregister_sam9x5_slow(slowck);
unregister_slow_osc:
	at91_clk_unregister_slow_osc(slow_osc);
unregister_slow_rc:
	at91_clk_unregister_slow_rc_osc(slow_rc);
}

static const struct clk_slow_bits at91sam9x5_bits = {
	.cr_rcen = BIT(0),
	.cr_osc32en = BIT(1),
	.cr_osc32byp = BIT(2),
	.cr_oscsel = BIT(3),
};

static void __init of_at91sam9x5_sckc_setup(struct device_node *np)
{
	at91sam9x5_sckc_register(np, 75, &at91sam9x5_bits);
}
CLK_OF_DECLARE(at91sam9x5_clk_sckc, "atmel,at91sam9x5-sckc",
	       of_at91sam9x5_sckc_setup);

static void __init of_sama5d3_sckc_setup(struct device_node *np)
{
	at91sam9x5_sckc_register(np, 500, &at91sam9x5_bits);
}
CLK_OF_DECLARE(sama5d3_clk_sckc, "atmel,sama5d3-sckc",
	       of_sama5d3_sckc_setup);

static const struct clk_slow_bits at91sam9x60_bits = {
	.cr_osc32en = BIT(1),
	.cr_osc32byp = BIT(2),
	.cr_oscsel = BIT(24),
};

static void __init of_sam9x60_sckc_setup(struct device_node *np)
{
	void __iomem *regbase = of_iomap(np, 0);
	struct clk_onecell_data *clk_data;
	struct clk *slow_rc, *slow_osc;
	const char *xtal_name;
	const char *parent_names[2] = { "slow_rc_osc", "slow_osc" };
	bool bypass;
	int ret;

	if (!regbase)
		return;

	slow_rc = clk_register_fixed_rate(parent_names[0], NULL, 0,
					  32768);
	if (IS_ERR(slow_rc))
		return;

	xtal_name = of_clk_get_parent_name(np, 0);
	if (!xtal_name)
		goto unregister_slow_rc;

	bypass = of_property_read_bool(np, "atmel,osc-bypass");
	slow_osc = at91_clk_register_slow_osc(regbase, parent_names[1],
					      xtal_name, 5000000, bypass,
					      &at91sam9x60_bits);
	if (IS_ERR(slow_osc))
		goto unregister_slow_rc;

	clk_data = kzalloc(sizeof(*clk_data), GFP_KERNEL);
	if (!clk_data)
		goto unregister_slow_osc;

	/* MD_SLCK and TD_SLCK. */
	clk_data->clk_num = 2;
	clk_data->clks = kcalloc(clk_data->clk_num,
				 sizeof(*clk_data->clks), GFP_KERNEL);
	if (!clk_data->clks)
		goto clk_data_free;

	clk_data->clks[0] = clk_register_fixed_rate("md_slck",
						   parent_names[0],
						   0, 32768);
	if (IS_ERR(clk_data->clks[0]))
		goto clks_free;

	clk_data->clks[1] = at91_clk_register_sam9x5_slow(regbase, "td_slck",
							 parent_names, 2,
							 &at91sam9x60_bits);
	if (IS_ERR(clk_data->clks[1]))
		goto unregister_md_slck;

	ret = of_clk_add_provider(np, of_clk_src_onecell_get, clk_data);
	if (WARN_ON(ret))
		goto unregister_td_slck;

	return;

unregister_td_slck:
	at91_clk_unregister_sam9x5_slow(clk_data->clks[1]);
unregister_md_slck:
	clk_unregister(clk_data->clks[0]);
clks_free:
	kfree(clk_data->clks);
clk_data_free:
	kfree(clk_data);
unregister_slow_osc:
	at91_clk_unregister_slow_osc(slow_osc);
unregister_slow_rc:
	clk_unregister(slow_rc);
}
CLK_OF_DECLARE(sam9x60_clk_sckc, "microchip,sam9x60-sckc",
	       of_sam9x60_sckc_setup);

static int clk_sama5d4_slow_osc_enable(struct clk_hw *hw)
{
	struct clk_sama5d4_slow_osc *osc = to_clk_sama5d4_slow_osc(hw);

	if (osc->prepared)
		return 0;

	/*
	 * Assume that if it has already been selected (for example by the
	 * bootloader), enough time has aready passed.
	 */
	if ((readl(osc->sckcr) & osc->bits->cr_oscsel)) {
		osc->prepared = true;
		return 0;
	}

	udelay(osc->startup_usec);
	osc->prepared = true;

	return 0;
}

static int clk_sama5d4_slow_osc_is_enabled(struct clk_hw *hw)
{
	struct clk_sama5d4_slow_osc *osc = to_clk_sama5d4_slow_osc(hw);

	return osc->prepared;
}

static const struct clk_ops sama5d4_slow_osc_ops = {
	.enable = clk_sama5d4_slow_osc_enable,
	.is_enabled = clk_sama5d4_slow_osc_is_enabled,
};

static const struct clk_slow_bits at91sama5d4_bits = {
	.cr_oscsel = BIT(3),
};

static void __init of_sama5d4_sckc_setup(struct device_node *np)
{
	void __iomem *regbase = of_iomap(np, 0);
	struct clk *slow_rc, *slowck;
	struct clk_sama5d4_slow_osc *osc;
	const char *parent_names[2] = { "slow_rc_osc", "slow_osc" };
	int ret;

	if (!regbase)
		return;

	slow_rc = clk_fixed(parent_names[0], 32768);
	if (IS_ERR(slow_rc))
		return;

	osc = xzalloc(sizeof(*osc));
	osc->parent_name = of_clk_get_parent_name(np, 0);
	osc->hw.clk.name = parent_names[1];
	osc->hw.clk.ops = &sama5d4_slow_osc_ops;
	osc->hw.clk.parent_names = &osc->parent_name;
	osc->hw.clk.num_parents = 1;

	/* osc->clk.flags = CLK_IGNORE_UNUSED; */

	osc->sckcr = regbase;
	osc->startup_usec = 1200000;
	osc->bits = &at91sama5d4_bits;

	ret = bclk_register(&osc->hw.clk);
	if (ret)
		goto free_slow_osc_data;

	slowck = at91_clk_register_sam9x5_slow(regbase, "slowck",
					       parent_names, 2,
					       &at91sama5d4_bits);
	if (IS_ERR(slowck))
		goto unregister_slow_osc;

	ret = of_clk_add_provider(np, of_clk_src_simple_get, slowck);
	if (WARN_ON(ret))
		goto unregister_slowck;

	return;

unregister_slowck:
	at91_clk_unregister_sam9x5_slow(slowck);
unregister_slow_osc:
	clk_unregister(&osc->hw.clk);
free_slow_osc_data:
	kfree(osc);
	clk_unregister(slow_rc);
}
CLK_OF_DECLARE(sama5d4_clk_sckc, "atmel,sama5d4-sckc",
	       of_sama5d4_sckc_setup);

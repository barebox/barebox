/*
 * drivers/clk/at91/sckc.c
 *
 *  Copyright (C) 2013 Boris BREZILLON <b.brezillon@overkiz.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <of_address.h>
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>



#define SLOW_CLOCK_FREQ		32768
#define SLOWCK_SW_CYCLES	5
#define SLOWCK_SW_TIME_USEC	((SLOWCK_SW_CYCLES * SECOND) / \
				 SLOW_CLOCK_FREQ)

#define	AT91_SCKC_CR			0x00
#define		AT91_SCKC_RCEN		(1 << 0)
#define		AT91_SCKC_OSC32EN	(1 << 1)
#define		AT91_SCKC_OSC32BYP	(1 << 2)
#define		AT91_SCKC_OSCSEL	(1 << 3)

struct clk_slow_osc {
	struct clk clk;
	void __iomem *sckcr;
	unsigned long startup_usec;
	const char *parent_name;
};

#define to_clk_slow_osc(clk) container_of(clk, struct clk_slow_osc, clk)

struct clk_sama5d4_slow_osc {
	struct clk clk;
	void __iomem *sckcr;
	unsigned long startup_usec;
	bool prepared;
	const char *parent_name;
};

#define to_clk_sama5d4_slow_osc(clk) container_of(clk, struct clk_sama5d4_slow_osc, clk)

struct clk_slow_rc_osc {
	struct clk clk;
	void __iomem *sckcr;
	unsigned long frequency;
	unsigned long startup_usec;
	const char *parent_name;
};

#define to_clk_slow_rc_osc(clk) container_of(clk, struct clk_slow_rc_osc, clk)

struct clk_sam9x5_slow {
	struct clk clk;
	void __iomem *sckcr;
	u8 parent;
	const char *parent_names[2];
};

#define to_clk_sam9x5_slow(clk) container_of(clk, struct clk_sam9x5_slow, clk)

static int clk_slow_osc_enable(struct clk *clk)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(clk);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & (AT91_SCKC_OSC32BYP | AT91_SCKC_OSC32EN))
		return 0;

	writel(tmp | AT91_SCKC_OSC32EN, sckcr);

	udelay(osc->startup_usec);

	return 0;
}

static void clk_slow_osc_disable(struct clk *clk)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(clk);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & AT91_SCKC_OSC32BYP)
		return;

	writel(tmp & ~AT91_SCKC_OSC32EN, sckcr);
}

static int clk_slow_osc_is_enabled(struct clk *clk)
{
	struct clk_slow_osc *osc = to_clk_slow_osc(clk);
	void __iomem *sckcr = osc->sckcr;
	u32 tmp = readl(sckcr);

	if (tmp & AT91_SCKC_OSC32BYP)
		return 1;

	return !!(tmp & AT91_SCKC_OSC32EN);
}

static const struct clk_ops slow_osc_ops = {
	.enable = clk_slow_osc_enable,
	.disable = clk_slow_osc_disable,
	.is_enabled = clk_slow_osc_is_enabled,
};

static struct clk *
at91_clk_register_slow_osc(void __iomem *sckcr,
			   const char *name,
			   const char *parent_name,
			   unsigned long startup,
			   bool bypass)
{
	int ret;
	struct clk_slow_osc *osc;

	if (!sckcr || !name || !parent_name)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));

	osc->clk.name = name;
	osc->clk.ops = &slow_osc_ops;
	osc->parent_name = parent_name;
	osc->clk.parent_names = &osc->parent_name;
	osc->clk.num_parents = 1;

	osc->sckcr = sckcr;
	osc->startup_usec = startup;

	if (bypass)
		writel((readl(sckcr) & ~AT91_SCKC_OSC32EN) | AT91_SCKC_OSC32BYP,
		       sckcr);

	ret = clk_register(&osc->clk);
	if (ret) {
		kfree(osc);
		return ERR_PTR(ret);
	}

	return &osc->clk;
}

static void
of_at91sam9x5_clk_slow_osc_setup(struct device_node *np, void __iomem *sckcr)
{
	struct clk *clk;
	const char *parent_name;
	const char *name = np->name;
	u32 startup;
	bool bypass;

	parent_name = of_clk_get_parent_name(np, 0);
	of_property_read_string(np, "clock-output-names", &name);
	of_property_read_u32(np, "atmel,startup-time-usec", &startup);
	bypass = of_property_read_bool(np, "atmel,osc-bypass");

	clk = at91_clk_register_slow_osc(sckcr, name, parent_name, startup,
					 bypass);
	if (IS_ERR(clk))
		return;

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
}

static unsigned long clk_slow_rc_osc_recalc_rate(struct clk *clk,
						 unsigned long parent_rate)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(clk);

	return osc->frequency;
}

static int clk_slow_rc_osc_enable(struct clk *clk)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(clk);
	void __iomem *sckcr = osc->sckcr;

	writel(readl(sckcr) | AT91_SCKC_RCEN, sckcr);

	udelay(osc->startup_usec);

	return 0;
}

static void clk_slow_rc_osc_disable(struct clk *clk)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(clk);
	void __iomem *sckcr = osc->sckcr;

	writel(readl(sckcr) & ~AT91_SCKC_RCEN, sckcr);
}

static int clk_slow_rc_osc_is_enabled(struct clk *clk)
{
	struct clk_slow_rc_osc *osc = to_clk_slow_rc_osc(clk);

	return !!(readl(osc->sckcr) & AT91_SCKC_RCEN);
}

static const struct clk_ops slow_rc_osc_ops = {
	.enable = clk_slow_rc_osc_enable,
	.disable = clk_slow_rc_osc_disable,
	.is_enabled = clk_slow_rc_osc_is_enabled,
	.recalc_rate = clk_slow_rc_osc_recalc_rate,
};

static struct clk *
at91_clk_register_slow_rc_osc(void __iomem *sckcr,
			      const char *name,
			      unsigned long frequency,
			      unsigned long startup)
{
	struct clk_slow_rc_osc *osc;
	int ret;

	if (!sckcr || !name)
		return ERR_PTR(-EINVAL);

	osc = xzalloc(sizeof(*osc));
	osc->clk.name = name;
	osc->clk.ops = &slow_rc_osc_ops;
	osc->clk.parent_names = NULL;
	osc->clk.num_parents = 0;
	/* init.flags = CLK_IGNORE_UNUSED; */

	osc->sckcr = sckcr;
	osc->frequency = frequency;
	osc->startup_usec = startup;

	ret = clk_register(&osc->clk);
	if (ret) {
		kfree(osc);
		return ERR_PTR(ret);
	}

	return &osc->clk;
}

static void
of_at91sam9x5_clk_slow_rc_osc_setup(struct device_node *np, void __iomem *sckcr)
{
	struct clk *clk;
	u32 frequency = 0;
	u32 startup = 0;
	const char *name = np->name;

	of_property_read_string(np, "clock-output-names", &name);
	of_property_read_u32(np, "clock-frequency", &frequency);
	of_property_read_u32(np, "atmel,startup-time-usec", &startup);

	clk = at91_clk_register_slow_rc_osc(sckcr, name, frequency, startup);
	if (IS_ERR(clk))
		return;

	of_clk_add_provider(np, of_clk_src_simple_get, clk);
}

static int clk_sam9x5_slow_set_parent(struct clk *clk, u8 index)
{
	struct clk_sam9x5_slow *slowck = to_clk_sam9x5_slow(clk);
	void __iomem *sckcr = slowck->sckcr;
	u32 tmp;

	if (index > 1)
		return -EINVAL;

	tmp = readl(sckcr);

	if ((!index && !(tmp & AT91_SCKC_OSCSEL)) ||
	    (index && (tmp & AT91_SCKC_OSCSEL)))
		return 0;

	if (index)
		tmp |= AT91_SCKC_OSCSEL;
	else
		tmp &= ~AT91_SCKC_OSCSEL;

	writel(tmp, sckcr);

	udelay(SLOWCK_SW_TIME_USEC);

	return 0;
}

static int clk_sam9x5_slow_get_parent(struct clk *clk)
{
	struct clk_sam9x5_slow *slowck = to_clk_sam9x5_slow(clk);

	return !!(readl(slowck->sckcr) & AT91_SCKC_OSCSEL);
}

static const struct clk_ops sam9x5_slow_ops = {
	.set_parent = clk_sam9x5_slow_set_parent,
	.get_parent = clk_sam9x5_slow_get_parent,
};

static struct clk *
at91_clk_register_sam9x5_slow(void __iomem *sckcr,
			      const char *name,
			      const char **parent_names,
			      int num_parents)
{
	struct clk_sam9x5_slow *slowck;
	int ret;

	if (!sckcr || !name || !parent_names || !num_parents)
		return ERR_PTR(-EINVAL);

	slowck = xzalloc(sizeof(*slowck));
	slowck->clk.name = name;
	slowck->clk.ops = &sam9x5_slow_ops;

	memcpy(slowck->parent_names, parent_names,
	       num_parents * sizeof(slowck->parent_names[0]));
	slowck->clk.parent_names = slowck->parent_names;
	slowck->clk.num_parents = num_parents;
	slowck->sckcr = sckcr;
	slowck->parent = !!(readl(sckcr) & AT91_SCKC_OSCSEL);

	ret = clk_register(&slowck->clk);
	if (ret) {
		kfree(slowck);
		return ERR_PTR(ret);
	}

	return &slowck->clk;
}

static int
of_at91sam9x5_clk_slow_setup(struct device_node *np, void __iomem *sckcr)
{
	struct clk *clk;
	const char *parent_names[2];
	unsigned int num_parents;
	const char *name = np->name;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > 2)
		return -EINVAL;

	of_clk_parent_fill(np, parent_names, num_parents);

	of_property_read_string(np, "clock-output-names", &name);

	clk = at91_clk_register_sam9x5_slow(sckcr, name, parent_names,
					    num_parents);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}

static const struct of_device_id sckc_clk_ids[] = {
	/* Slow clock */
	{
		.compatible = "atmel,at91sam9x5-clk-slow-osc",
		.data = of_at91sam9x5_clk_slow_osc_setup,
	},
	{
		.compatible = "atmel,at91sam9x5-clk-slow-rc-osc",
		.data = of_at91sam9x5_clk_slow_rc_osc_setup,
	},
	{
		.compatible = "atmel,at91sam9x5-clk-slow",
		.data = of_at91sam9x5_clk_slow_setup,
	},
	{ /*sentinel*/ }
};

static int of_at91sam9x5_sckc_setup(struct device_node *np)
{
	struct device_node *childnp;
	void (*clk_setup)(struct device_node *, void __iomem *);
	const struct of_device_id *clk_id;
	void __iomem *regbase = of_iomap(np, 0);

	if (!regbase)
		return -ENOMEM;

	for_each_child_of_node(np, childnp) {
		clk_id = of_match_node(sckc_clk_ids, childnp);
		if (!clk_id)
			continue;
		clk_setup = clk_id->data;
		clk_setup(childnp, regbase);
	}

	return 0;
}
CLK_OF_DECLARE(at91sam9x5_clk_sckc, "atmel,at91sam9x5-sckc",
	       of_at91sam9x5_sckc_setup);

static int clk_sama5d4_slow_osc_enable(struct clk *clk)
{
	struct clk_sama5d4_slow_osc *osc = to_clk_sama5d4_slow_osc(clk);

	if (osc->prepared)
		return 0;

	/*
	 * Assume that if it has already been selected (for example by the
	 * bootloader), enough time has aready passed.
	 */
	if ((readl(osc->sckcr) & AT91_SCKC_OSCSEL)) {
		osc->prepared = true;
		return 0;
	}

	udelay(osc->startup_usec);
	osc->prepared = true;

	return 0;
}

static int clk_sama5d4_slow_osc_is_enabled(struct clk *clk)
{
	struct clk_sama5d4_slow_osc *osc = to_clk_sama5d4_slow_osc(clk);

	return osc->prepared;
}

static const struct clk_ops sama5d4_slow_osc_ops = {
	.enable = clk_sama5d4_slow_osc_enable,
	.is_enabled = clk_sama5d4_slow_osc_is_enabled,
};

static int of_sama5d4_sckc_setup(struct device_node *np)
{
	void __iomem *regbase = of_iomap(np, 0);
	struct clk *clk;
	struct clk_sama5d4_slow_osc *osc;
	const char *parent_names[2] = { "slow_rc_osc", "slow_osc" };
	bool bypass;
	int ret;

	if (!regbase)
		return -ENOMEM;

	clk = clk_fixed(parent_names[0], 32768);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	bypass = of_property_read_bool(np, "atmel,osc-bypass");

	osc = xzalloc(sizeof(*osc));
	osc->parent_name = of_clk_get_parent_name(np, 0);
	osc->clk.name = parent_names[1];
	osc->clk.ops = &sama5d4_slow_osc_ops;
	osc->clk.parent_names = &osc->parent_name;
	osc->clk.num_parents = 1;
	osc->sckcr = regbase;
	osc->startup_usec = 1200000;

	if (bypass)
		writel((readl(regbase) | AT91_SCKC_OSC32BYP), regbase);

	ret = clk_register(&osc->clk);
	if (ret) {
		kfree(osc);
		return ret;
	}

	clk = at91_clk_register_sam9x5_slow(regbase, "slowck", parent_names, 2);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(sama5d4_clk_sckc, "atmel,sama5d4-sckc",
	       of_sama5d4_sckc_setup);

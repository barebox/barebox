/*
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
#include <io.h>
#include <linux/list.h>
#include <linux/clk.h>
#include <linux/clk/at91_pmc.h>
#include <mfd/syscon.h>
#include <regmap.h>

#include "pmc.h"

#define USB_SOURCE_MAX		2

#define SAM9X5_USB_DIV_SHIFT	8
#define SAM9X5_USB_MAX_DIV	0xf

#define RM9200_USB_DIV_SHIFT	28
#define RM9200_USB_DIV_TAB_SIZE	4

struct at91sam9x5_clk_usb {
	struct clk clk;
	struct regmap *regmap;
	const char *parent_names[USB_SOURCE_MAX];
};

#define to_at91sam9x5_clk_usb(clk) \
	container_of(clk, struct at91sam9x5_clk_usb, clk)

struct at91rm9200_clk_usb {
	struct clk clk;
	struct regmap *regmap;
	u32 divisors[4];
	const char *parent_name;
};

#define to_at91rm9200_clk_usb(clk) \
	container_of(clk, struct at91rm9200_clk_usb, clk)

static unsigned long at91sam9x5_clk_usb_recalc_rate(struct clk *clk,
						    unsigned long parent_rate)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);
	unsigned int usbr;
	u8 usbdiv;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);
	usbdiv = (usbr & AT91_PMC_OHCIUSBDIV) >> SAM9X5_USB_DIV_SHIFT;

	return DIV_ROUND_CLOSEST(parent_rate, (usbdiv + 1));
}

static int at91sam9x5_clk_usb_set_parent(struct clk *clk, u8 index)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);

	if (index > 1)
		return -EINVAL;

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_USBS,
			  index ? AT91_PMC_USBS : 0);

	return 0;
}

static int at91sam9x5_clk_usb_get_parent(struct clk *clk)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);
	unsigned int usbr;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);

	return usbr & AT91_PMC_USBS;
}

static int at91sam9x5_clk_usb_set_rate(struct clk *clk, unsigned long rate,
				       unsigned long parent_rate)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);
	unsigned long div;

	if (!rate)
		return -EINVAL;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);
	if (div > SAM9X5_USB_MAX_DIV + 1 || !div)
		return -EINVAL;

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_OHCIUSBDIV,
			  (div - 1) << SAM9X5_USB_DIV_SHIFT);

	return 0;
}

static const struct clk_ops at91sam9x5_usb_ops = {
	.recalc_rate = at91sam9x5_clk_usb_recalc_rate,
	.get_parent = at91sam9x5_clk_usb_get_parent,
	.set_parent = at91sam9x5_clk_usb_set_parent,
	.set_rate = at91sam9x5_clk_usb_set_rate,
};

static int at91sam9n12_clk_usb_enable(struct clk *clk)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_USBS,
			  AT91_PMC_USBS);

	return 0;
}

static void at91sam9n12_clk_usb_disable(struct clk *clk)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);

	regmap_write_bits(usb->regmap, AT91_PMC_USB, AT91_PMC_USBS, 0);
}

static int at91sam9n12_clk_usb_is_enabled(struct clk *clk)
{
	struct at91sam9x5_clk_usb *usb = to_at91sam9x5_clk_usb(clk);
	unsigned int usbr;

	regmap_read(usb->regmap, AT91_PMC_USB, &usbr);

	return usbr & AT91_PMC_USBS;
}

static const struct clk_ops at91sam9n12_usb_ops = {
	.enable = at91sam9n12_clk_usb_enable,
	.disable = at91sam9n12_clk_usb_disable,
	.is_enabled = at91sam9n12_clk_usb_is_enabled,
	.recalc_rate = at91sam9x5_clk_usb_recalc_rate,
	.set_rate = at91sam9x5_clk_usb_set_rate,
};

static struct clk *
at91sam9x5_clk_register_usb(struct regmap *regmap, const char *name,
			    const char **parent_names, u8 num_parents)
{
	struct at91sam9x5_clk_usb *usb;
	int ret;

	usb = kzalloc(sizeof(*usb), GFP_KERNEL);
	usb->clk.name = name;
	usb->clk.ops = &at91sam9x5_usb_ops;
	memcpy(usb->parent_names, parent_names,
	       num_parents * sizeof(usb->parent_names[0]));
	usb->clk.parent_names = usb->parent_names;
	usb->clk.num_parents = num_parents;
	usb->clk.flags = CLK_SET_RATE_PARENT;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_PARENT_GATE | */
	/* 	     CLK_SET_RATE_PARENT; */
	usb->regmap = regmap;

	ret = clk_register(&usb->clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->clk;
}

static struct clk *
at91sam9n12_clk_register_usb(struct regmap *regmap, const char *name,
			     const char *parent_name)
{
	struct at91sam9x5_clk_usb *usb;
	int ret;

	usb = xzalloc(sizeof(*usb));
	usb->clk.name = name;
	usb->clk.ops = &at91sam9n12_usb_ops;
	usb->parent_names[0] = parent_name;
	usb->clk.parent_names = &usb->parent_names[0];
	usb->clk.num_parents = 1;
	/* init.flags = CLK_SET_RATE_GATE | CLK_SET_RATE_PARENT; */
	usb->regmap = regmap;

	ret = clk_register(&usb->clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->clk;
}

static unsigned long at91rm9200_clk_usb_recalc_rate(struct clk *clk,
						    unsigned long parent_rate)
{
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(clk);
	unsigned int pllbr;
	u8 usbdiv;

	regmap_read(usb->regmap, AT91_CKGR_PLLBR, &pllbr);

	usbdiv = (pllbr & AT91_PMC_USBDIV) >> RM9200_USB_DIV_SHIFT;
	if (usb->divisors[usbdiv])
		return parent_rate / usb->divisors[usbdiv];

	return 0;
}

static long at91rm9200_clk_usb_round_rate(struct clk *clk, unsigned long rate,
					  unsigned long *parent_rate)
{
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(clk);
	struct clk *parent = clk_get_parent(clk);
	unsigned long bestrate = 0;
	int bestdiff = -1;
	unsigned long tmprate;
	int tmpdiff;
	int i = 0;

	for (i = 0; i < RM9200_USB_DIV_TAB_SIZE; i++) {
		unsigned long tmp_parent_rate;

		if (!usb->divisors[i])
			continue;

		tmp_parent_rate = rate * usb->divisors[i];
		tmp_parent_rate = clk_round_rate(parent, tmp_parent_rate);
		tmprate = DIV_ROUND_CLOSEST(tmp_parent_rate, usb->divisors[i]);
		if (tmprate < rate)
			tmpdiff = rate - tmprate;
		else
			tmpdiff = tmprate - rate;

		if (bestdiff < 0 || bestdiff > tmpdiff) {
			bestrate = tmprate;
			bestdiff = tmpdiff;
			*parent_rate = tmp_parent_rate;
		}

		if (!bestdiff)
			break;
	}

	return bestrate;
}

static int at91rm9200_clk_usb_set_rate(struct clk *clk, unsigned long rate,
				       unsigned long parent_rate)
{
	int i;
	struct at91rm9200_clk_usb *usb = to_at91rm9200_clk_usb(clk);
	unsigned long div;

	if (!rate)
		return -EINVAL;

	div = DIV_ROUND_CLOSEST(parent_rate, rate);

	for (i = 0; i < RM9200_USB_DIV_TAB_SIZE; i++) {
		if (usb->divisors[i] == div) {
			regmap_write_bits(usb->regmap, AT91_CKGR_PLLBR,
					  AT91_PMC_USBDIV,
					  i << RM9200_USB_DIV_SHIFT);

			return 0;
		}
	}

	return -EINVAL;
}

static const struct clk_ops at91rm9200_usb_ops = {
	.recalc_rate = at91rm9200_clk_usb_recalc_rate,
	.round_rate = at91rm9200_clk_usb_round_rate,
	.set_rate = at91rm9200_clk_usb_set_rate,
};

static struct clk *
at91rm9200_clk_register_usb(struct regmap *regmap, const char *name,
			    const char *parent_name, const u32 *divisors)
{
	struct at91rm9200_clk_usb *usb;
	int ret;

	usb = xzalloc(sizeof(*usb));
	usb->clk.name = name;
	usb->clk.ops = &at91rm9200_usb_ops;
	usb->parent_name = parent_name;
	usb->clk.parent_names = &usb->parent_name;
	usb->clk.num_parents = 1;
	/* init.flags = CLK_SET_RATE_PARENT; */

	usb->regmap = regmap;
	memcpy(usb->divisors, divisors, sizeof(usb->divisors));

	ret = clk_register(&usb->clk);
	if (ret) {
		kfree(usb);
		return ERR_PTR(ret);
	}

	return &usb->clk;
}

static int of_at91sam9x5_clk_usb_setup(struct device_node *np)
{
	struct clk *clk;
	unsigned int num_parents;
	const char *parent_names[USB_SOURCE_MAX];
	const char *name = np->name;
	struct regmap *regmap;

	num_parents = of_clk_get_parent_count(np);
	if (num_parents == 0 || num_parents > USB_SOURCE_MAX)
		return -EINVAL;

	of_clk_parent_fill(np, parent_names, num_parents);

	of_property_read_string(np, "clock-output-names", &name);

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk = at91sam9x5_clk_register_usb(regmap, name, parent_names,
					 num_parents);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(at91sam9x5_clk_usb, "atmel,at91sam9x5-clk-usb",
	       of_at91sam9x5_clk_usb_setup);

static int of_at91sam9n12_clk_usb_setup(struct device_node *np)
{
	struct clk *clk;
	const char *parent_name;
	const char *name = np->name;
	struct regmap *regmap;

	parent_name = of_clk_get_parent_name(np, 0);
	if (!parent_name)
		return -EINVAL;

	of_property_read_string(np, "clock-output-names", &name);

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk = at91sam9n12_clk_register_usb(regmap, name, parent_name);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(at91sam9n12_clk_usb, "atmel,at91sam9n12-clk-usb",
	       of_at91sam9n12_clk_usb_setup);

static int of_at91rm9200_clk_usb_setup(struct device_node *np)
{
	struct clk *clk;
	const char *parent_name;
	const char *name = np->name;
	u32 divisors[4] = {0, 0, 0, 0};
	struct regmap *regmap;

	parent_name = of_clk_get_parent_name(np, 0);
	if (!parent_name)
		return -EINVAL;

	of_property_read_u32_array(np, "atmel,clk-divisors", divisors, 4);
	if (!divisors[0])
		return -EINVAL;

	of_property_read_string(np, "clock-output-names", &name);

	regmap = syscon_node_to_regmap(of_get_parent(np));
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	clk = at91rm9200_clk_register_usb(regmap, name, parent_name, divisors);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(np, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(at91rm9200_clk_usb, "atmel,at91rm9200-clk-usb",
	       of_at91rm9200_clk_usb_setup);

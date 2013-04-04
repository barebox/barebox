/*
 * drivers/clk/lookup_clkdev.c
 *
 *  Copyright (C) 2008 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Helper for the clk API to assist looking up a struct clk.
 */

#include <common.h>
#include <linux/list.h>
#include <errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <init.h>
#include <malloc.h>
#include <stdio.h>

static LIST_HEAD(clocks);

/*
 * Find the correct struct clk for the device and connection ID.
 * We do slightly fuzzy matching here:
 *  An entry with a NULL ID is assumed to be a wildcard.
 *  If an entry has a device ID, it must match
 *  If an entry has a connection ID, it must match
 * Then we take the most specific entry - with the following
 * order of precedence: dev+con > dev only > con only.
 */
static struct clk *clk_find(const char *dev_id, const char *con_id)
{
	struct clk_lookup *p;
	struct clk *clk = NULL;
	int match, best = 0;

	list_for_each_entry(p, &clocks, node) {
		match = 0;
		if (p->dev_id) {
			if (!dev_id || strcmp(p->dev_id, dev_id))
				continue;
			match += 2;
		}
		if (p->con_id) {
			if (!con_id || strcmp(p->con_id, con_id))
				continue;
			match += 1;
		}

		if (match > best) {
			clk = p->clk;
			if (match != 3)
				best = match;
			else
				break;
		}
	}
	return clk;
}

static struct clk *clk_find_physbase(struct device_d *dev, const char *con_id)
{
	struct clk_lookup *p;
	unsigned long start;
	struct clk *clk = ERR_PTR(-ENOSYS);

	if (!dev || !dev->resource)
		return ERR_PTR(-ENOSYS);

	start = dev->resource[0].start;

	list_for_each_entry(p, &clocks, node) {
		if (p->physbase == ~0)
			continue;
		if (p->physbase != start)
			continue;
		if (p->con_id) {
			if (!con_id || strcmp(p->con_id, con_id))
				continue;
			return p->clk;
		}
		clk = p->clk;
	}

	return clk;

}

struct clk *clk_get_sys(const char *dev_id, const char *con_id)
{
	struct clk *clk;

	clk = clk_find(dev_id, con_id);

	return clk ? clk : ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(clk_get_sys);

struct clk *clk_get(struct device_d *dev, const char *con_id)
{
	const char *dev_id = dev ? dev_name(dev) : NULL;
	struct clk *clk;

	clk = clk_find_physbase(dev, con_id);
	if (!IS_ERR(clk))
		return clk;

	return clk_get_sys(dev_id, con_id);
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);

void clkdev_add(struct clk_lookup *cl)
{
	if (cl->dev_id)
		cl->physbase = ~0;

	list_add_tail(&cl->node, &clocks);
}
EXPORT_SYMBOL(clkdev_add);

void __init clkdev_add_table(struct clk_lookup *cl, size_t num)
{
	while (num--) {
		if (cl->dev_id)
			cl->physbase = ~0;
		list_add_tail(&cl->node, &clocks);
		cl++;
	}
}

#define MAX_DEV_ID	20
#define MAX_CON_ID	16

struct clk_lookup_alloc {
	struct clk_lookup cl;
	char	dev_id[MAX_DEV_ID];
	char	con_id[MAX_CON_ID];
};

struct clk_lookup *clkdev_alloc(struct clk *clk, const char *con_id,
	const char *dev_fmt, ...)
{
	struct clk_lookup_alloc *cla;

	cla = kzalloc(sizeof(*cla), GFP_KERNEL);
	if (!cla)
		return NULL;

	cla->cl.physbase = ~0;
	cla->cl.clk = clk;
	if (con_id) {
		strlcpy(cla->con_id, con_id, sizeof(cla->con_id));
		cla->cl.con_id = cla->con_id;
	}

	if (dev_fmt) {
		va_list ap;

		va_start(ap, dev_fmt);
		vscnprintf(cla->dev_id, sizeof(cla->dev_id), dev_fmt, ap);
		cla->cl.dev_id = cla->dev_id;
		va_end(ap);
	}

	return &cla->cl;
}
EXPORT_SYMBOL(clkdev_alloc);

int clk_register_clkdev(struct clk *clk, const char *con_id,
	const char *dev_fmt, ...)
{
	struct clk_lookup *cl;
	va_list ap;

	if (IS_ERR(clk))
                return PTR_ERR(clk);

	va_start(ap, dev_fmt);
	cl = clkdev_alloc(clk, con_id, dev_fmt, ap);
	va_end(ap);

	if (!cl)
		return -ENOMEM;

	clkdev_add(cl);

	return 0;
}

int clk_add_alias(const char *alias, const char *alias_dev_name, char *id,
	struct device_d *dev)
{
	struct clk *r = clk_get(dev, id);
	struct clk_lookup *l;

	if (IS_ERR(r))
		return PTR_ERR(r);

	l = clkdev_alloc(r, alias, alias_dev_name);
	clk_put(r);
	if (!l)
		return -ENODEV;
	clkdev_add(l);
	return 0;
}
EXPORT_SYMBOL(clk_add_alias);

/*
 * clkdev_drop - remove a clock dynamically allocated
 */
void clkdev_drop(struct clk_lookup *cl)
{
	list_del(&cl->node);
	kfree(cl);
}
EXPORT_SYMBOL(clkdev_drop);

int clkdev_add_physbase(struct clk *clk, unsigned long base, const char *id)
{
	struct clk_lookup *cl;

	cl = xzalloc(sizeof(*cl));

	cl->clk = clk;
	cl->con_id = id;
	cl->physbase = base;

	clkdev_add(cl);

	return 0;
}
EXPORT_SYMBOL(clkdev_add_physbase);

/*
 * clk.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <errno.h>
#include <linux/clk.h>
#include <linux/err.h>

static LIST_HEAD(clks);

static int clk_parent_enable(struct clk *clk)
{
	struct clk *parent = clk_get_parent(clk);

	if (!IS_ERR_OR_NULL(parent))
		return clk_enable(parent);

	return 0;
}

static void clk_parent_disable(struct clk *clk)
{
	struct clk *parent = clk_get_parent(clk);

	if (!IS_ERR_OR_NULL(parent))
		clk_disable(parent);
}

int clk_enable(struct clk *clk)
{
	int ret;

	if (!clk->enable_count) {
		ret = clk_parent_enable(clk);
		if (ret)
			return ret;

		if (clk->ops->enable) {
			ret = clk->ops->enable(clk);
			if (ret) {
				clk_parent_disable(clk);
				return ret;
			}
		}
	}

	clk->enable_count++;

	return 0;
}

void clk_disable(struct clk *clk)
{
	if (!clk->enable_count)
		return;

	clk->enable_count--;

	if (!clk->enable_count) {
		if (clk->ops->disable)
			clk->ops->disable(clk);

		clk_parent_disable(clk);
	}
}

unsigned long clk_get_rate(struct clk *clk)
{
	struct clk *parent;
	unsigned long parent_rate = 0;

	parent = clk_get_parent(clk);
	if (!IS_ERR_OR_NULL(parent))
		parent_rate = clk_get_rate(parent);

	if (clk->ops->recalc_rate)
		return clk->ops->recalc_rate(clk, parent_rate);

	return parent_rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	return clk_get_rate(clk);
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *parent;
	unsigned long parent_rate = 0;

	parent = clk_get_parent(clk);
	if (parent)
		parent_rate = clk_get_rate(parent);

	if (clk->ops->set_rate)
		return clk->ops->set_rate(clk, rate, parent_rate);

	return -ENOSYS;
}

struct clk *clk_lookup(const char *name)
{
	struct clk *c;

	if (!name)
		return ERR_PTR(-ENODEV);

	list_for_each_entry(c, &clks, list) {
		if (!strcmp(c->name, name))
			return c;
	}

	return ERR_PTR(-ENODEV);
}

int clk_set_parent(struct clk *clk, struct clk *parent)
{
	int i;

	if (!clk->num_parents)
		return -EINVAL;
	if (!clk->ops->set_parent)
		return -EINVAL;

	for (i = 0; i < clk->num_parents; i++) {
		if (IS_ERR_OR_NULL(clk->parents[i]))
			clk->parents[i] = clk_lookup(clk->parent_names[i]);

		if (!IS_ERR_OR_NULL(clk->parents[i]))
			if (clk->parents[i] == parent)
				break;
	}

	if (i == clk->num_parents)
		return -EINVAL;

	return clk->ops->set_parent(clk, i);
}

struct clk *clk_get_parent(struct clk *clk)
{
	int idx;

	if (!clk->num_parents)
		return ERR_PTR(-ENODEV);

	if (clk->num_parents != 1) {
		if (!clk->ops->get_parent)
			return ERR_PTR(-EINVAL);

		idx = clk->ops->get_parent(clk);

		if (idx >= clk->num_parents)
			return ERR_PTR(-ENODEV);
	} else {
		idx = 0;
	}

	if (IS_ERR_OR_NULL(clk->parents[idx]))
		clk->parents[idx] = clk_lookup(clk->parent_names[idx]);

	return clk->parents[idx];
}

int clk_register(struct clk *clk)
{
	clk->parents = xzalloc(sizeof(struct clk *) * clk->num_parents);

	list_add_tail(&clk->list, &clks);

	return 0;
}

static void dump_one(struct clk *clk, int verbose, int indent)
{
	struct clk *c;

	printf("%*s%s (rate %ld, %sabled)\n", indent * 4, "", clk->name, clk_get_rate(clk),
			clk->enable_count ? "en" : "dis");
	if (verbose) {

		if (clk->num_parents > 1) {
			int i;
			printf("%*s`---- possible parents: ", indent * 4, "");
			for (i = 0; i < clk->num_parents; i++)
				printf("%s ", clk->parent_names[i]);
			printf("\n");
		}
	}

	list_for_each_entry(c, &clks, list) {
		struct clk *parent = clk_get_parent(c);

		if (parent == clk) {
			dump_one(c, verbose, indent + 1);
		}
	}
}

void clk_dump(int verbose)
{
	struct clk *c;

	list_for_each_entry(c, &clks, list) {
		struct clk *parent = clk_get_parent(c);

		if (IS_ERR_OR_NULL(parent))
			dump_one(c, verbose, 0);
	}
}

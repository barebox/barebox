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
#include <malloc.h>
#include <stringlist.h>
#include <complete.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clk/clk-conf.h>
#include <pinctrl.h>

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

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return PTR_ERR(clk);

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
	if (!clk)
		return;

	if (IS_ERR(clk))
		return;

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

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return 0;

	parent = clk_get_parent(clk);


	if (!IS_ERR_OR_NULL(parent))
		parent_rate = clk_get_rate(parent);

	if (clk->ops->recalc_rate)
		return clk->ops->recalc_rate(clk, parent_rate);

	return parent_rate;
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	unsigned long parent_rate = 0;
	struct clk *parent;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return 0;

	parent = clk_get_parent(clk);
	if (parent)
		parent_rate = clk_get_rate(parent);

	if (clk->ops->round_rate)
		return clk->ops->round_rate(clk, rate, &parent_rate);

	return clk_get_rate(clk);
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *parent;
	unsigned long parent_rate = 0;
	int ret;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return PTR_ERR(clk);

	if (!clk->ops->set_rate)
		return -ENOSYS;

	parent = clk_get_parent(clk);
	if (parent) {
		parent_rate = clk_get_rate(parent);

		if (clk->flags & CLK_OPS_PARENT_ENABLE) {
			ret = clk_enable(parent);
			if (ret)
				return ret;
		}
	}

	ret = clk->ops->set_rate(clk, rate, parent_rate);

	if (parent && clk->flags & CLK_OPS_PARENT_ENABLE)
		clk_disable(parent);

	return ret;
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

int clk_set_parent(struct clk *clk, struct clk *newparent)
{
	int i, ret;
	struct clk *curparent = clk_get_parent(clk);

	if (IS_ERR(clk))
		return PTR_ERR(clk);
	if (IS_ERR(newparent))
		return PTR_ERR(newparent);

	if (!clk->num_parents)
		return -EINVAL;
	if (!clk->ops->set_parent)
		return -EINVAL;

	for (i = 0; i < clk->num_parents; i++) {
		if (IS_ERR_OR_NULL(clk->parents[i]))
			clk->parents[i] = clk_lookup(clk->parent_names[i]);

		if (!IS_ERR_OR_NULL(clk->parents[i]))
			if (clk->parents[i] == newparent)
				break;
	}

	if (i == clk->num_parents)
		return -EINVAL;

	if (clk->enable_count)
		clk_enable(newparent);

	if (clk->flags & CLK_OPS_PARENT_ENABLE) {
		clk_enable(curparent);
		clk_enable(newparent);
	}

	ret = clk->ops->set_parent(clk, i);

	if (clk->flags & CLK_OPS_PARENT_ENABLE) {
		clk_disable(curparent);
		clk_disable(newparent);
	}

	if (clk->enable_count)
		clk_disable(curparent);

	return ret;
}

struct clk *clk_get_parent(struct clk *clk)
{
	int idx;

	if (IS_ERR(clk))
		return clk;

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
	struct clk *c;

	list_for_each_entry(c, &clks, list) {
		if (!strcmp(c->name, clk->name)) {
			pr_err("%s clk %s is already registered, skipping!\n",
				__func__, clk->name);
			return -EBUSY;
		}
	}

	clk->parents = xzalloc(sizeof(struct clk *) * clk->num_parents);

	list_add_tail(&clk->list, &clks);

	return 0;
}

int clk_is_enabled(struct clk *clk)
{
	int enabled;

	if (IS_ERR(clk))
		return 0;

	if (clk->ops->is_enabled) {
		/*
		 * If we can ask a clk, do it
		 */
		enabled = clk->ops->is_enabled(clk);
	} else {
		if (clk->ops->enable) {
			/*
			 * If we can't ask a clk, but it can be enabled,
			 * depend on the enable_count.
			 */
			enabled = clk->enable_count;
		} else {
			/*
			 * We can't ask a clk, it has no enable op,
			 * so assume it's enabled and go on and ask
			 * the parent.
			 */
			enabled = 1;
		}
	}

	if (!enabled)
		return 0;

	clk = clk_get_parent(clk);

	if (IS_ERR(clk))
		return 1;

	return clk_is_enabled(clk);
}

/*
 * Generic struct clk_ops callbacks
 */
int clk_is_enabled_always(struct clk *clk)
{
	return 1;
}

long clk_parent_round_rate(struct clk *clk, unsigned long rate,
				unsigned long *prate)
{
	if (!(clk->flags & CLK_SET_RATE_PARENT))
		return *prate;

	return clk_round_rate(clk_get_parent(clk), rate);
}

int clk_parent_set_rate(struct clk *clk, unsigned long rate,
				unsigned long parent_rate)
{
	if (!(clk->flags & CLK_SET_RATE_PARENT))
		return 0;
	return clk_set_rate(clk_get_parent(clk), rate);
}

#if defined(CONFIG_COMMON_CLK_OF_PROVIDER)
/**
 * struct of_clk_provider - Clock provider registration structure
 * @link: Entry in global list of clock providers
 * @node: Pointer to device tree node of clock provider
 * @get: Get clock callback.  Returns NULL or a struct clk for the
 *       given clock specifier
 * @data: context pointer to be passed into @get callback
 */
struct of_clk_provider {
	struct list_head link;

	struct device_node *node;
	struct clk *(*get)(struct of_phandle_args *clkspec, void *data);
	void *data;
};

extern struct of_device_id __clk_of_table_start[];
const struct of_device_id __clk_of_table_sentinel
	__attribute__ ((unused,section (".__clk_of_table_end")));

static LIST_HEAD(of_clk_providers);

struct clk *of_clk_src_simple_get(struct of_phandle_args *clkspec,
		void *data)
{
	return data;
}
EXPORT_SYMBOL_GPL(of_clk_src_simple_get);

struct clk *of_clk_src_onecell_get(struct of_phandle_args *clkspec, void *data)
{
	struct clk_onecell_data *clk_data = data;
	unsigned int idx = clkspec->args[0];

	if (idx >= clk_data->clk_num) {
		pr_err("%s: invalid clock index %d\n", __func__, idx);
		return ERR_PTR(-EINVAL);
	}

	return clk_data->clks[idx];
}
EXPORT_SYMBOL_GPL(of_clk_src_onecell_get);

/**
 * of_clk_add_provider() - Register a clock provider for a node
 * @np: Device node pointer associated with clock provider
 * @clk_src_get: callback for decoding clock
 * @data: context pointer for @clk_src_get callback.
 */
int of_clk_add_provider(struct device_node *np,
			struct clk *(*clk_src_get)(struct of_phandle_args *clkspec,
						   void *data),
			void *data)
{
	struct of_clk_provider *cp;

	cp = kzalloc(sizeof(struct of_clk_provider), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;

	cp->node = np;
	cp->data = data;
	cp->get = clk_src_get;

	list_add(&cp->link, &of_clk_providers);
	pr_debug("Added clock from %s\n", np->full_name);

	return 0;
}
EXPORT_SYMBOL_GPL(of_clk_add_provider);

/**
 * of_clk_del_provider() - Remove a previously registered clock provider
 * @np: Device node pointer associated with clock provider
 */
void of_clk_del_provider(struct device_node *np)
{
	struct of_clk_provider *cp;

	list_for_each_entry(cp, &of_clk_providers, link) {
		if (cp->node == np) {
			list_del(&cp->link);
			kfree(cp);
			break;
		}
	}
}
EXPORT_SYMBOL_GPL(of_clk_del_provider);

struct clk *of_clk_get_from_provider(struct of_phandle_args *clkspec)
{
	struct of_clk_provider *provider;
	struct clk *clk = ERR_PTR(-EPROBE_DEFER);

	/* Check if we have such a provider in our array */
	list_for_each_entry(provider, &of_clk_providers, link) {
		if (provider->node == clkspec->np)
			clk = provider->get(clkspec, provider->data);
		if (!IS_ERR(clk))
			break;
	}

	return clk;
}

/**
 * of_clk_get_parent_count() - Count the number of clocks a device node has
 * @np: device node to count
 *
 * Returns: The number of clocks that are possible parents of this node
 */
unsigned int of_clk_get_parent_count(struct device_node *np)
{
	int count;

	count = of_count_phandle_with_args(np, "clocks", "#clock-cells");
	if (count < 0)
		return 0;

	return count;
}
EXPORT_SYMBOL_GPL(of_clk_get_parent_count);

char *of_clk_get_parent_name(struct device_node *np, unsigned int index)
{
	struct of_phandle_args clkspec;
	const char *clk_name;
	int rc;

	rc = of_parse_phandle_with_args(np, "clocks", "#clock-cells", index,
			&clkspec);
	if (rc)
		return NULL;

	if (of_property_read_string_index(clkspec.np, "clock-output-names",
				clkspec.args_count ? clkspec.args[0] : 0,
				&clk_name) < 0)
		clk_name = clkspec.np->name;

	return xstrdup(clk_name);
}
EXPORT_SYMBOL_GPL(of_clk_get_parent_name);

/**
 * of_clk_parent_fill() - Fill @parents with names of @np's parents and return
 * number of parents
 * @np: Device node pointer associated with clock provider
 * @parents: pointer to char array that hold the parents' names
 * @size: size of the @parents array
 *
 * Return: number of parents for the clock node.
 */
int of_clk_parent_fill(struct device_node *np, const char **parents,
		       unsigned int size)
{
	unsigned int i = 0;

	while (i < size && (parents[i] = of_clk_get_parent_name(np, i)) != NULL)
		i++;

	return i;
}
EXPORT_SYMBOL_GPL(of_clk_parent_fill);

struct clock_provider {
	of_clk_init_cb_t clk_init_cb;
	struct device_node *np;
	struct list_head node;
};

/*
 * This function looks for a parent clock. If there is one, then it
 * checks that the provider for this parent clock was initialized, in
 * this case the parent clock will be ready.
 */
static int parent_ready(struct device_node *np)
{
	int i = 0;

	while (true) {
		struct clk *clk = of_clk_get(np, i);

		/* this parent is ready we can check the next one */
		if (!IS_ERR(clk)) {
			clk_put(clk);
			i++;
			continue;
		}

		/* at least one parent is not ready, we exit now */
		if (PTR_ERR(clk) == -EPROBE_DEFER)
			return 0;

		/*
		 * Here we make assumption that the device tree is
		 * written correctly. So an error means that there is
		 * no more parent. As we didn't exit yet, then the
		 * previous parent are ready. If there is no clock
		 * parent, no need to wait for them, then we can
		 * consider their absence as being ready
		 */
		return 1;
	}
}

/**
 * of_clk_init() - Scan and init clock providers from the DT
 * @root: parent of the first level to probe or NULL for the root of the tree
 * @matches: array of compatible values and init functions for providers.
 *
 * This function scans the device tree for matching clock providers and
 * calls their initialization functions
 *
 * Returns 0 on success, < 0 on failure.
 */
int of_clk_init(struct device_node *root, const struct of_device_id *matches)
{
	struct clock_provider *clk_provider, *next;
	bool is_init_done;
	bool force = false;
	LIST_HEAD(clk_provider_list);
	const struct of_device_id *match;

	if (!root)
		root = of_find_node_by_path("/");
	if (!root)
		return -EINVAL;
	if (!matches)
		matches = __clk_of_table_start;

	/* First prepare the list of the clocks providers */
	for_each_matching_node_and_match(root, matches, &match) {
		struct clock_provider *parent;

		if (!of_device_is_available(root))
			continue;

		parent = xzalloc(sizeof(*parent));

		parent->clk_init_cb = match->data;
		parent->np = root;
		list_add_tail(&parent->node, &clk_provider_list);
	}

	while (!list_empty(&clk_provider_list)) {
		is_init_done = false;
		list_for_each_entry_safe(clk_provider, next,
					 &clk_provider_list, node) {

			struct device_node *np = clk_provider->np;
			if (force || parent_ready(np)) {

				of_pinctrl_select_state_default(np);
				clk_provider->clk_init_cb(np);
				of_clk_set_defaults(np, true);

				list_del(&clk_provider->node);
				free(clk_provider);
				is_init_done = true;
			}
		}

		/*
		 * We didn't manage to initialize any of the
		 * remaining providers during the last loop, so now we
		 * initialize all the remaining ones unconditionally
		 * in case the clock parent was not mandatory
		 */
		if (!is_init_done)
			force = true;
	}

	return 0;
}
#endif

static void dump_one(struct clk *clk, int verbose, int indent)
{
	struct clk *c;

	printf("%*s%s (rate %lu, %sabled)\n", indent * 4, "", clk->name, clk_get_rate(clk),
			clk_is_enabled(clk) ? "en" : "dis");
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

int clk_name_complete(struct string_list *sl, char *instr)
{
	struct clk *c;
	int len;

	if (!instr)
		instr = "";

	len = strlen(instr);

	list_for_each_entry(c, &clks, list) {
		if (strncmp(instr, c->name, len))
			continue;

		string_list_add_asprintf(sl, "%s ", c->name);
	}

	return COMPLETE_CONTINUE;
}

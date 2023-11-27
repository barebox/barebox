// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * clk.c - generic barebox clock support. Based on Linux clk support
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
	struct clk_hw *hw;
	int ret;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return PTR_ERR(clk);

	hw = clk_to_clk_hw(clk);

	if (!clk->enable_count) {
		ret = clk_parent_enable(clk);
		if (ret)
			return ret;

		if (clk->ops->enable) {
			ret = clk->ops->enable(hw);
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
	struct clk_hw *hw;

	if (!clk)
		return;

	if (IS_ERR(clk))
		return;

	if (!clk->enable_count)
		return;

	if (clk->enable_count == 1 && clk->flags & CLK_IS_CRITICAL) {
		pr_warn("Disabling critical clock %s\n", clk->name);
		return;
	}

	clk->enable_count--;

	hw = clk_to_clk_hw(clk);

	if (!clk->enable_count) {
		if (clk->ops->disable)
			clk->ops->disable(hw);

		clk_parent_disable(clk);
	}
}

unsigned long clk_get_rate(struct clk *clk)
{
	struct clk_hw *hw;
	struct clk *parent;
	unsigned long parent_rate = 0;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return 0;

	parent = clk_get_parent(clk);


	if (!IS_ERR_OR_NULL(parent))
		parent_rate = clk_get_rate(parent);

	hw = clk_to_clk_hw(clk);

	if (clk->ops->recalc_rate)
		return clk->ops->recalc_rate(hw, parent_rate);

	return parent_rate;
}

unsigned long clk_hw_get_rate(struct clk_hw *hw)
{
	return clk_get_rate(clk_hw_to_clk(hw));
}

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	struct clk_hw *hw;
	unsigned long parent_rate = 0;
	struct clk *parent;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return 0;

	parent = clk_get_parent(clk);
	if (parent)
		parent_rate = clk_get_rate(parent);

	hw = clk_to_clk_hw(clk);

	if (clk->ops->round_rate)
		return clk->ops->round_rate(hw, rate, &parent_rate);

	return clk_get_rate(clk);
}

long clk_hw_round_rate(struct clk_hw *hw, unsigned long rate)
{
	return clk_round_rate(&hw->clk, rate);
}

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk_hw *hw;
	struct clk *parent;
	unsigned long parent_rate = 0;
	int ret;

	if (!clk)
		return 0;

	if (IS_ERR(clk))
		return PTR_ERR(clk);

	if (clk_get_rate(clk) == clk_round_rate(clk, rate))
		return 0;

	if (!clk->ops->set_rate)
		return -ENOSYS;

	if (clk->flags & CLK_SET_RATE_UNGATE) {
		ret = clk_enable(clk);
		if (ret)
			return ret;
	}

	parent = clk_get_parent(clk);
	if (parent) {
		parent_rate = clk_get_rate(parent);

		if (clk->flags & CLK_OPS_PARENT_ENABLE) {
			ret = clk_enable(parent);
			if (ret)
				goto out;
		}
	}

	hw = clk_to_clk_hw(clk);

	ret = clk->ops->set_rate(hw, rate, parent_rate);

	if (parent && clk->flags & CLK_OPS_PARENT_ENABLE)
		clk_disable(parent);

out:
	if (clk->flags & CLK_SET_RATE_UNGATE)
		clk_disable(clk);

	return ret;
}

int clk_hw_set_rate(struct clk_hw *hw, unsigned long rate)
{
	return clk_set_rate(&hw->clk, rate);
}

static int clk_fetch_parent_index(struct clk *clk,
				  struct clk *parent)
{
	int i;

	if (!parent)
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

	return i;
}

/**
 * clk_hw_get_parent_index - return the index of the parent clock
 * @hw: clk_hw associated with the clk being consumed
 *
 * Fetches and returns the index of parent clock. Returns -EINVAL if the given
 * clock does not have a current parent.
 */
int clk_hw_get_parent_index(struct clk_hw *hw)
{
	struct clk_hw *parent = clk_hw_get_parent(hw);

	if (WARN_ON(parent == NULL))
		return -EINVAL;

	return clk_fetch_parent_index(clk_hw_to_clk(hw), clk_hw_to_clk(parent));
}
EXPORT_SYMBOL_GPL(clk_hw_get_parent_index);

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
	struct clk_hw *hw;
	int i, ret;
	struct clk *curparent = clk_get_parent(clk);

	if (!clk || !newparent)
		return 0;

	if (IS_ERR(clk))
		return PTR_ERR(clk);
	if (IS_ERR(newparent))
		return PTR_ERR(newparent);

	if (!clk->num_parents)
		return -EINVAL;
	if (!clk->ops->set_parent)
		return -EINVAL;

	i = clk_fetch_parent_index(clk, newparent);
	if (i < 0)
		return i;

	if (clk->enable_count)
		clk_enable(newparent);

	if (clk->flags & CLK_OPS_PARENT_ENABLE) {
		clk_enable(curparent);
		clk_enable(newparent);
	}

	hw = clk_to_clk_hw(clk);

	ret = clk->ops->set_parent(hw, i);

	if (clk->flags & CLK_OPS_PARENT_ENABLE) {
		clk_disable(curparent);
		clk_disable(newparent);
	}

	if (clk->enable_count)
		clk_disable(curparent);

	return ret;
}

int clk_hw_set_parent(struct clk_hw *hw, struct clk_hw *newparent)
{
	return clk_set_parent(&hw->clk, &newparent->clk);
}

static struct clk *clk_get_parent_by_index(struct clk *clk, u8 idx)
{
	if (IS_ERR_OR_NULL(clk->parents[idx]))
		clk->parents[idx] = clk_lookup(clk->parent_names[idx]);

	return clk->parents[idx];
}

struct clk_hw *
clk_hw_get_parent_by_index(const struct clk_hw *hw, unsigned int idx)
{
	struct clk *clk = clk_hw_to_clk(hw);

	if (!clk || idx >= clk->num_parents || !clk->parents)
		return NULL;

	return clk_to_clk_hw(clk_get_parent_by_index(clk, idx));
}
EXPORT_SYMBOL_GPL(clk_hw_get_parent_by_index);

struct clk *clk_get_parent(struct clk *clk)
{
	struct clk_hw *hw;
	int idx;

	if (IS_ERR_OR_NULL(clk))
		return clk;

	if (!clk->num_parents)
		return ERR_PTR(-ENODEV);

	hw = clk_to_clk_hw(clk);

	if (clk->num_parents != 1) {
		if (!clk->ops->get_parent)
			return ERR_PTR(-EINVAL);

		idx = clk->ops->get_parent(hw);

		if (idx >= clk->num_parents)
			return ERR_PTR(-ENODEV);
	} else {
		idx = 0;
	}

	return clk_get_parent_by_index(clk, idx);
}

struct clk_hw *clk_hw_get_parent(struct clk_hw *hw)
{
	struct clk *clk = clk_get_parent(clk_hw_to_clk(hw));

	if (IS_ERR(clk))
		return ERR_CAST(clk);

	return clk_to_clk_hw(clk);
}

/**
 * clk_set_phase - adjust the phase shift of a clock signal
 * @clk: clock signal source
 * @degrees: number of degrees the signal is shifted
 *
 * Shifts the phase of a clock signal by the specified
 * degrees. Returns 0 on success, -EERROR otherwise.
 *
 * This function makes no distinction about the input or reference
 * signal that we adjust the clock signal phase against. For example
 * phase locked-loop clock signal generators we may shift phase with
 * respect to feedback clock signal input, but for other cases the
 * clock phase may be shifted with respect to some other, unspecified
 * signal.
 *
 * Additionally the concept of phase shift does not propagate through
 * the clock tree hierarchy, which sets it apart from clock rates and
 * clock accuracy. A parent clock phase attribute does not have an
 * impact on the phase attribute of a child clock.
 */
int clk_set_phase(struct clk *clk, int degrees)
{
	if (!clk)
		return 0;

	/* sanity check degrees */
	degrees %= 360;
	if (degrees < 0)
		degrees += 360;

	if (!clk->ops->set_phase)
		return -EINVAL;

	return clk->ops->set_phase(clk_to_clk_hw(clk), degrees);
}

/**
 * clk_get_phase - return the phase shift of a clock signal
 * @clk: clock signal source
 *
 * Returns the phase shift of a clock node in degrees, otherwise returns
 * -EERROR.
 */
int clk_get_phase(struct clk *clk)
{
	int ret;

	if (!clk->ops->get_phase)
		return 0;

	ret = clk->ops->get_phase(clk_to_clk_hw(clk));

	return ret;
}

int bclk_register(struct clk *clk)
{
	struct clk_hw *hw = clk_to_clk_hw(clk);
	struct clk *c;
	int ret;

	list_for_each_entry(c, &clks, list) {
		if (!strcmp(c->name, clk->name)) {
			pr_err("%s clk %s is already registered, skipping!\n",
				__func__, clk->name);
			return -EBUSY;
		}
	}

	clk->parents = xzalloc(sizeof(struct clk *) * clk->num_parents);

	list_add_tail(&clk->list, &clks);

	if (clk->ops->init) {
		ret = clk->ops->init(hw);
		if (ret)
			goto out;
	}

	if (clk->flags & CLK_IS_CRITICAL)
		clk_enable(clk);

	return 0;
out:
	list_del(&clk->list);
	free(clk->parents);

	return ret;
}

struct clk *clk_register(struct device *dev, struct clk_hw *hw)
{
	struct clk *clk;
	const struct clk_init_data *init = hw->init;
	char **parent_names;
	int i, ret;

	if (!hw->init)
		return ERR_PTR(-EINVAL);

	clk = clk_hw_to_clk(hw);

	memset(clk, 0, sizeof(*clk));

	clk->name = xstrdup(init->name);
	clk->ops = init->ops;
	clk->num_parents = init->num_parents;
	parent_names = xzalloc(init->num_parents * sizeof(char *));

	for (i = 0; i < init->num_parents; i++)
		parent_names[i] = xstrdup(init->parent_names[i]);

	clk->parent_names = (const char *const*)parent_names;

	clk->flags = init->flags;

	ret = bclk_register(clk);
	if (ret) {
		for (i = 0; i < init->num_parents; i++)
			free(parent_names[i]);
		free(parent_names);
		return ERR_PTR(ret);
	}

	return clk;
}

int clk_is_enabled(struct clk *clk)
{
	int enabled;
	struct clk_hw *hw = clk_to_clk_hw(clk);

	if (IS_ERR(clk))
		return 0;

	if (clk->ops->is_enabled) {
		/*
		 * If we can ask a clk, do it
		 */
		enabled = clk->ops->is_enabled(hw);
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

int clk_hw_is_enabled(struct clk_hw *hw)
{
	return clk_is_enabled(&hw->clk);
}

/*
 * Generic struct clk_ops callbacks
 */
int clk_is_enabled_always(struct clk_hw *hw)
{
	return 1;
}

long clk_parent_round_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long *prate)
{
	struct clk *clk = clk_hw_to_clk(hw);

	if (!(clk->flags & CLK_SET_RATE_PARENT))
		return *prate;

	return clk_round_rate(clk_get_parent(clk), rate);
}

int clk_parent_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct clk *clk = clk_hw_to_clk(hw);

	if (!(clk->flags & CLK_SET_RATE_PARENT))
		return 0;
	return clk_set_rate(clk_get_parent(clk), rate);
}

int clk_name_set_parent(const char *clkname, const char *clkparentname)
{
	struct clk *clk = clk_lookup(clkname);
	struct clk *parent = clk_lookup(clkparentname);

	if (IS_ERR(clk))
		return -ENOENT;
	if (IS_ERR(parent))
		return -ENOENT;
	return clk_set_parent(clk, parent);
}

int clk_name_set_rate(const char *clkname, unsigned long rate)
{
	struct clk *clk = clk_lookup(clkname);

	if (IS_ERR(clk))
		return -ENOENT;

	return clk_set_rate(clk, rate);
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
	struct clk_hw *(*get_hw)(struct of_phandle_args *clkspec, void *data);
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

struct clk_hw *of_clk_hw_simple_get(struct of_phandle_args *clkspec, void *data)
{
	return data;
}
EXPORT_SYMBOL_GPL(of_clk_hw_simple_get);

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

struct clk_hw *
of_clk_hw_onecell_get(struct of_phandle_args *clkspec, void *data)
{
	struct clk_hw_onecell_data *hw_data = data;
	unsigned int idx = clkspec->args[0];

	if (idx >= hw_data->num) {
		pr_err("%s: invalid index %u\n", __func__, idx);
		return ERR_PTR(-EINVAL);
	}

	return hw_data->hws[idx];
}
EXPORT_SYMBOL_GPL(of_clk_hw_onecell_get);


static int __of_clk_add_provider(struct device_node *np,
			struct clk *(*clk_src_get)(struct of_phandle_args *clkspec,
						   void *data),
			struct clk_hw *(*clk_hw_src_get)(struct of_phandle_args *clkspec,
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
	cp->get_hw = clk_hw_src_get;

	list_add(&cp->link, &of_clk_providers);
	pr_debug("Added clock from %pOF\n", np);

	of_clk_set_defaults(np, true);

	return 0;
}

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
	return __of_clk_add_provider(np, clk_src_get, NULL, data);
}
EXPORT_SYMBOL_GPL(of_clk_add_provider);

int of_clk_add_hw_provider(struct device_node *np,
			struct clk_hw *(*clk_hw_src_get)(struct of_phandle_args *clkspec,
							 void *data),
			void *data)
{
	return __of_clk_add_provider(np, NULL, clk_hw_src_get, data);
}
EXPORT_SYMBOL_GPL(of_clk_add_hw_provider);

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

	/* Ignore error, as CLK_OF_DECLARE clocks have no proper driver. */
	of_device_ensure_probed(clkspec->np);

	/* Check if we have such a provider in our array */
	list_for_each_entry(provider, &of_clk_providers, link) {
		if (provider->node == clkspec->np) {
			if (provider->get)
				clk = provider->get(clkspec, provider->data);
			else
				clk = clk_hw_to_clk(provider->get_hw(clkspec, provider->data));
		}
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

char *of_clk_get_parent_name(const struct device_node *np, int index)
{
	struct of_phandle_args clkspec;
	struct property *prop;
	const char *clk_name;
	const __be32 *vp;
	u32 pv;
	int rc;
	int count;
	struct clk *clk;

	rc = of_parse_phandle_with_args(np, "clocks", "#clock-cells", index,
					&clkspec);
	if (rc)
		return NULL;

	index = clkspec.args_count ? clkspec.args[0] : 0;
	count = 0;

	/* if there is an indices property, use it to transfer the index
	 * specified into an array offset for the clock-output-names property.
	 */
	of_property_for_each_u32(clkspec.np, "clock-indices", prop, vp, pv) {
		if (index == pv) {
			index = count;
			break;
		}
		count++;
	}
	/* We went off the end of 'clock-indices' without finding it */
	if (prop && !vp)
		return NULL;

	if (of_property_read_string_index(clkspec.np, "clock-output-names",
					  index,
					  &clk_name) < 0) {
		/*
		 * Best effort to get the name if the clock has been
		 * registered with the framework. If the clock isn't
		 * registered, we return the node name as the name of
		 * the clock as long as #clock-cells = 0.
		 */
		clk = of_clk_get_from_provider(&clkspec);
		if (IS_ERR(clk)) {
			if (clkspec.args_count == 0)
				clk_name = clkspec.np->name;
			else
				clk_name = NULL;
		} else {
			clk_name = __clk_get_name(clk);
			clk_put(clk);
		}
	}


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

static LIST_HEAD(probed_clks);

static bool of_clk_probed(struct device_node *np)
{
	struct clock_provider *clk_provider;

	list_for_each_entry(clk_provider, &probed_clks, node)
		if (clk_provider->np == np)
			return true;
	return false;
}

/**
 * of_clk_init() - Scan and init clock providers from the DT
 *
 * This function scans the device tree for matching clock providers and
 * calls their initialization functions
 *
 * Returns 0 on success, < 0 on failure.
 */
int of_clk_init(void)
{
	struct device_node *root = of_get_root_node();
	const struct of_device_id *matches = __clk_of_table_start;
	struct clock_provider *clk_provider, *next;
	bool is_init_done;
	bool force = false;
	LIST_HEAD(clk_provider_list);
	const struct of_device_id *match;

	if (!root)
		return -EINVAL;

	/* First prepare the list of the clocks providers */
	for_each_matching_node_and_match(root, matches, &match) {
		struct clock_provider *parent;

		if (!of_device_is_available(root))
			continue;

		if (of_clk_probed(root)) {
			pr_debug("%s: already probed: %pOF\n", __func__, root);
			continue;
		}

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

				list_move_tail(&clk_provider->node, &probed_clks);
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

static const char *clk_hw_stat(struct clk *clk)
{
	struct clk_hw *hw = clk_to_clk_hw(clk);

	if (clk->ops->is_enabled) {
		if (clk->ops->is_enabled(hw))
			return "enabled";
		else
			return "disabled";
	}

	if (!clk->ops->enable)
		return "always enabled";

	return "unknown";
}

static void dump_one(struct clk *clk, int verbose, int indent)
{
	int enabled = clk_is_enabled(clk);
	const char *hwstat, *stat;

	hwstat = clk_hw_stat(clk);

	if (enabled == 0)
		stat = "disabled";
	else
		stat = "enabled";

	printf("%*s%s (rate %lu, enable_count: %d, %s)\n", indent * 4, "",
	       clk->name,
	       clk_get_rate(clk),
	       clk->enable_count,
	       hwstat);

	if (verbose) {

		if (clk->num_parents > 1) {
			int i;
			printf("%*s`---- possible parents: ", indent * 4, "");
			for (i = 0; i < clk->num_parents; i++)
				printf("%s ", clk->parent_names[i]);
			printf("\n");
		}
	}
}

static void dump_subtree(struct clk *clk, int verbose, int indent)
{
	struct clk *c;

	dump_one(clk, verbose, indent);

	list_for_each_entry(c, &clks, list) {
		struct clk *parent = clk_get_parent(c);

		if (parent == clk)
			dump_subtree(c, verbose, indent + 1);
	}
}

void clk_dump(int verbose)
{
	struct clk *c;

	list_for_each_entry(c, &clks, list) {
		struct clk *parent = clk_get_parent(c);

		if (IS_ERR_OR_NULL(parent))
			dump_subtree(c, verbose, 0);
	}
}

static int clk_print_parent(struct clk *clk, int verbose)
{
	struct clk *c;
	int indent;

	c = clk_get_parent(clk);
	if (IS_ERR_OR_NULL(c))
		return 0;

	indent = clk_print_parent(c, verbose);

	dump_one(c, verbose, indent);

	return indent + 1;
}

void clk_dump_one(struct clk *clk, int verbose)
{
	int indent;
	struct clk *c;

	indent = clk_print_parent(clk, verbose);

	printf("\033[1m");
	dump_one(clk, verbose, indent);
	printf("\033[0m");

	list_for_each_entry(c, &clks, list) {
		struct clk *parent = clk_get_parent(c);

		if (parent == clk)
			dump_subtree(c, verbose, indent + 1);
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

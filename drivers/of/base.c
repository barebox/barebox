// SPDX-License-Identifier: GPL-2.0-only
/*
 * base.c - basic devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 */
#include <common.h>
#include <deep-probe.h>
#include <of.h>
#include <of_address.h>
#include <errno.h>
#include <malloc.h>
#include <init.h>
#include <memory.h>
#include <bootsource.h>
#include <linux/sizes.h>
#include <of_graph.h>
#include <string.h>
#include <libfile.h>
#include <linux/clk.h>
#include <linux/ctype.h>
#include <linux/err.h>

static struct device_node *root_node;

/**
 * of_node_has_prefix - Test if a node name has a given prefix
 * @np: The node name to test
 * @prefix: The prefix to see if @np starts with
 *
 * Returns:
 * * strlen(@prefix) if @np starts with @prefix
 * * 0 if @np does not start with @prefix
 */
size_t of_node_has_prefix(const struct device_node *np, const char *prefix)
{
	return np ? str_has_prefix(kbasename(np->full_name), prefix) : 0;
}
EXPORT_SYMBOL(of_node_has_prefix);

bool of_node_name_eq(const struct device_node *np, const char *name)
{
	const char *node_name;
	size_t len;

	if (!np)
		return false;

	node_name = kbasename(np->full_name);
	len = strchrnul(node_name, '@') - node_name;

	return (strlen(name) == len) && (strncmp(node_name, name, len) == 0);
}
EXPORT_SYMBOL(of_node_name_eq);

/*
 * Iterate over all nodes of a tree. As a devicetree does not
 * have a dedicated list head, the start node (usually the root
 * node) will not be iterated over.
 */
static inline struct device_node *of_next_node(struct device_node *node)
{
	struct device_node *next;

	if (!node)
		return root_node;

	next = list_first_entry(&node->list, struct device_node, list);

	return next->parent ? next : NULL;
}

#define of_tree_for_each_node_from(node, from) \
	for (node = of_next_node(from); node; node = of_next_node(node))

/**
 * struct alias_prop - Alias property in 'aliases' node
 * @link:	List node to link the structure in aliases_lookup list
 * @alias:	Alias property name
 * @np:		Pointer to device_node that the alias stands for
 * @id:		Index value from end of alias name
 * @stem:	Alias string without the index
 *
 * The structure represents one alias property of 'aliases' node as
 * an entry in aliases_lookup list.
 */
struct alias_prop {
	struct list_head link;
	const char *alias;
	struct device_node *np;
	int id;
	char stem[0];
};

static LIST_HEAD(aliases_lookup);

static struct device_node *of_aliases;

#define OF_ROOT_NODE_SIZE_CELLS_DEFAULT 1
#define OF_ROOT_NODE_ADDR_CELLS_DEFAULT 1

int of_bus_n_addr_cells(struct device_node *np)
{
	u32 cells;

	for (; np; np = np->parent)
		if (!of_property_read_u32(np, "#address-cells", &cells))
			return cells;

	/* No #address-cells property for the root node */
	return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}

int of_n_addr_cells(struct device_node *np)
{
	if (np->parent)
		np = np->parent;

	return of_bus_n_addr_cells(np);
}
EXPORT_SYMBOL(of_n_addr_cells);

int of_bus_n_size_cells(struct device_node *np)
{
	u32 cells;

	for (; np; np = np->parent)
		if (!of_property_read_u32(np, "#size-cells", &cells))
			return cells;

	/* No #size-cells property for the root node */
	return OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
}

int of_n_size_cells(struct device_node *np)
{
	if (np->parent)
		np = np->parent;

	return of_bus_n_size_cells(np);
}
EXPORT_SYMBOL(of_n_size_cells);

struct property *of_find_property(const struct device_node *np,
				  const char *name, int *lenp)
{
	struct property *pp;

	if (!np)
		return NULL;

	list_for_each_entry(pp, &np->properties, list)
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			return pp;
		}

	return NULL;
}
EXPORT_SYMBOL(of_find_property);

static void of_alias_add(struct alias_prop *ap, struct device_node *np,
			 int id, const char *stem, int stem_len)
{
	ap->np = np;
	ap->id = id;
	strncpy(ap->stem, stem, stem_len);
	ap->stem[stem_len] = 0;
	list_add_tail(&ap->link, &aliases_lookup);
	pr_debug("adding DT alias:%s: stem=%s id=%i node=%pOF\n",
		 ap->alias, ap->stem, ap->id, np);
}

static struct device_node *of_alias_resolve(struct device_node *root, struct property *pp)
{
	/* Skip those we do not want to proceed */
	if (!of_prop_cmp(pp->name, "name") ||
	    !of_prop_cmp(pp->name, "phandle") ||
	    !of_prop_cmp(pp->name, "linux,phandle"))
		return NULL;

	return of_find_node_by_path_from(root, of_property_get_value(pp));
}

static int of_alias_id_parse(const char *start, int *len)
{
	const char *end = start + strlen(start);

	/* walk the alias backwards to extract the id and work out
	 * the 'stem' string */
	while (isdigit(*(end-1)) && end > start)
		end--;

	*len = end - start;

	return simple_strtol(end, NULL, 10);
}

/**
 * of_alias_scan - Scan all properties of 'aliases' node
 *
 * The function scans all the properties of 'aliases' node and populates
 * the global lookup table with the properties.
 */
void of_alias_scan(void)
{
	struct property *pp;
	struct alias_prop *app, *tmp;

	list_for_each_entry_safe(app, tmp, &aliases_lookup, link)
		free(app);

	INIT_LIST_HEAD(&aliases_lookup);

	if (!root_node)
		return;

	of_aliases = of_find_node_by_path("/aliases");
	if (!of_aliases)
		return;

	list_for_each_entry(pp, &of_aliases->properties, list) {
		const char *start = pp->name;
		struct device_node *np;
		struct alias_prop *ap;
		int id, len;

		np = of_alias_resolve(root_node, pp);
		if (!np)
			continue;

		id = of_alias_id_parse(start, &len);
		if (id < 0)
			continue;

		/* Allocate an alias_prop with enough space for the stem */
		ap = xzalloc(sizeof(*ap) + len + 1);
		ap->alias = start;
		of_alias_add(ap, np, id, start, len);
	}
}
EXPORT_SYMBOL(of_alias_scan);

/**
 * of_alias_get_id - Get alias id for the given device_node
 * @np:		Pointer to the given device_node
 * @stem:	Alias stem of the given device_node
 *
 * The function travels the lookup table to get alias id for the given
 * device_node and alias stem.  It returns the alias id if find it.
 */
int of_alias_get_id(struct device_node *np, const char *stem)
{
	struct alias_prop *app;
	int id = -ENODEV;

	list_for_each_entry(app, &aliases_lookup, link) {
		if (of_node_cmp(app->stem, stem) != 0)
			continue;

		if (np == app->np) {
			id = app->id;
			break;
		}
	}

	return id;
}
EXPORT_SYMBOL_GPL(of_alias_get_id);

/**
 * of_alias_get_highest_id - Get highest alias id for the given stem
 * @stem:	Alias stem to be examined
 *
 * The function travels the lookup table to get the highest alias id for the
 * given alias stem.  It returns the alias id if found.
 */
int of_alias_get_highest_id(const char *stem)
{
	struct alias_prop *app;
	int id = -ENODEV;

	list_for_each_entry(app, &aliases_lookup, link) {
		if (strcmp(app->stem, stem) != 0)
			continue;

		if (app->id > id)
			id = app->id;
	}

	return id;
}
EXPORT_SYMBOL_GPL(of_alias_get_highest_id);

int of_alias_get_id_from(struct device_node *root, struct device_node *np,
			 const char *stem)
{
	struct device_node *aliasnp, *entrynp;
	struct property *pp;

	if (!root)
		return of_alias_get_id(np, stem);

	aliasnp = of_find_node_by_path_from(root, "/aliases");
	if (!aliasnp)
		return -ENODEV;

	for_each_property_of_node(aliasnp, pp) {
		const char *start = pp->name;
		int id, len;

		entrynp = of_alias_resolve(root_node, pp);
		if (entrynp != np)
			continue;

		id = of_alias_id_parse(start, &len);
		if (id < 0)
			continue;

		if (strncasecmp(start, stem, len))
			continue;

		return id;
	}

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_alias_get_id_from);

const char *of_alias_get(struct device_node *np)
{
	struct alias_prop *app;

	list_for_each_entry(app, &aliases_lookup, link) {
		if (np == app->np)
			return app->alias;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(of_alias_get);

static const char *of_get_partition_device_alias(struct device_node *np)
{
	const char *alias;

	alias = of_alias_get(np);
	if (alias)
		return alias;

	np = of_get_parent(np);
	if (np && of_node_is_fixed_partitions(np))
		np = of_get_parent(np);

	return of_alias_get(np);
}

const char *of_property_get_alias_from(struct device_node *root,
				       const char *np_name, const char *propname,
				       int index)
{
	struct device_node *node, *rnode;
	const char *path;
	int ret;

	node = of_find_node_by_path_or_alias(root, np_name);
	if (!node)
		return NULL;

	ret = of_property_read_string_index(node, propname, index, &path);
	if (ret < 0)
		return NULL;

	rnode = of_find_node_by_path(path);
	if (!rnode)
		return NULL;

	return of_get_partition_device_alias(rnode);
}
EXPORT_SYMBOL_GPL(of_property_get_alias_from);

const char *of_parse_phandle_and_get_alias_from(struct device_node *root,
						const char *np_name, const char *phandle_name,
						int index)
{
	struct device_node *node, *rnode;

	node = of_find_node_by_path_or_alias(root, np_name);
	if (!node)
		return NULL;

	rnode = of_parse_phandle_from(node, root, phandle_name, index);
	if (!rnode)
		return NULL;

	return of_get_partition_device_alias(rnode);
}
EXPORT_SYMBOL_GPL(of_parse_phandle_and_get_alias_from);

/*
 * of_find_node_by_alias - Find a node given an alias name
 * @root:    the root node of the tree. If NULL, use internal tree
 * @alias:   the alias name to find
 */
struct device_node *of_find_node_by_alias(struct device_node *root, const char *alias)
{
	struct device_node *aliasnp;
	int ret;
	const char *path;

	if (!root)
		root = root_node;

	aliasnp = of_find_node_by_path_from(root, "/aliases");
	if (!aliasnp)
		return NULL;

	ret = of_property_read_string(aliasnp, alias, &path);
	if (ret)
		return NULL;

	return of_find_node_by_path_from(root, path);
}
EXPORT_SYMBOL_GPL(of_find_node_by_alias);

/*
 * of_find_node_by_phandle_from - Find a node given a phandle from given
 * root node.
 * @handle:  phandle of the node to find
 * @root:    root node of the tree to search in. If NULL use the
 *           internal tree.
 */
struct device_node *of_find_node_by_phandle_from(phandle phandle,
		struct device_node *root)
{
	struct device_node *node;

	of_tree_for_each_node_from(node, root)
		if (node->phandle == phandle)
			return node;

	return NULL;
}
EXPORT_SYMBOL(of_find_node_by_phandle_from);

/*
 * of_find_node_by_phandle - Find a node given a phandle
 * @handle:    phandle of the node to find
 */
struct device_node *of_find_node_by_phandle(phandle phandle)
{
	return of_find_node_by_phandle_from(phandle, root_node);
}
EXPORT_SYMBOL(of_find_node_by_phandle);

/*
 * of_get_tree_max_phandle - Find the maximum phandle of a tree
 * @root:    root node of the tree to search in. If NULL use the
 *           internal tree.
 */
phandle of_get_tree_max_phandle(struct device_node *root)
{
	struct device_node *n;
	phandle max = 0;

	of_tree_for_each_node_from(n, root) {
		if (n->phandle > max)
			max = n->phandle;
	}

	return max;
}
EXPORT_SYMBOL(of_get_tree_max_phandle);

/*
 * of_node_create_phandle - create a phandle for a node
 * @node:    The node to create a phandle in
 *
 * returns the new phandle or the existing phandle if the node
 * already has a phandle.
 */
phandle of_node_create_phandle(struct device_node *node)
{
	phandle p;
	struct device_node *root;

	if (node->phandle)
		return node->phandle;

	root = of_find_root_node(node);

	p = of_get_tree_max_phandle(root) + 1;

	node->phandle = p;

	p = cpu_to_be32(p);

	of_set_property(node, "phandle", &p, sizeof(p), 1);

	return node->phandle;
}
EXPORT_SYMBOL(of_node_create_phandle);

int of_set_property_to_child_phandle(struct device_node *node, char *prop_name)
{
	int ret;
	phandle p;

	/* Check if property exist */
	if (!of_get_property(of_get_parent(node), prop_name, NULL))
		return -EINVAL;

	/* Create or get existing phandle of child node */
	p = of_node_create_phandle(node);
	p = cpu_to_be32(p);

	node = of_get_parent(node);

	ret = of_set_property(node, prop_name, &p, sizeof(p), 0);

	return ret;
}
EXPORT_SYMBOL(of_set_property_to_child_phandle);

/*
 * Find a property with a given name for a given node
 * and return the value.
 */
const void *of_get_property(const struct device_node *np, const char *name,
			 int *lenp)
{
	struct property *pp = of_find_property(np, name, lenp);

	if (!pp)
		return NULL;

	return of_property_get_value(pp);
}
EXPORT_SYMBOL(of_get_property);

/*
 * arch_match_cpu_phys_id - Match the given logical CPU and physical id
 *
 * @cpu: logical cpu index of a core/thread
 * @phys_id: physical identifier of a core/thread
 *
 * Returns true if the physical identifier and the logical cpu index
 * correspond to the same core/thread, false otherwise.
 */
static bool arch_match_cpu_phys_id(int cpu, u64 phys_id)
{
	return (u32)phys_id == cpu;
}

/**
 * Checks if the given "prop_name" property holds the physical id of the
 * core/thread corresponding to the logical cpu 'cpu'. If 'thread' is not
 * NULL, local thread number within the core is returned in it.
 */
static bool __of_find_n_match_cpu_property(struct device_node *cpun,
			const char *prop_name, int cpu, unsigned int *thread)
{
	const __be32 *cell;
	int ac, prop_len, tid;
	u64 hwid;

	ac = of_n_addr_cells(cpun);
	cell = of_get_property(cpun, prop_name, &prop_len);
	if (!cell || !ac)
		return false;
	prop_len /= sizeof(*cell) * ac;
	for (tid = 0; tid < prop_len; tid++) {
		hwid = of_read_number(cell, ac);
		if (arch_match_cpu_phys_id(cpu, hwid)) {
			if (thread)
				*thread = tid;
			return true;
		}
		cell += ac;
	}
	return false;
}

/*
 * arch_find_n_match_cpu_physical_id - See if the given device node is
 * for the cpu corresponding to logical cpu 'cpu'.  Return true if so,
 * else false.  If 'thread' is non-NULL, the local thread number within the
 * core is returned in it.
 */
static bool arch_find_n_match_cpu_physical_id(struct device_node *cpun,
					      int cpu, unsigned int *thread)
{
	return __of_find_n_match_cpu_property(cpun, "reg", cpu, thread);
}

/**
 * of_get_cpu_node - Get device node associated with the given logical CPU
 *
 * @cpu: CPU number(logical index) for which device node is required
 * @thread: if not NULL, local thread number within the physical core is
 *          returned
 *
 * The main purpose of this function is to retrieve the device node for the
 * given logical CPU index. It should be used to initialize the of_node in
 * cpu device. Once of_node in cpu device is populated, all the further
 * references can use that instead.
 *
 * CPU logical to physical index mapping is architecture specific and is built
 * before booting secondary cores. This function uses arch_match_cpu_phys_id
 * which can be overridden by architecture specific implementation.
 *
 * Returns a node pointer for the logical cpu with refcount incremented, use
 * of_node_put() on it when done. Returns NULL if not found.
 */
struct device_node *of_get_cpu_node(int cpu, unsigned int *thread)
{
	struct device_node *cpun;

	for_each_node_by_type(cpun, "cpu") {
		if (arch_find_n_match_cpu_physical_id(cpun, cpu, thread))
			return cpun;
	}
	return NULL;
}
EXPORT_SYMBOL(of_get_cpu_node);

/** Checks if the given "compat" string matches one of the strings in
 * the device's "compatible" property. Returns 0 on mismatch and a
 * positive score on match with the maximum being OF_DEVICE_COMPATIBLE_MAX_SCORE,
 * which is only returned if the first compatible matched.
 */
int of_device_is_compatible(const struct device_node *device,
		const char *compat)
{
	struct property *prop;
	const char *cp;
	int index = 0, score = 0;

	prop = of_find_property(device, "compatible", NULL);
	for (cp = of_prop_next_string(prop, NULL); cp;
	     cp = of_prop_next_string(prop, cp), index++) {
		if (of_compat_cmp(cp, compat, strlen(compat)) == 0) {
			score = OF_DEVICE_COMPATIBLE_MAX_SCORE - (index << 2);
			break;
		}
	}

	return score;
}
EXPORT_SYMBOL(of_device_is_compatible);

/**
 *	of_find_node_by_name_address - Find a node by its full name
 *	@from:	The node to start searching from or NULL, the node
 *		you pass will not be searched, only the next one
 *		will; typically, you pass what the previous call
 *		returned.
 *	@name:	The name string to match against
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_name_address(struct device_node *from,
	const char *name)
{
	struct device_node *np;

	of_tree_for_each_node_from(np, from)
		if (np->name && !of_node_cmp(np->name, name))
			return np;

	return NULL;
}
EXPORT_SYMBOL(of_find_node_by_name_address);

/**
 *	of_find_node_by_name - Find a node by its "name" property
 *	@from:	The node to start searching from or NULL, the node
 *		you pass will not be searched, only the next one
 *		will; typically, you pass what the previous call
 *		returned.
 *	@name:	The name string to match against
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_name(struct device_node *from,
	const char *name)
{
	struct device_node *np;

	of_tree_for_each_node_from(np, from)
		if (np->name && of_node_name_eq(np, name))
			return np;

	return NULL;
}
EXPORT_SYMBOL(of_find_node_by_name);

/**
 *	of_find_node_by_type - Find a node by its "device_type" property
 *	@from:  The node to start searching from, or NULL to start searching
 *		the entire device tree. The node you pass will not be
 *		searched, only the next one will; typically, you pass
 *		what the previous call returned.
 *	@type:  The type string to match against.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_type(struct device_node *from,
		const char *type)
{
	struct device_node *np;
	const char *device_type;
	int ret;

	of_tree_for_each_node_from(np, from) {
		ret = of_property_read_string(np, "device_type", &device_type);
		if (!ret && !of_node_cmp(device_type, type))
			return np;
	}
	return NULL;
}
EXPORT_SYMBOL(of_find_node_by_type);

/**
 *	of_find_compatible_node - Find a node based on type and one of the
 *                                tokens in its "compatible" property
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@type:		The type string to match "device_type" or NULL to ignore
 *                      (currently always ignored in barebox)
 *	@compatible:	The string to match to one of the tokens in the device
 *			"compatible" list.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_compatible_node(struct device_node *from,
	const char *type, const char *compatible)
{
	struct device_node *np;

	of_tree_for_each_node_from(np, from)
		if (of_device_is_compatible(np, compatible))
			return np;

	return NULL;
}
EXPORT_SYMBOL(of_find_compatible_node);

/**
 *	of_find_node_with_property - Find a node which has a property with
 *                                   the given name.
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@prop_name:	The name of the property to look for.
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_with_property(struct device_node *from,
	const char *prop_name)
{
	struct device_node *np;

	of_tree_for_each_node_from(np, from) {
		struct property *pp = of_find_property(np, prop_name, NULL);
		if (pp)
			return np;
	}

	return NULL;
}
EXPORT_SYMBOL(of_find_node_with_property);

/**
 * of_match_node - Tell if a device_node has a matching of_device_id structure
 *      @matches:       array of of device match structures to search in
 *      @node:          the of device structure to match against
 *
 *      Low level utility function used by device matching.
 *
 *      Return: pointer to the best matching of_device_id structure, or NULL
 */
const struct of_device_id *of_match_node(const struct of_device_id *matches,
					 const struct device_node *node)
{
	const struct of_device_id *best_match = NULL;
	int score, best_score = 0;

	if (!matches || !node)
		return NULL;

	for (; matches->compatible; matches++) {
		score = of_device_is_compatible(node, matches->compatible);

		if (score > best_score) {
			best_match = matches;
			best_score = score;

			if (score == OF_DEVICE_COMPATIBLE_MAX_SCORE)
				break;
		}
	}

	return best_match;
}

/**
 *	of_find_matching_node_and_match - Find a node based on an of_device_id
 *					  match table.
 *	@from:		The node to start searching from or NULL, the node
 *			you pass will not be searched, only the next one
 *			will; typically, you pass what the previous call
 *			returned.
 *	@matches:	array of of device match structures to search in
 *	@match		Updated to point at the matches entry which matched
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_matching_node_and_match(struct device_node *from,
					const struct of_device_id *matches,
					const struct of_device_id **match)
{
	struct device_node *np;

	if (match)
		*match = NULL;

	of_tree_for_each_node_from(np, from) {
		const struct of_device_id *m = of_match_node(matches, np);
		if (m) {
			if (match)
				*match = m;
			return np;
		}
	}

	return NULL;
}
EXPORT_SYMBOL(of_find_matching_node_and_match);

bool of_match(struct device *dev, const struct driver *drv)
{
	const struct of_device_id *id;

	id = of_match_node(drv->of_compatible, dev->of_node);
	if (!id)
		return false;

	dev->of_id_entry = id;

	return true;
}
EXPORT_SYMBOL(of_match);
/**
 * of_find_property_value_of_size
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @min:	minimum allowed length of property value
 * @max:	maximum allowed length of property value (0 means unlimited)
 * @len:	if !=NULL, actual length is written to here
 *
 * Search for a property in a device node and valid the requested size.
 *
 * Return: The property value on success, -EINVAL if the property does not
 * exist, -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data is too small or too large.
 *
 */
static const void *of_find_property_value_of_size(const struct device_node *np,
			const char *propname, u32 min, u32 max, size_t *len)
{
	struct property *prop = of_find_property(np, propname, NULL);
	const void *value;

	if (!prop)
		return ERR_PTR(-EINVAL);
	value = of_property_get_value(prop);
	if (!value)
		return ERR_PTR(-ENODATA);
	if (prop->length < min)
		return ERR_PTR(-EOVERFLOW);
	if (max && prop->length > max)
		return ERR_PTR(-EOVERFLOW);

	if (len)
		*len = prop->length;

	return value;
}

/**
 * of_property_read_u32_index - Find and read a u32 from a multi-value property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @index:	index of the u32 in the list of values
 * @out_value:	pointer to return value, modified only if no error.
 *
 * Search for a property in a device node and read nth 32-bit value from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid u32 value can be decoded.
 */
int of_property_read_u32_index(const struct device_node *np,
				       const char *propname,
				       u32 index, u32 *out_value)
{
	const u32 *val = of_find_property_value_of_size(np, propname,
					((index + 1) * sizeof(*out_value)),
							0, NULL);

	if (IS_ERR(val))
		return PTR_ERR(val);

	*out_value = be32_to_cpup(((__be32 *)val) + index);
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u32_index);

/**
 * of_property_count_elems_of_size - Count the number of elements in a property
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @elem_size:	size of the individual element
 *
 * Search for a property in a device node and count the number of elements of
 * size elem_size in it. Returns number of elements on sucess, -EINVAL if the
 * property does not exist or its length does not match a multiple of elem_size
 * and -ENODATA if the property does not have a value.
 */
int of_property_count_elems_of_size(const struct device_node *np,
				const char *propname, int elem_size)
{
	struct property *prop = of_find_property(np, propname, NULL);

	if (!prop)
		return -EINVAL;
	if (!of_property_get_value(prop))
		return -ENODATA;

	if (prop->length % elem_size != 0) {
		pr_err("size of %s in node %pOF is not a multiple of %d\n",
		       propname, np, elem_size);
		return -EINVAL;
	}

	return prop->length / elem_size;
}
EXPORT_SYMBOL_GPL(of_property_count_elems_of_size);

/**
 * of_property_read_u8_array - Find and read an array of u8 from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 8-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * dts entry of array should be like:
 *	property = /bits/ 8 <0x50 0x60 0x70>;
 *
 * The out_value is modified only if a valid u8 value can be decoded.
 */
int of_property_read_u8_array(const struct device_node *np,
			const char *propname, u8 *out_values, size_t sz)
{
	const u8 *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)),
						       0, NULL);

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = *val++;
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u8_array);

/**
 * of_property_read_u16_array - Find and read an array of u16 from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 16-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * dts entry of array should be like:
 *	property = /bits/ 16 <0x5000 0x6000 0x7000>;
 *
 * The out_value is modified only if a valid u16 value can be decoded.
 */
int of_property_read_u16_array(const struct device_node *np,
			const char *propname, u16 *out_values, size_t sz)
{
	const __be16 *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)),
							   0, NULL);

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = be16_to_cpup(val++);
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u16_array);

/**
 * of_property_read_u32_array - Find and read an array of 32 bit integers
 * from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 32-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid u32 value can be decoded.
 */
int of_property_read_u32_array(const struct device_node *np,
			       const char *propname, u32 *out_values,
			       size_t sz)
{
	const __be32 *val = of_find_property_value_of_size(np, propname,
						(sz * sizeof(*out_values)),
							   0, NULL);

	if (IS_ERR(val))
		return PTR_ERR(val);

	while (sz--)
		*out_values++ = be32_to_cpup(val++);
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u32_array);

/**
 * of_property_read_u64 - Find and read a 64 bit integer from a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 *
 * Search for a property in a device node and read a 64-bit value from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid u64 value can be decoded.
 */
int of_property_read_u64(const struct device_node *np, const char *propname,
			 u64 *out_value)
{
	const __be32 *val = of_find_property_value_of_size(np, propname,
						sizeof(*out_value), 0, NULL);

	if (IS_ERR(val))
		return PTR_ERR(val);

	*out_value = of_read_number(val, 2);
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u64);

/**
 * of_property_read_variable_u8_array - Find and read an array of u8 from a
 * property, with bounds on the minimum and maximum array size.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_values:	pointer to found values.
 * @sz_min:	minimum number of array elements to read
 * @sz_max:	maximum number of array elements to read, if zero there is no
 *		upper limit on the number of elements in the dts entry but only
 *		sz_min will be read.
 *
 * Search for a property in a device node and read 8-bit value(s) from
 * it.
 *
 * dts entry of array should be like:
 *  ``property = /bits/ 8 <0x50 0x60 0x70>;``
 *
 * Return: The number of elements read on success, -EINVAL if the property
 * does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
 * if the property data is smaller than sz_min or longer than sz_max.
 *
 * The out_values is modified only if a valid u8 value can be decoded.
 */
int of_property_read_variable_u8_array(const struct device_node *np,
					const char *propname, u8 *out_values,
					size_t sz_min, size_t sz_max)
{
	size_t sz, count;
	const u8 *val = of_find_property_value_of_size(np, propname,
						(sz_min * sizeof(*out_values)),
						(sz_max * sizeof(*out_values)),
						&sz);

	if (IS_ERR(val))
		return PTR_ERR(val);

	if (!sz_max)
		sz = sz_min;
	else
		sz /= sizeof(*out_values);

	count = sz;
	while (count--)
		*out_values++ = *val++;

	return sz;
}
EXPORT_SYMBOL_GPL(of_property_read_variable_u8_array);

/**
 * of_property_read_variable_u16_array - Find and read an array of u16 from a
 * property, with bounds on the minimum and maximum array size.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_values:	pointer to found values.
 * @sz_min:	minimum number of array elements to read
 * @sz_max:	maximum number of array elements to read, if zero there is no
 *		upper limit on the number of elements in the dts entry but only
 *		sz_min will be read.
 *
 * Search for a property in a device node and read 16-bit value(s) from
 * it.
 *
 * dts entry of array should be like:
 *  ``property = /bits/ 16 <0x5000 0x6000 0x7000>;``
 *
 * Return: The number of elements read on success, -EINVAL if the property
 * does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
 * if the property data is smaller than sz_min or longer than sz_max.
 *
 * The out_values is modified only if a valid u16 value can be decoded.
 */
int of_property_read_variable_u16_array(const struct device_node *np,
					const char *propname, u16 *out_values,
					size_t sz_min, size_t sz_max)
{
	size_t sz, count;
	const __be16 *val = of_find_property_value_of_size(np, propname,
						(sz_min * sizeof(*out_values)),
						(sz_max * sizeof(*out_values)),
						&sz);

	if (IS_ERR(val))
		return PTR_ERR(val);

	if (!sz_max)
		sz = sz_min;
	else
		sz /= sizeof(*out_values);

	count = sz;
	while (count--)
		*out_values++ = be16_to_cpup(val++);

	return sz;
}
EXPORT_SYMBOL_GPL(of_property_read_variable_u16_array);

/**
 * of_property_read_variable_u32_array - Find and read an array of 32 bit
 * integers from a property, with bounds on the minimum and maximum array size.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_values:	pointer to return found values.
 * @sz_min:	minimum number of array elements to read
 * @sz_max:	maximum number of array elements to read, if zero there is no
 *		upper limit on the number of elements in the dts entry but only
 *		sz_min will be read.
 *
 * Search for a property in a device node and read 32-bit value(s) from
 * it.
 *
 * Return: The number of elements read on success, -EINVAL if the property
 * does not exist, -ENODATA if property does not have a value, and -EOVERFLOW
 * if the property data is smaller than sz_min or longer than sz_max.
 *
 * The out_values is modified only if a valid u32 value can be decoded.
 */
int of_property_read_variable_u32_array(const struct device_node *np,
			       const char *propname, u32 *out_values,
			       size_t sz_min, size_t sz_max)
{
	size_t sz, count;
	const __be32 *val = of_find_property_value_of_size(np, propname,
						(sz_min * sizeof(*out_values)),
						(sz_max * sizeof(*out_values)),
						&sz);

	if (IS_ERR(val))
		return PTR_ERR(val);

	if (!sz_max)
		sz = sz_min;
	else
		sz /= sizeof(*out_values);

	count = sz;
	while (count--)
		*out_values++ = be32_to_cpup(val++);

	return sz;
}
EXPORT_SYMBOL_GPL(of_property_read_variable_u32_array);

/**
 * of_property_read_variable_u64_array - Find and read an array of 64 bit integers
 * from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
 * @sz:		number of array elements to read
 *
 * Search for a property in a device node and read 64-bit value(s) from
 * it. Returns 0 on success, -EINVAL if the property does not exist,
 * -ENODATA if property does not have a value, and -EOVERFLOW if the
 * property data isn't large enough.
 *
 * The out_value is modified only if a valid u64 value can be decoded.
 */
int of_property_read_variable_u64_array(const struct device_node *np,
			       const char *propname, u64 *out_values,
			       size_t sz_min, size_t sz_max)
{
	size_t sz, count;
	const __be32 *val = of_find_property_value_of_size(np, propname,
						(sz_min * sizeof(*out_values)),
						(sz_max * sizeof(*out_values)),
						&sz);

	if (IS_ERR(val))
		return PTR_ERR(val);

	if (!sz_max)
		sz = sz_min;
	else
		sz /= sizeof(*out_values);

	count = sz;
	while (count--) {
		*out_values++ = of_read_number(val, 2);
		val += 2;
	}

	return sz;
}
EXPORT_SYMBOL_GPL(of_property_read_variable_u64_array);

/**
 * of_property_read_string - Find and read a string from a property
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_string:	pointer to null terminated return string, modified only if
 *		return value is 0.
 *
 * Search for a property in a device tree node and retrieve a null
 * terminated string value (pointer to data, not a copy). Returns 0 on
 * success, -EINVAL if the property does not exist, -ENODATA if property
 * does not have a value, and -EILSEQ if the string is not null-terminated
 * within the length of the property data.
 *
 * The out_string pointer is modified only if a valid string can be decoded.
 */
int of_property_read_string(struct device_node *np, const char *propname,
				const char **out_string)
{
	struct property *prop = of_find_property(np, propname, NULL);
	const void *value;

	if (!prop)
		return -EINVAL;
	value = of_property_get_value(prop);
	if (!value)
		return -ENODATA;
	if (strnlen(value, prop->length) >= prop->length)
		return -EILSEQ;
	*out_string = value;
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_string);

/**
 * of_property_match_string() - Find string in a list and return index
 * @np: pointer to node containing string list property
 * @propname: string list property name
 * @string: pointer to string to search for in string list
 *
 * This function searches a string list property and returns the index
 * of a specific string value.
 */
int of_property_match_string(const struct device_node *np, const char *propname,
			     const char *string)
{
	struct property *prop = of_find_property(np, propname, NULL);
	size_t l;
	int i;
	const char *p, *end;

	if (!prop)
		return -EINVAL;

	p = of_property_get_value(prop);
	if (!p)
		return -ENODATA;

	end = p + prop->length;

	for (i = 0; p < end; i++, p += l) {
		l = strlen(p) + 1;
		if (p + l > end)
			return -EILSEQ;
		pr_debug("comparing %s with %s\n", string, p);
		if (strcmp(string, p) == 0)
			return i; /* Found it; return index */
	}
	return -ENODATA;
}
EXPORT_SYMBOL_GPL(of_property_match_string);

const __be32 *of_prop_next_u32(const struct property *prop, const __be32 *cur,
			u32 *pu)
{
	const void *curv = cur;
	const void *value;

	if (!prop)
		return NULL;

	value = of_property_get_value(prop);

	if (!cur) {
		curv = value;
		goto out_val;
	}

	curv += sizeof(*cur);
	if (curv >= value + prop->length)
		return NULL;

out_val:
	*pu = be32_to_cpup(curv);
	return curv;
}
EXPORT_SYMBOL_GPL(of_prop_next_u32);

const char *of_prop_next_string(const struct property *prop, const char *cur)
{
	const void *curv = cur;
	const void *value;

	if (!prop)
		return NULL;

	value = of_property_get_value(prop);

	if (!cur)
		return value;

	curv += strlen(cur) + 1;
	if (curv >= value + prop->length)
		return NULL;

	return curv;
}
EXPORT_SYMBOL_GPL(of_prop_next_string);

/**
 * of_property_write_bool - Create/Delete empty (bool) property.
 *
 * @np:		device node from which the property is to be set.
 * @propname:	name of the property to be set.
 * @value	true to set, false to delete
 *
 * Search for a property in a device node and create or delete the property.
 * If the property already exists and write value is false, the property is
 * deleted. If write value is true and the property does not exist, it is
 * created. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_bool(struct device_node *np, const char *propname,
			   const bool value)
{
	struct property *prop = of_find_property(np, propname, NULL);

	if (!value) {
		if (prop)
			of_delete_property(prop);
		return 0;
	}

	if (!prop)
		prop = of_new_property(np, propname, NULL, 0);
	if (!prop)
		return -ENOMEM;

	return 0;
}

/**
 * of_property_write_u8_array - Write an array of u8 to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 8-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u8_array(struct device_node *np,
			       const char *propname, const u8 *values,
			       size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, values, sizeof(*values) * sz);
	if (!prop)
		return -ENOMEM;

	return 0;
}

/**
 * of_property_write_u16_array - Write an array of u16 to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 16-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u16_array(struct device_node *np,
				const char *propname, const u16 *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be16 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--)
		*val++ = cpu_to_be16(*values++);

	return 0;
}

/**
 * of_property_write_u32_array - Write an array of u32 to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 32-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u32_array(struct device_node *np,
				const char *propname, const u32 *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be32 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--)
		*val++ = cpu_to_be32(*values++);

	return 0;
}

/**
 * of_property_write_u64_array - Write an array of u64 to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @values:	pointer to array elements to write.
 * @sz:		number of array elements to write.
 *
 * Search for a property in a device node and write 64-bit value(s) to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_u64_array(struct device_node *np,
				const char *propname, const u64 *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname, NULL);
	__be32 *val;

	if (prop)
		of_delete_property(prop);

	prop = of_new_property(np, propname, NULL, 2 * sizeof(*val) * sz);
	if (!prop)
		return -ENOMEM;

	val = prop->value;
	while (sz--) {
		of_write_number(val, *values++, 2);
		val += 2;
	}

	return 0;
}

/**
 * of_property_write_strings - Write strings to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @...:	pointers to strings to write
 *
 * Search for a property in a device node and write a string to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created, -EINVAL if no strings specified.
 */
int of_property_write_strings(struct device_node *np,
			      const char *propname, ...)
{
	const char *val;
	char *buf = NULL, *next;
	size_t len = 0;
	va_list ap;
	int ret = 0;

	va_start(ap, propname);
	for (val = va_arg(ap, char *); val; val = va_arg(ap, char *))
		len += strlen(val) + 1;
	va_end(ap);

	if (!len)
		return -EINVAL;

	buf = malloc(len);
	if (!buf)
		return -ENOMEM;

	next = buf;

	va_start(ap, propname);
	for (val = va_arg(ap, char *); val; val = va_arg(ap, char *))
		next = stpcpy(next, val) + 1;
	va_end(ap);

	ret = of_set_property(np, propname, buf, len, 1);
	free(buf);
	return ret;
}

/**
 * of_property_write_string - Write a string to a property. If
 * the property does not exist, it will be created and appended to the given
 * device node.
 *
 * @np:		device node to which the property value is to be written.
 * @propname:	name of the property to be written.
 * @value:	pointer to the string to write
 *
 * Search for a property in a device node and write a string to
 * it. If the property does not exist, it will be created and appended to
 * the device node. Returns 0 on success, -ENOMEM if the property or array
 * of elements cannot be created.
 */
int of_property_write_string(struct device_node *np,
			     const char *propname, const char *value)
{
	size_t len = strlen(value);

	return of_set_property(np, propname, value, len + 1, 1);
}

/**
 * of_parse_phandle_from - Resolve a phandle property to a device_node pointer from
 * a given root node
 * @np: Pointer to device node holding phandle property
 * @root: root node of the tree to search in. If NULL use the internal tree.
 * @phandle_name: Name of property holding a phandle value
 * @index: For properties holding a table of phandles, this is the index into
 *         the table
 *
 * Returns the device_node pointer found or NULL.
 */
struct device_node *of_parse_phandle_from(const struct device_node *np,
					struct device_node *root,
					const char *phandle_name, int index)
{
	const __be32 *phandle;
	int size;

	phandle = of_get_property(np, phandle_name, &size);
	if ((!phandle) || (size < sizeof(*phandle) * (index + 1)))
		return NULL;

	return of_find_node_by_phandle_from(be32_to_cpup(phandle + index),
					root);
}
EXPORT_SYMBOL(of_parse_phandle_from);

/**
 * of_parse_phandle - Resolve a phandle property to a device_node pointer
 * @np: Pointer to device node holding phandle property
 * @phandle_name: Name of property holding a phandle value
 * @index: For properties holding a table of phandles, this is the index into
 *         the table
 *
 * Returns the device_node pointer found or NULL.
 */
struct device_node *of_parse_phandle(const struct device_node *np,
				     const char *phandle_name, int index)
{
	return of_parse_phandle_from(np, root_node, phandle_name, index);
}
EXPORT_SYMBOL(of_parse_phandle);

int of_phandle_iterator_init(struct of_phandle_iterator *it,
		const struct device_node *np,
		const char *list_name,
		const char *cells_name,
		int cell_count)
{
	const __be32 *list;
	int size;

	memset(it, 0, sizeof(*it));

	/*
	 * one of cell_count or cells_name must be provided to determine the
	 * argument length.
	 */
	if (cell_count < 0 && !cells_name)
		return -EINVAL;

	list = of_get_property(np, list_name, &size);
	if (!list)
		return -ENOENT;

	it->cells_name = cells_name;
	it->cell_count = cell_count;
	it->parent = np;
	it->list_end = list + size / sizeof(*list);
	it->phandle_end = list;
	it->cur = list;

	return 0;
}
EXPORT_SYMBOL_GPL(of_phandle_iterator_init);

int of_phandle_iterator_next(struct of_phandle_iterator *it)
{
	uint32_t count = 0;

	if (it->node) {
		of_node_put(it->node);
		it->node = NULL;
	}

	if (!it->cur || it->phandle_end >= it->list_end)
		return -ENOENT;

	it->cur = it->phandle_end;

	/* If phandle is 0, then it is an empty entry with no arguments. */
	it->phandle = be32_to_cpup(it->cur++);

	if (it->phandle) {

		/*
		 * Find the provider node and parse the #*-cells property to
		 * determine the argument length.
		 */
		it->node = of_find_node_by_phandle(it->phandle);

		if (it->cells_name) {
			if (!it->node) {
				pr_err("%pOF: could not find phandle %d\n",
				       it->parent, it->phandle);
				goto err;
			}

			if (of_property_read_u32(it->node, it->cells_name,
						 &count)) {
				/*
				 * If both cell_count and cells_name is given,
				 * fall back to cell_count in absence
				 * of the cells_name property
				 */
				if (it->cell_count >= 0) {
					count = it->cell_count;
				} else {
					pr_err("%pOF: could not get %s for %pOF\n",
					       it->parent,
					       it->cells_name,
					       it->node);
					goto err;
				}
			}
		} else {
			count = it->cell_count;
		}

		/*
		 * Make sure that the arguments actually fit in the remaining
		 * property data length
		 */
		if (it->cur + count > it->list_end) {
			if (it->cells_name)
				pr_err("%pOF: %s = %d found %td\n",
					it->parent, it->cells_name,
					count, it->list_end - it->cur);
			else
				pr_err("%pOF: phandle %s needs %d, found %td\n",
					it->parent, of_node_full_name(it->node),
					count, it->list_end - it->cur);
			goto err;
		}
	}

	it->phandle_end = it->cur + count;
	it->cur_count = count;

	return 0;

err:
	if (it->node) {
		of_node_put(it->node);
		it->node = NULL;
	}

	return -EINVAL;
}
EXPORT_SYMBOL_GPL(of_phandle_iterator_next);

int of_phandle_iterator_args(struct of_phandle_iterator *it,
			     uint32_t *args,
			     int size)
{
	int i, count;

	count = it->cur_count;

	if (WARN_ON(size < count))
		count = size;

	for (i = 0; i < count; i++)
		args[i] = be32_to_cpup(it->cur++);

	return count;
}

int __of_parse_phandle_with_args(const struct device_node *np,
				 const char *list_name,
				 const char *cells_name,
				 int cell_count, int index,
				 struct of_phandle_args *out_args)
{
	struct of_phandle_iterator it;
	int rc, cur_index = 0;

	if (index < 0)
		return -EINVAL;

	/* Loop over the phandles until all the requested entry is found */
	of_for_each_phandle(&it, rc, np, list_name, cells_name, cell_count) {
		/*
		 * All of the error cases bail out of the loop, so at
		 * this point, the parsing is successful. If the requested
		 * index matches, then fill the out_args structure and return,
		 * or return -ENOENT for an empty entry.
		 */
		rc = -ENOENT;
		if (cur_index == index) {
			if (!it.phandle)
				goto err;

			if (out_args) {
				int c;

				c = of_phandle_iterator_args(&it,
							     out_args->args,
							     MAX_PHANDLE_ARGS);
				out_args->np = it.node;
				out_args->args_count = c;
			} else {
				of_node_put(it.node);
			}

			/* Found it! return success */
			return 0;
		}

		cur_index++;
	}

	/*
	 * Unlock node before returning result; will be one of:
	 * -ENOENT : index is for empty phandle
	 * -EINVAL : parsing error on data
	 */

 err:
	of_node_put(it.node);
	return rc;
}
EXPORT_SYMBOL(__of_parse_phandle_with_args);

/**
 * of_count_phandle_with_args() - Find the number of phandles references in a property
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 *
 * Return: The number of phandle + argument tuples within a property. It
 * is a typical pattern to encode a list of phandle and variable
 * arguments into a single property. The number of arguments is encoded
 * by a property in the phandle-target node. For example, a gpios
 * property would contain a list of GPIO specifies consisting of a
 * phandle and 1 or more arguments. The number of arguments are
 * determined by the #gpio-cells property in the node pointed to by the
 * phandle.
 */
int of_count_phandle_with_args(const struct device_node *np, const char *list_name,
				const char *cells_name)
{
	struct of_phandle_iterator it;
	int rc, cur_index = 0;

	/*
	 * If cells_name is NULL we assume a cell count of 0. This makes
	 * counting the phandles trivial as each 32bit word in the list is a
	 * phandle and no arguments are to consider. So we don't iterate through
	 * the list but just use the length to determine the phandle count.
	 */
	if (!cells_name) {
		const __be32 *list;
		int size;

		list = of_get_property(np, list_name, &size);
		if (!list)
			return -ENOENT;

		return size / sizeof(*list);
	}

	rc = of_phandle_iterator_init(&it, np, list_name, cells_name, -1);
	if (rc)
		return rc;

	while ((rc = of_phandle_iterator_next(&it)) == 0)
		cur_index += 1;

	if (rc != -ENOENT)
		return rc;

	return cur_index;
}
EXPORT_SYMBOL(of_count_phandle_with_args);

/**
 * of_machine_is_compatible - Test root of device tree for a given compatible value
 * @compat: compatible string to look for in root node's compatible property.
 *
 * Returns true if the root node has the given value in its
 * compatible property.
 */
int of_machine_is_compatible(const char *compat)
{
	if (!root_node)
		return 0;

	return of_device_is_compatible(root_node, compat);
}
EXPORT_SYMBOL(of_machine_is_compatible);

/**
 *	of_find_node_by_path_from - Find a node matching a full OF path
 *      relative to a given root node.
 *	@path:	The full path to match
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path_from(struct device_node *from,
					const char *path)
{
	char *slash, *p, *freep;

	if (!from)
		from = root_node;

	if (!from || !path || *path != '/')
		return NULL;

	path++;

	freep = p = xstrdup(path);

	while (1) {
		if (!*p)
			goto out;

		slash = strchr(p, '/');
		if (slash)
			*slash = 0;

		from = of_get_child_by_name(from, p);
		if (!from)
			goto out;

		if (!slash)
			goto out;

		p = slash + 1;
	}
out:
	free(freep);

	return from;
}
EXPORT_SYMBOL(of_find_node_by_path_from);

/**
 *	of_find_node_by_path - Find a node matching a full OF path
 *	@path:	The full path to match
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path(const char *path)
{
	return of_find_node_by_path_from(root_node, path);
}
EXPORT_SYMBOL(of_find_node_by_path);

/**
 *	of_find_node_by_path_or_alias - Find a node matching a full OF path
 *	or an alias
 *	@root:	The root node. If NULL the internal tree is used
 *	@str:	the full path or alias
 *
 *	Returns a pointer to the node found or NULL.
 */
struct device_node *of_find_node_by_path_or_alias(struct device_node *root,
		const char *str)
{
	struct device_node *node;
	const char *slash;
	char *alias;
	size_t len = 0;

	if (*str ==  '/')
		return of_find_node_by_path_from(root, str);

	slash = strchr(str, '/');

	if (!slash)
		return of_find_node_by_alias(root, str);

	len = slash - str + 1;
	alias = xmalloc(len);
	strlcpy(alias, str, len);

	node = of_find_node_by_alias(root, alias);

	if (!node)
		goto out;

	node = of_find_node_by_path_from(node, slash);
out:
	free(alias);
	return node;
}
EXPORT_SYMBOL(of_find_node_by_path_or_alias);

/**
 * of_modalias_node - Lookup appropriate modalias for a device node
 * @node:	pointer to a device tree node
 * @modalias:	Pointer to buffer that modalias value will be copied into
 * @len:	Length of modalias value
 *
 * Based on the value of the compatible property, this routine will attempt
 * to choose an appropriate modalias value for a particular device tree node.
 * It does this by stripping the manufacturer prefix (as delimited by a ',')
 * from the first entry in the compatible list property.
 *
 * This routine returns 0 on success, <0 on failure.
 */
int of_modalias_node(struct device_node *node, char *modalias, int len)
{
	const char *compatible, *p;
	int cplen;

	compatible = of_get_property(node, "compatible", &cplen);
	if (!compatible || strlen(compatible) > cplen)
		return -ENODEV;
	p = strchr(compatible, ',');
	strlcpy(modalias, p ? p + 1 : compatible, len);
	return 0;
}
EXPORT_SYMBOL_GPL(of_modalias_node);

static struct device_node *of_chosen;
static const char *of_model;

struct device_node *of_get_root_node(void)
{
	return root_node;
}

int of_set_root_node(struct device_node *node)
{
	if (node && root_node)
		return -EBUSY;

	root_node = node;

	of_chosen = of_find_node_by_path("/chosen");
	of_property_read_string(root_node, "model", &of_model);

	if (of_model)
		barebox_set_model(of_model);

	of_alias_scan();

	return 0;
}

static int barebox_of_populate(void)
{
	if (IS_ENABLED(CONFIG_OFDEVICE) && deep_probe_is_supported())
		return of_probe();

	return 0;
}
of_populate_initcall(barebox_of_populate);

int barebox_register_of(struct device_node *root)
{
	if (root_node)
		return -EBUSY;

	of_set_root_node(root);
	of_fix_tree(root);

	if (IS_ENABLED(CONFIG_OFDEVICE)) {
		of_clk_init();
		if (!deep_probe_is_supported())
			return of_probe();
	}

	return 0;
}

int barebox_register_fdt(const void *dtb)
{
	struct device_node *root;

	if (root_node)
		return -EBUSY;

	root = of_unflatten_dtb(dtb, INT_MAX);
	if (IS_ERR(root)) {
		pr_err("Cannot unflatten dtb: %pe\n", root);
		return PTR_ERR(root);
	}

	return barebox_register_of(root);
}

/**
 *  of_device_is_available - check if a device is available for use
 *
 *  @device: Node to check for availability
 *
 *  Returns 1 if the status property is absent or set to "okay" or "ok",
 *  0 otherwise
 */
int of_device_is_available(const struct device_node *device)
{
	const char *status;
	int statlen;

	status = of_get_property(device, "status", &statlen);
	if (status == NULL)
		return 1;

	if (statlen > 0) {
		if (!strcmp(status, "okay") || !strcmp(status, "ok"))
			return 1;
	}

	return 0;
}
EXPORT_SYMBOL(of_device_is_available);

/**
 *  of_device_is_big_endian - check if a device has BE registers
 *
 *  @device: Node to check for endianness
 *
 *  Returns true if the device has a "big-endian" property, or if the kernel
 *  was compiled for BE *and* the device has a "native-endian" property.
 *  Returns false otherwise.
 *
 *  Callers would nominally use ioread32be/iowrite32be if
 *  of_device_is_big_endian() == true, or readl/writel otherwise.
 */
bool of_device_is_big_endian(const struct device_node *device)
{
	if (of_property_read_bool(device, "big-endian"))
		return true;
	if (IS_ENABLED(CONFIG_CPU_BIG_ENDIAN) &&
	    of_property_read_bool(device, "native-endian"))
		return true;
	return false;
}
EXPORT_SYMBOL(of_device_is_big_endian);

/**
 *	of_get_parent - Get a node's parent if any
 *	@node:	Node to get parent
 *
 *	Returns a pointer to the parent node or NULL if already at root.
 */
struct device_node *of_get_parent(const struct device_node *node)
{
	return (!node) ? NULL : node->parent;
}
EXPORT_SYMBOL(of_get_parent);

/**
 *	of_get_next_available_child - Find the next available child node
 *	@node:	parent node
 *	@prev:	previous child of the parent node, or NULL to get first
 *
 *      This function is like of_get_next_child(), except that it
 *      automatically skips any disabled nodes (i.e. status = "disabled").
 */
struct device_node *of_get_next_available_child(const struct device_node *node,
	struct device_node *prev)
{
	prev = list_prepare_entry(prev, &node->children, parent_list);
	list_for_each_entry_continue(prev, &node->children, parent_list)
		if (of_device_is_available(prev))
			return prev;
	return NULL;
}
EXPORT_SYMBOL(of_get_next_available_child);

/**
 *	of_get_next_child - Iterate a node children
 *	@node:  parent node
 *	@prev:  previous child of the parent node, or NULL to get first
 *
 *	Returns a node pointer with refcount incremented.
 */
struct device_node *of_get_next_child(const struct device_node *node,
	struct device_node *prev)
{
	if (prev)
		node = prev->parent;

	prev = list_prepare_entry(prev, &node->children, parent_list);
	list_for_each_entry_continue(prev, &node->children, parent_list)
		return prev;
	return NULL;
}
EXPORT_SYMBOL(of_get_next_child);

/**
 *	of_get_child_count - Count child nodes of given parent node
 *	@parent:	parent node
 *
 *      Returns the number of child nodes or -EINVAL on NULL parent node.
 */
int of_get_child_count(const struct device_node *parent)
{
	struct device_node *child;
	int num = 0;

	if (!parent)
		return -EINVAL;

	for_each_child_of_node(parent, child)
		num++;

	return num;
}
EXPORT_SYMBOL(of_get_child_count);

/**
 *	of_get_available_child_count - Count available child nodes of given
 *      parent node
 *	@parent:	parent node
 *
 *      Returns the number of available child nodes or -EINVAL on NULL parent
 *      node.
 */
int of_get_available_child_count(const struct device_node *parent)
{
	struct device_node *child;
	int num = 0;

	if (!parent)
		return -EINVAL;

	for_each_child_of_node(parent, child)
		if (of_device_is_available(child))
			num++;

	return num;
}
EXPORT_SYMBOL(of_get_available_child_count);

/**
 * of_get_compatible_child - Find compatible child node
 * @parent:	parent node
 * @compatible:	compatible string
 *
 * Lookup child node whose compatible property contains the given compatible
 * string.
 *
 * Returns a node pointer with refcount incremented, use of_node_put() on it
 * when done; or NULL if not found.
 */
struct device_node *of_get_compatible_child(const struct device_node *parent,
				const char *compatible)
{
	struct device_node *child;

	for_each_child_of_node(parent, child) {
		if (of_device_is_compatible(child, compatible))
			return child;
	}

	return NULL;
}
EXPORT_SYMBOL(of_get_compatible_child);

/**
 *	of_get_child_by_name - Find the child node by name for a given parent
 *	@node:	parent node
 *	@name:	child name to look for.
 *
 *      This function looks for child node for given matching full name
 *
 *	Returns a node pointer if found or NULL.
 */
struct device_node *of_get_child_by_name(const struct device_node *node,
				const char *name)
{
	struct device_node *child;

	for_each_child_of_node(node, child)
		if (child->name && (of_node_cmp(child->name, name) == 0))
			return child;

	return NULL;
}
EXPORT_SYMBOL(of_get_child_by_name);

/**
 *	of_get_child_by_name_stem - Find the child node by name for a given parent
 *	@node:	parent node
 *	@name:	child name to look for.
 *
 *      This function looks for child node for given matching name excluding the
 *      unit address
 *
 *	Returns a node pointer if found or NULL.
 */
struct device_node *of_get_child_by_name_stem(const struct device_node *node,
				const char *name)
{
	struct device_node *child;

	for_each_child_of_node(node, child)
		if (of_node_name_eq(child, name))
			break;
	return child;
}
EXPORT_SYMBOL(of_get_child_by_name_stem);

/**
 * of_property_read_string_helper() - Utility helper for parsing string properties
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_strs:	output array of string pointers.
 * @sz:		number of array elements to read.
 * @skip:	Number of strings to skip over at beginning of list.
 *
 * Don't call this function directly. It is a utility helper for the
 * of_property_read_string*() family of functions.
 */
int of_property_read_string_helper(const struct device_node *np,
				   const char *propname, const char **out_strs,
				   size_t sz, int skip)
{
	const struct property *prop = of_find_property(np, propname, NULL);
	int l = 0, i = 0;
	const char *p, *end;

	if (!prop)
		return -EINVAL;
	p = of_property_get_value(prop);
	if (!p)
		return -ENODATA;
	end = p + prop->length;

	for (i = 0; p < end && (!out_strs || i < skip + sz); i++, p += l) {
		l = strnlen(p, end - p) + 1;
		if (p + l > end)
			return -EILSEQ;
		if (out_strs && i >= skip)
			*out_strs++ = p;
	}
	i -= skip;
	return i <= 0 ? -ENODATA : i;
}

static void __of_print_property_prefixed(const struct property *p, int indent,
					 unsigned maxpropsize, const char *prefix)
{
	unsigned length;

	printf("%s%*s%s", prefix, indent * 8, "", p->name);

	length = min_t(unsigned, p->length, maxpropsize);
	if (length) {
		printf(" = ");
		of_print_property(of_property_get_value(p), length);
	}
	if (length != p->length)
		printf(" /* %u more bytes omitted */", p->length - length);

	printf(";\n");
}

static int __of_print_nodes(struct device_node *node, int indent,
			    unsigned maxpropsize, const char *prefix)
{
	struct device_node *n;
	struct property *p;
	int ret;

	if (!node)
		return 0;

	if (!prefix)
		prefix = "";

	printf("%s%*s%s%s\n", prefix, indent * 8, "", node->name, node->name ? " {" : "{");

	list_for_each_entry(p, &node->properties, list)
		__of_print_property_prefixed(p, indent + 1, maxpropsize, prefix);

	if (ctrlc())
		return -EINTR;

	list_for_each_entry(n, &node->children, parent_list) {
		ret = __of_print_nodes(n, indent + 1, maxpropsize, prefix);
		if (ret)
			return ret;
	}

	printf("%s%*s};\n", prefix, indent * 8, "");
	return 0;
}

void of_print_nodes(struct device_node *node, int indent, unsigned maxpropsize)
{
	__of_print_nodes(node, indent, maxpropsize, NULL);
}

static void __of_print_property(struct property *p, int indent, unsigned maxpropsize)
{
	__of_print_property_prefixed(p, indent, maxpropsize, "");
}

void of_print_properties(struct device_node *node, unsigned maxpropsize)
{
	struct property *prop;

	list_for_each_entry(prop, &node->properties, list)
		__of_print_property(prop, 0, maxpropsize);
}

static int __of_print_parents(struct device_node *node)
{
	int indent;

	if (!node->parent)
		return 0;

	indent = __of_print_parents(node->parent);

	printf("%*s%s {\n", indent * 8, "", node->name);

	return indent + 1;
}

static void of_print_parents(struct device_node *node, int *printed)
{
	if (*printed)
		return;

	__of_print_parents(node);

	*printed = 1;
}

static void of_print_close(struct device_node *node, int *printed)
{
	int depth = 0, i, j;

	if (!*printed)
		return;

	while ((node = node->parent))
		depth++;

	for (i = depth; i > 0; i--) {
		for (j = 0; j + 1 < i; j++)
			printf("\t");
		printf("};\n");
	}
}

/**
 * of_diff - compare two device trees against each other
 * @a: The first device tree
 * @b: The second device tree
 * @indent: The initial indentation level when printing
 *
 * This function compares two device trees against each other and prints
 * a diff-like result.
 */
int of_diff(struct device_node *a, struct device_node *b, int indent)
{
	struct property *ap, *bp;
	struct device_node *ca, *cb;
	int printed = 0, diff = 0;
	bool silent = indent < 0;

	list_for_each_entry(ap, &a->properties, list) {
		bp = of_find_property(b, ap->name, NULL);
		if (!bp) {
			diff++;
			if (silent)
				continue;
			of_print_parents(a, &printed);
			printf("- ");
			__of_print_property(ap, indent, ~0);
			continue;
		}

		if (ap->length != bp->length || memcmp(of_property_get_value(ap), of_property_get_value(bp), bp->length)) {
			diff++;
			if (silent)
				continue;
			of_print_parents(a, &printed);
			printf("- ");
			__of_print_property(ap, indent, ~0);
			printf("+ ");
			__of_print_property(bp, indent, ~0);
		}
	}

	list_for_each_entry(bp, &b->properties, list) {
		ap = of_find_property(a, bp->name, NULL);
		if (!ap) {
			diff++;
			if (silent)
				continue;
			of_print_parents(a, &printed);
			printf("+ ");
			__of_print_property(bp, indent, ~0);
		}
	}

	for_each_child_of_node(a, ca) {
		cb = of_get_child_by_name(b, ca->name);
		if (cb) {
			diff += of_diff(ca, cb, silent ? indent : indent + 1);
		} else {
			diff++;
			if (silent)
				continue;
			of_print_parents(a, &printed);
			__of_print_nodes(ca, indent, ~0, "- ");
		}
	}

	for_each_child_of_node(b, cb) {
		if (!of_get_child_by_name(a, cb->name)) {
			diff++;
			if (silent)
				continue;
			of_print_parents(a, &printed);
			__of_print_nodes(cb, indent, ~0, "+ ");
		}
	}

	if (!silent)
		of_print_close(a, &printed);

	return diff;
}

struct device_node *of_new_node(struct device_node *parent, const char *name)
{
	struct device_node *node;

	node = xzalloc(sizeof(*node));
	node->parent = parent;
	if (parent)
		list_add_tail(&node->parent_list, &parent->children);

	INIT_LIST_HEAD(&node->children);
	INIT_LIST_HEAD(&node->properties);

	if (parent) {
		node->name = xstrdup_const(name);
		node->full_name = basprintf("%pOF/%s",
					      node->parent, name);
		list_add(&node->list, &parent->list);
	} else {
		node->name = xstrdup_const("");
		node->full_name = xstrdup("");
		INIT_LIST_HEAD(&node->list);
	}

	return node;
}

struct property *__of_new_property(struct device_node *node, const char *name,
				   void *data, int len)
{
	struct property *prop;

	prop = xzalloc(sizeof(*prop));
	prop->name = xstrdup_const(name);
	prop->length = len;
	prop->value = data;

	list_add_tail(&prop->list, &node->properties);

	return prop;
}

/**
 * of_new_property - Add a new property to a node
 * @node:	device node to which the property is added
 * @name:	Name of the new property
 * @data:	Value of the property (can be NULL)
 * @len:	Length of the value
 *
 * This adds a new property to a device node. @data is copied and no longer needed
 * after calling this function.
 *
 * Return: A pointer to the new property
 */
struct property *of_new_property(struct device_node *node, const char *name,
		const void *data, int len)
{
	char *buf;

	buf = xzalloc(len);
	if (data)
		memcpy(buf, data, len);

	return __of_new_property(node, name, buf, len);
}

/**
 * of_new_property_const - Add a new property to a node
 * @node:	device node to which the property is added
 * @name:	Name of the new property
 * @data:	Value of the property (can be NULL)
 * @len:	Length of the value
 *
 * This adds a new property to a device node. @data is used directly in the
 * property and must be valid until the property is deleted again or set to
 * another value. Normally you shouldn't use this function, use of_new_property()
 * instead.
 *
 * Return: A pointer to the new property
 */
struct property *of_new_property_const(struct device_node *node, const char *name,
		const void *data, int len)
{
	struct property *prop;

	prop = xzalloc(sizeof(*prop));
	prop->name = xstrdup(name);
	prop->length = len;
	prop->value_const = data;

	list_add_tail(&prop->list, &node->properties);

	return prop;
}

void of_delete_property(struct property *pp)
{
	if (!pp)
		return;

	list_del(&pp->list);

	free_const(pp->name);
	free(pp->value);
	free(pp);
}

struct property *of_rename_property(struct device_node *np,
				    const char *old_name, const char *new_name)
{
	struct property *pp;

	pp = of_find_property(np, old_name, NULL);
	if (!pp)
		return NULL;

	of_property_write_bool(np, new_name, false);

	free_const(pp->name);
	pp->name = xstrdup(new_name);
	return pp;
}

struct property *of_copy_property(const struct device_node *src,
				  const char *propname,
				  struct device_node *dst)
{
	struct property *prop;

	prop = of_find_property(src, propname, NULL);
	if (!prop)
		return NULL;

	if (of_property_present(dst, propname))
		return ERR_PTR(-EEXIST);

	return of_new_property(dst, propname,
			       of_property_get_value(prop), prop->length);
}
EXPORT_SYMBOL_GPL(of_copy_property);


/**
 * of_set_property - create a property for a given node
 * @node - the node
 * @name - the name of the property
 * @val - the value for the property
 * @len - the length of the properties value
 * @create - if true, the property is created if not existing already
 */
int of_set_property(struct device_node *np, const char *name, const void *val, int len,
		int create)
{
	struct property *pp = of_find_property(np, name, NULL);

	if (!np)
		return -ENOENT;

	if (!pp && !create)
		return -ENOENT;

	of_delete_property(pp);

	pp = of_new_property(np, name, val, len);
	if (!pp)
		return -ENOMEM;

	return 0;
}

int of_append_property(struct device_node *np, const char *name, const void *val, int len)
{
	struct property *pp;
	int orig_len;
	void *buf;

	if (!np)
		return -ENOENT;

	pp = of_find_property(np, name, NULL);
	if (!pp) {
		of_new_property(np, name, val, len);
		return 0;
	}

	orig_len = pp->length;
	buf = realloc(pp->value, orig_len + len);
	if (!buf)
		return -ENOMEM;

	memcpy(buf + orig_len, val, len);

	pp->value = buf;
	pp->length += len;

	if (pp->value_const) {
		memcpy(buf, pp->value_const, orig_len);
		pp->value_const = NULL;
	}

	return 0;
}

int of_prepend_property(struct device_node *np, const char *name, const void *val, int len)
{
	struct property *pp;
	const void *oldval;
	void *buf;
	int oldlen;

	pp = of_find_property(np, name, &oldlen);
	if (!pp) {
		of_new_property(np, name, val, len);
		return 0;
	}

	oldval = of_property_get_value(pp);

	buf = malloc(len + oldlen);
	if (!buf)
		return -ENOMEM;

	memcpy(buf, val, len);
	memcpy(buf + len, oldval, oldlen);

	free(pp->value);
	pp->value = buf;
	pp->length = len + oldlen;
	pp->value_const = NULL;

	return 0;
}

int of_property_sprintf(struct device_node *np,
			const char *propname, const char *fmt, ...)
{
	struct property *pp;
	struct va_format vaf;
	char *buf = NULL;
	va_list args;
	int len;

	if (!np)
		return -ENOENT;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	len = asprintf(&buf, "%pV", &vaf);
	va_end(args);

	if (len < 0)
		return -ENOMEM;

	len++; /* trailing NUL */

	pp = of_find_property(np, propname, NULL);
	of_delete_property(pp);

	__of_new_property(np, propname, buf, len);
	return len;
}

static int mem_bank_num;

int of_add_memory(struct device_node *node, bool dump)
{
	const char *device_type;
	struct resource res;
	int n = 0, ret;

	ret = of_property_read_string(node, "device_type", &device_type);
	if (ret || of_node_cmp(device_type, "memory"))
		return -ENXIO;

	while (!of_address_to_resource(node, n, &res)) {
		int err;
		n++;
		if (!resource_size(&res))
			continue;

		if (!of_device_is_available(node))
			continue;

		err = of_add_memory_bank(node, dump, mem_bank_num,
				res.start, resource_size(&res));
		if (err)
			ret = err;

		mem_bank_num++;
	}

	return ret;
}

const char *of_get_model(void)
{
	return of_model;
}

const struct of_device_id of_default_bus_match_table[] = {
	{
		.compatible = "simple-bus",
	}, {
		.compatible = "simple-pm-bus",
	}, {
		.compatible = "simple-mfd",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, of_default_bus_match_table);

static int of_probe_memory(void)
{
	struct device_node *memory = root_node;
	int ret = 0;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return 0;

	/* Parse all available node with "memory" device_type */
	while (1) {
		int err;

		memory = of_find_node_by_type(memory, "memory");
		if (!memory)
			break;

		err = of_add_memory(memory, false);
		if (err)
			ret = err;
	}

	return ret;
}
mem_initcall(of_probe_memory);

struct device *of_platform_root_device;

static void of_platform_device_create_root(struct device_node *np)
{
	struct device *dev;
	int ret;

	if (of_platform_root_device)
		return;

	dev = xzalloc(sizeof(*dev));
	dev->id = DEVICE_ID_SINGLE;
	dev->of_node = np;
	dev_set_name(dev, "machine");

	ret = platform_device_register(dev);
	if (WARN_ON(ret)) {
		free_device(dev);
		return;
	}

	of_platform_root_device = dev;
}

static const struct of_device_id reserved_mem_matches[] = {
	{ .compatible = "ramoops" },
	{ .compatible = "nvmem-rmem" },
	{}
};
MODULE_DEVICE_TABLE(of, reserved_mem_matches);

/**
 * of_probe - Probe unflattened device tree starting at of_get_root_node
 *
 * The function walks the device tree and creates devices as needed.
 * With care, it can be called more than once, but if you really need that,
 * consider first if deep probe would help instead.
 */
int of_probe(void)
{
	struct device_node *node;

	if(!root_node)
		return -ENODEV;

	/*
	 * We do this first thing, so board drivers can patch the device
	 * tree prior to device creation if needed.
	 */
	of_platform_device_create_root(root_node);

	/*
	 * Handle certain compatibles explicitly, since we don't want to create
	 * platform_devices for every node in /reserved-memory with a
	 * "compatible",
	 */
	for_each_matching_node(node, reserved_mem_matches)
		of_platform_device_create(node, NULL);

	node = of_find_node_by_path("/firmware");
	if (node)
		of_platform_populate(node, NULL, NULL);

	of_platform_populate(root_node, of_default_bus_match_table, NULL);

	return 0;
}

/**
 * of_create_node - create a new node including its parents
 * @path - the nodepath to create
 */
struct device_node *of_create_node(struct device_node *root, const char *path)
{
	char *slash, *p, *freep;
	struct device_node *tmp, *dn = root;

	if (*path != '/')
		return NULL;

	path++;

	p = freep = xstrdup(path);

	while (1) {
		if (!*p)
			goto out;

		slash = strchr(p, '/');
		if (slash)
			*slash = 0;

		tmp = of_get_child_by_name(dn, p);
		if (tmp)
			dn = tmp;
		else
			dn = of_new_node(dn, p);

		if (!dn)
			goto out;

		if (!slash)
			goto out;

		p = slash + 1;
	}
out:
	free(freep);

	return dn;
}

void of_merge_nodes(struct device_node *np, const struct device_node *other)
{
	struct device_node *child;
	struct property *pp;

	list_for_each_entry(pp, &other->properties, list)
		of_new_property(np, pp->name, pp->value, pp->length);

	for_each_child_of_node(other, child)
		of_copy_node(np, child);
}

struct device_node *of_copy_node(struct device_node *parent, const struct device_node *other)
{
	struct device_node *np;

	np = of_new_node(parent, other->name);
	np->phandle = other->phandle;

	of_merge_nodes(np, other);

	return np;
}

struct device_node *of_dup(const struct device_node *root)
{
	if (IS_ERR_OR_NULL(root))
		return ERR_CAST(root);

	return of_copy_node(NULL, root);
}

void of_delete_node(struct device_node *node)
{
	struct device_node *n, *nt;
	struct property *p, *pt;

	if (!node)
		return;

	if (node == root_node) {
		pr_err("Won't delete root device node\n");
		return;
	}

	list_for_each_entry_safe(p, pt, &node->properties, list)
		of_delete_property(p);

	list_for_each_entry_safe(n, nt, &node->children, parent_list)
		of_delete_node(n);

	if (node->parent) {
		list_del(&node->parent_list);
		list_del(&node->list);
	}

	free_const(node->name);
	free(node->full_name);
	free(node);
}

/*
 * of_find_node_by_chosen - Find a node given a chosen property pointing at it
 * @propname:   the name of the property containing a path or alias
 *              The function will lookup the first string in the property
 *              value up to the first : character or till \0.
 * @options     The Remainder (without : or \0 at the end) will be written
 *              to *options if not NULL.
 */
struct device_node *of_find_node_by_chosen(const char *propname,
					   const char **options)
{
	const char *value, *p;
	char *buf;
	struct device_node *dn;

	value = of_get_property(of_chosen, propname, NULL);
	if (!value)
		return NULL;

	p = strchrnul(value, ':');
	buf = xstrndup(value, p - value);

	dn = of_find_node_by_path_or_alias(NULL, buf);

	free(buf);

	if (options && *p)
		*options = p + 1;

	return dn;
}

struct device_node *of_get_stdoutpath(unsigned int *baudrate)
{
	const char *opts = NULL;
	struct device_node *dn;

	dn = of_find_node_by_chosen("stdout-path", &opts);
	if (!dn)
		dn = of_find_node_by_chosen("linux,stdout-path", &opts);
	if (!dn)
		return NULL;

	if (baudrate && opts) {
		unsigned rate = simple_strtoul(opts, NULL, 10);
		if (rate)
			*baudrate = rate;
	}

	return dn;
}

int of_device_is_stdout_path(struct device *dev, unsigned int *baudrate)
{
	unsigned int tmp = *baudrate;

	if (!dev || !dev->of_node || dev->of_node != of_get_stdoutpath(&tmp))
		return false;

	*baudrate = tmp;
	return true;
}

/**
 * of_add_initrd - add initrd properties to the devicetree
 * @root - the root node of the tree
 * @start - physical start address of the initrd image
 * @end - physical end address of the initrd image
 *
 * Add initrd properties to the devicetree, or, if end is 0,
 * delete them.
 *
 * Note that Linux interprets end differently than barebox. For Linux end points
 * to the first address after the memory occupied by the image while barebox
 * lets end pointing to the last occupied byte.
 */
int of_add_initrd(struct device_node *root, resource_size_t start,
		resource_size_t end)
{
	struct device_node *chosen;
	__be32 buf[2];

	chosen = of_create_node(root, "/chosen");
	if (!chosen)
		return -ENOMEM;

	if (end) {
		of_write_number(buf, start, 2);
		of_set_property(chosen, "linux,initrd-start", buf, 8, 1);
		of_write_number(buf, end + 1, 2);
		of_set_property(chosen, "linux,initrd-end", buf, 8, 1);
	} else {
		struct property *pp;

		pp = of_find_property(chosen, "linux,initrd-start", NULL);
		if (pp)
			of_delete_property(pp);

		pp = of_find_property(chosen, "linux,initrd-end", NULL);
		if (pp)
			of_delete_property(pp);
	}

	return 0;
}

/**
 * of_device_enable - enable a devicenode device
 * @node - the node to enable
 *
 * This deletes the status property of a devicenode effectively
 * enabling the device.
 */
int of_device_enable(struct device_node *node)
{
	struct property *pp;

	pp = of_find_property(node, "status", NULL);
	if (!pp)
		return 0;

	of_delete_property(pp);

	return 0;
}

/**
 * of_device_enable_path - enable a devicenode
 * @path - the nodepath to enable
 *
 * wrapper around of_device_enable taking the nodepath as argument
 */
int of_device_enable_path(const char *path)
{
	struct device_node *node;

	node = of_find_node_by_path(path);
	if (!node)
		return -ENODEV;

	return of_device_enable(node);
}

/**
 * of_device_enable_by_alias - enable a device node by alias
 * @alias - the alias of the device tree node to enable
 */
int of_device_enable_by_alias(const char *alias)
{
	struct device_node *node;

	node = of_find_node_by_alias(NULL, alias);
	if (!node)
		return -ENODEV;

	return of_device_enable(node);
}

/**
 * of_device_disable - disable a devicenode device
 * @node - the node to disable
 *
 * This sets the status of a devicenode to "disabled"
 */
int of_device_disable(struct device_node *node)
{
	return of_property_write_string(node, "status", "disabled");
}

/**
 * of_device_disable_path - disable a devicenode
 * @path - the nodepath to disable
 *
 * wrapper around of_device_disable taking the nodepath as argument
 */
int of_device_disable_path(const char *path)
{
	struct device_node *node;

	node = of_find_node_by_path(path);
	if (!node)
		return -ENODEV;

	return of_device_disable(node);
}

/**
 * of_device_disable_by_alias - disable a devicenode by alias
 * @alias - the alias of the device tree node to disable
 */
int of_device_disable_by_alias(const char *alias)
{
	struct device_node *node;

	node = of_find_node_by_alias(NULL, alias);
	if (!node)
		return -ENODEV;

	return of_device_disable(node);
}

/**
 * of_read_file - unflatten oftree file
 * @filename - path to file to unflatten its contents
 *
 * Returns the root node of the tree or an error pointer on error.
 */
struct device_node *of_read_file(const char *filename)
{
	void *fdt;
	size_t size;
	struct device_node *root;

	fdt = read_file(filename, &size);
	if (!fdt) {
		pr_err("unable to read %s: %m\n", filename);
		return ERR_PTR(-errno);
	}

	if (IS_ENABLED(CONFIG_FILETYPE) && file_detect_type(fdt, size) != filetype_oftree) {
		pr_err("%s is not a flat device tree file.\n", filename);
		root = ERR_PTR(-EINVAL);
		goto out;
	}

	root = of_unflatten_dtb(fdt, size);
out:
	free(fdt);

	return root;
}

/**
 * of_get_reproducible_name() - get a reproducible name of a node
 * @node: The node to get a name from
 *
 * This function constructs a reproducible name for a node. This name can be
 * used to find the same node in another device tree. The name is constructed
 * from different patterns which are appended to each other.
 * - If a node has no "reg" property, the name of the node is used in angle
 *   brackets, prepended with the result of the parent node
 * - If the parent node has a "ranges" property then the address in MMIO space
 *   is used in square brackets
 * - If a node has a "reg" property, but is not translatable in MMIO space then
 *   the start address is used in curly brackets, prepended with the result of
 *   the parent node.
 *
 * Returns a dynamically allocated string containing the name
 */
char *of_get_reproducible_name(struct device_node *node)
{
	const __be32 *reg;
	u64 addr;
	u64 offset;
	int na;
	char *str, *res;

	if (!node)
		return 0;

	reg = of_get_property(node, "reg", NULL);
        if (!reg) {
		str = of_get_reproducible_name(node->parent);
		res = basprintf("%s<%s>", str, node->name);
		free(str);
		return res;
	}

	if (node->parent && of_get_property(node->parent, "ranges", NULL)) {
		addr = of_translate_address(node, reg);
		return basprintf("[0x%llx]", addr);
	}

	na = of_n_addr_cells(node);

	/*
	 * Special workaround for the of partition binding. In the old binding
	 * the partitions are directly under the hardware devicenode whereas in
	 * the new binding the partitions are in an extra subnode with
	 * "fixed-partitions" compatible. We skip this extra subnode from the
	 * reproducible name to get the same name for both bindings.
	 */
	if (node->parent && of_node_is_fixed_partitions(node->parent))
		node = node->parent;

	offset = of_read_number(reg, na);

	str = of_get_reproducible_name(node->parent);
	res = basprintf("%s{%llx}", str, offset);
	free(str);

	return res;
}

struct device_node *of_find_node_by_reproducible_name(struct device_node *from,
						      const char *name)
{
	struct device_node *np;

	of_tree_for_each_node_from(np, from) {
		char *rep = of_get_reproducible_name(np);
		int res;

		res = of_node_cmp(rep, name);

		free(rep);

		if (!res)
			return np;
	}
	return NULL;
}

struct device_node *of_get_node_by_reproducible_name(struct device_node *dstroot,
						     struct device_node *srcnp)
{
	struct device_node *dstnp;
	char *name;

	name = of_get_reproducible_name(srcnp);
	dstnp = of_find_node_by_reproducible_name(dstroot, name);
	free(name);

	return dstnp;
}

/**
 * of_graph_parse_endpoint() - parse common endpoint node properties
 * @node: pointer to endpoint device_node
 * @endpoint: pointer to the OF endpoint data structure
 */
int of_graph_parse_endpoint(const struct device_node *node,
			    struct of_endpoint *endpoint)
{
	struct device_node *port_node = of_get_parent(node);

	if (!port_node)
		pr_warn("%s(): endpoint %pOF has no parent node\n",
			__func__, node);

	memset(endpoint, 0, sizeof(*endpoint));

	endpoint->local_node = node;
	/*
	 * It doesn't matter whether the two calls below succeed.
	 * If they don't then the default value 0 is used.
	 */
	of_property_read_u32(port_node, "reg", &endpoint->port);
	of_property_read_u32(node, "reg", &endpoint->id);

	return 0;
}
EXPORT_SYMBOL(of_graph_parse_endpoint);

/**
 * of_graph_get_port_by_id() - get the port matching a given id
 * @parent: pointer to the parent device node
 * @id: id of the port
 *
 * Return: A 'port' node pointer with refcount incremented.
 */
struct device_node *of_graph_get_port_by_id(struct device_node *parent, u32 id)
{
	struct device_node *port, *node = of_get_child_by_name(parent, "ports");

	if (node)
		parent = node;

	for_each_child_of_node(parent, port) {
		u32 port_id = 0;

		if (strncmp(port->name, "port", 4) != 0)
			continue;
		of_property_read_u32(port, "reg", &port_id);
		if (id == port_id)
			return port;
	}

	return NULL;
}
EXPORT_SYMBOL(of_graph_get_port_by_id);

/**
 * of_graph_get_next_endpoint() - get next endpoint node
 * @parent: pointer to the parent device node
 * @prev: previous endpoint node, or NULL to get first
 *
 * Return: An 'endpoint' node pointer with refcount incremented. Refcount
 * of the passed @prev node is decremented.
 */
struct device_node *of_graph_get_next_endpoint(const struct device_node *parent,
					struct device_node *prev)
{
	struct device_node *endpoint;
	struct device_node *port;

	if (!parent)
		return NULL;

	/*
	 * Start by locating the port node. If no previous endpoint is specified
	 * search for the first port node, otherwise get the previous endpoint
	 * parent port node.
	 */
	if (!prev) {
		struct device_node *node;

		node = of_get_child_by_name(parent, "ports");
		if (node)
			parent = node;

		port = of_get_child_by_name_stem(parent, "port");
		if (!port) {
			pr_err("%s(): no port node found in %pOF\n",
			       __func__, parent);
			return NULL;
		}
	} else {
		port = of_get_parent(prev);
		if (!port) {
			pr_warn("%s(): endpoint %pOF has no parent node\n",
			      __func__, prev);
			return NULL;
		}
	}

	while (1) {
		/*
		 * Now that we have a port node, get the next endpoint by
		 * getting the next child. If the previous endpoint is NULL this
		 * will return the first child.
		 */
		endpoint = of_get_next_child(port, prev);
		if (endpoint)
			return endpoint;

		/* No more endpoints under this port, try the next one. */
		prev = NULL;

		do {
			port = of_get_next_child(parent, port);
			if (!port)
				return NULL;
		} while (port->name && !of_node_name_eq(port, "port"));
	}
}
EXPORT_SYMBOL(of_graph_get_next_endpoint);

/**
 * of_graph_get_endpoint_by_regs() - get endpoint node of specific identifiers
 * @parent: pointer to the parent device node
 * @port_reg: identifier (value of reg property) of the parent port node
 * @reg: identifier (value of reg property) of the endpoint node
 *
 * Return: An 'endpoint' node pointer which is identified by reg and at the same
 * is the child of a port node identified by port_reg. reg and port_reg are
 * ignored when they are -1. Use of_node_put() on the pointer when done.
 */
struct device_node *of_graph_get_endpoint_by_regs(
	const struct device_node *parent, int port_reg, int reg)
{
	struct of_endpoint endpoint;
	struct device_node *node = NULL;

	for_each_endpoint_of_node(parent, node) {
		of_graph_parse_endpoint(node, &endpoint);
		if (((port_reg == -1) || (endpoint.port == port_reg)) &&
			((reg == -1) || (endpoint.id == reg)))
			return node;
	}
	return NULL;
}
EXPORT_SYMBOL(of_graph_get_endpoint_by_regs);

/**
 * of_graph_get_remote_port_parent() - get remote port's parent node
 * @node: pointer to a local endpoint device_node
 *
 * Return: Remote device node associated with remote endpoint node linked
 *	   to @node.
 */
struct device_node *of_graph_get_remote_port_parent(
			       const struct device_node *node)
{
	struct device_node *np;
	unsigned int depth;

	/* Get remote endpoint node. */
	np = of_parse_phandle(node, "remote-endpoint", 0);

	/* Walk 3 levels up only if there is 'ports' node. */
	for (depth = 3; depth && np; depth--) {
		np = np->parent;
		if (depth == 2 && of_node_cmp(np->name, "ports"))
			break;
	}
	return np;
}
EXPORT_SYMBOL(of_graph_get_remote_port_parent);

/**
 * of_graph_get_remote_port() - get remote port node
 * @node: pointer to a local endpoint device_node
 *
 * Return: Remote port node associated with remote endpoint node linked
 *	   to @node.
 */
struct device_node *of_graph_get_remote_port(const struct device_node *node)
{
	struct device_node *np;

	/* Get remote endpoint node. */
	np = of_parse_phandle(node, "remote-endpoint", 0);
	if (!np)
		return NULL;
	return np->parent;
}
EXPORT_SYMBOL(of_graph_get_remote_port);

/**
 * of_graph_get_remote_node() - get remote parent device_node for given port/endpoint
 * @node: pointer to parent device_node containing graph port/endpoint
 * @port: identifier (value of reg property) of the parent port node
 * @endpoint: identifier (value of reg property) of the endpoint node
 *
 * Return: Remote device node associated with remote endpoint node linked
 * to @node. Use of_node_put() on it when done.
 */
struct device_node *of_graph_get_remote_node(const struct device_node *node,
					     u32 port, u32 endpoint)
{
	struct device_node *endpoint_node, *remote;

	endpoint_node = of_graph_get_endpoint_by_regs(node, port, endpoint);
	if (!endpoint_node) {
		pr_debug("no valid endpoint (%d, %d) for node %pOF\n",
			 port, endpoint, node);
		return NULL;
	}

	remote = of_graph_get_remote_port_parent(endpoint_node);
	of_node_put(endpoint_node);
	if (!remote) {
		pr_debug("no valid remote node\n");
		return NULL;
	}

	if (!of_device_is_available(remote)) {
		pr_debug("not available for remote node\n");
		of_node_put(remote);
		return NULL;
	}

	return remote;
}
EXPORT_SYMBOL(of_graph_get_remote_node);

int of_graph_port_is_available(struct device_node *node)
{
	struct device_node *endpoint;
	int available = 0;

	for_each_child_of_node(node, endpoint) {
		struct device_node *remote_parent;

		remote_parent = of_graph_get_remote_port_parent(endpoint);
		if (!remote_parent)
			continue;

		if (!of_device_is_available(remote_parent))
			continue;

		available = 1;
	}

	return available;
}
EXPORT_SYMBOL(of_graph_port_is_available);

/**
 * of_alias_from_compatible - Lookup appropriate alias for a device node
 *			      depending on compatible
 * @node:	pointer to a device tree node
 * @alias:	Pointer to buffer that alias value will be copied into
 * @len:	Length of alias value
 *
 * Based on the value of the compatible property, this routine will attempt
 * to choose an appropriate alias value for a particular device tree node.
 * It does this by stripping the manufacturer prefix (as delimited by a ',')
 * from the first entry in the compatible list property.
 *
 * Note: The matching on just the "product" side of the compatible is a relic
 * from I2C and SPI. Please do not add any new user.
 *
 * Return: This routine returns 0 on success, <0 on failure.
 */
int of_alias_from_compatible(const struct device_node *node, char *alias, int len)
{
	const char *compatible, *p;
	int cplen;

	compatible = of_get_property(node, "compatible", &cplen);
	if (!compatible || strlen(compatible) > cplen)
		return -ENODEV;
	p = strchr(compatible, ',');
	strscpy(alias, p ? p + 1 : compatible, len);
	return 0;
}
EXPORT_SYMBOL_GPL(of_alias_from_compatible);

/**
 * of_get_machine_compatible - get first compatible string from the root node.
 *
 * Returns the string or NULL.
 */
const char *of_get_machine_compatible(void)
{
	struct property *prop;
	const char *name, *p;

	if (!root_node)
		return NULL;

	prop = of_find_property(root_node, "compatible", NULL);
	name = of_prop_next_string(prop, NULL);
	if (!name)
		return NULL;

	p = strchr(name, ',');
	return nonempty(p ? p + 1 : name);
}
EXPORT_SYMBOL(of_get_machine_compatible);

static int of_init_early_vars(void)
{
	struct device_node *bootsource;

	if (!IS_ENABLED(CONFIG_BAREBOX_DT_2ND))
		return 0;

	bootsource = of_find_node_by_chosen("bootsource", NULL);
	if (bootsource)
		bootsource_of_node_set(bootsource);

	return 0;
}
postcore_initcall(of_init_early_vars);

static int of_init_late_vars(void)
{
	const char *name;

	name = of_get_machine_compatible();
	barebox_set_hostname_no_overwrite(name ?: "barebox");

	return 0;
}
late_initcall(of_init_late_vars);

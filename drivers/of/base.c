/*
 * base.c - basic devicetree functions
 *
 * Copyright (c) 2012 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <init.h>
#include <memory.h>
#include <sizes.h>
#include <linux/ctype.h>
#include <linux/amba/bus.h>
#include <linux/err.h>

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

static LIST_HEAD(phandle_list);

struct device_node *root_node;

struct device_node *of_aliases;

int of_n_addr_cells(struct device_node *np)
{
	const __be32 *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#address-cells", NULL);
		if (ip)
			return be32_to_cpup(ip);
	} while (np->parent);
	/* No #address-cells property for the root node */
	return OF_ROOT_NODE_ADDR_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_addr_cells);

int of_n_size_cells(struct device_node *np)
{
	const __be32 *ip;

	do {
		if (np->parent)
			np = np->parent;
		ip = of_get_property(np, "#size-cells", NULL);
		if (ip)
			return be32_to_cpup(ip);
	} while (np->parent);
	/* No #size-cells property for the root node */
	return OF_ROOT_NODE_SIZE_CELLS_DEFAULT;
}
EXPORT_SYMBOL(of_n_size_cells);

static void of_bus_default_count_cells(struct device_node *dev,
				       int *addrc, int *sizec)
{
	if (addrc)
		*addrc = of_n_addr_cells(dev);
	if (sizec)
		*sizec = of_n_size_cells(dev);
}

static void of_bus_count_cells(struct device_node *dev,
			int *addrc, int *sizec)
{
	of_bus_default_count_cells(dev, addrc, sizec);
}

struct property *of_find_property(const struct device_node *node, const char *name)
{
	struct property *p;

	if (!node)
		return NULL;

	list_for_each_entry(p, &node->properties, list)
		if (!strcmp(p->name, name))
			return p;
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
	pr_debug("adding DT alias:%s: stem=%s id=%i node=%s\n",
		 ap->alias, ap->stem, ap->id, np->full_name);
}

/**
 * of_alias_scan - Scan all properties of 'aliases' node
 *
 * The function scans all the properties of 'aliases' node and populates
 * the global lookup table with the properties.  It returns the
 * number of alias_prop found, or error code in error case.
 */
static void of_alias_scan(void)
{
	struct property *pp;
	struct alias_prop *app, *tmp;

	list_for_each_entry_safe(app, tmp, &aliases_lookup, link)
		free(app);

	INIT_LIST_HEAD(&aliases_lookup);

	of_aliases = of_find_node_by_path(root_node, "/aliases");
	if (!of_aliases)
		return;

	list_for_each_entry(pp, &of_aliases->properties, list) {
		const char *start = pp->name;
		const char *end = start + strlen(start);
		struct device_node *np;
		struct alias_prop *ap;
		int id, len;

		/* Skip those we do not want to proceed */
		if (!strcmp(pp->name, "name") ||
		    !strcmp(pp->name, "phandle") ||
		    !strcmp(pp->name, "linux,phandle"))
			continue;

		np = of_find_node_by_path(root_node, pp->value);
		if (!np)
			continue;

		/* walk the alias backwards to extract the id and work out
		 * the 'stem' string */
		while (isdigit(*(end-1)) && end > start)
			end--;
		len = end - start;

		id = simple_strtol(end, 0, 10);
		if (id < 0)
			continue;

		/* Allocate an alias_prop with enough space for the stem */
		ap = xzalloc(sizeof(*ap) + len + 1);
		if (!ap)
			continue;
		ap->alias = start;
		of_alias_add(ap, np, id, start, len);
	}
}

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
		if (strcmp(app->stem, stem) != 0)
			continue;

		if (np == app->np) {
			id = app->id;
			break;
		}
	}

	return id;
}
EXPORT_SYMBOL_GPL(of_alias_get_id);

u64 of_translate_address(struct device_node *node, const __be32 *in_addr)
{
	struct property *p;
	u64 addr = be32_to_cpu(*in_addr);

	while (1) {
		int na, nc;

		if (!node->parent)
			return addr;

		node = node->parent;
		p = of_find_property(node, "ranges");
		if (!p && node->parent)
			return OF_BAD_ADDR;
		of_bus_count_cells(node, &na, &nc);
		if (na != 1 || nc != 1) {
			printk("%s: #size-cells != 1 or #address-cells != 1 "
					"currently not supported\n", node->name);
			return OF_BAD_ADDR;
		}
	}
}
EXPORT_SYMBOL(of_translate_address);

/*
 * of_find_node_by_phandle - Find a node given a phandle
 * @handle:    phandle of the node to find
 */
struct device_node *of_find_node_by_phandle(phandle phandle)
{
	struct device_node *node;

	list_for_each_entry(node, &phandle_list, phandles)
		if (node->phandle == phandle)
			return node;
	return NULL;
}
EXPORT_SYMBOL(of_find_node_by_phandle);

/*
 * Find a property with a given name for a given node
 * and return the value.
 */
const void *of_get_property(const struct device_node *np, const char *name,
			 int *lenp)
{
	struct property *pp = of_find_property(np, name);

	if (!pp)
		return NULL;

	if (lenp)
		*lenp = pp->length;

	return pp ? pp->value : NULL;
}
EXPORT_SYMBOL(of_get_property);

/** Checks if the given "compat" string matches one of the strings in
 * the device's "compatible" property
 */
int of_device_is_compatible(const struct device_node *device,
		const char *compat)
{
	const char *cp;
	int cplen, l;

	cp = of_get_property(device, "compatible", &cplen);
	if (cp == NULL)
		return 0;
	while (cplen > 0) {
		if (strcmp(cp, compat) == 0)
			return 1;
		l = strlen(cp) + 1;
		cp += l;
		cplen -= l;
	}

	return 0;
}
EXPORT_SYMBOL(of_device_is_compatible);

int of_match(struct device_d *dev, struct driver_d *drv)
{
	struct of_device_id *id;

	id = drv->of_compatible;

	while (id->compatible) {
		if (of_device_is_compatible(dev->device_node, id->compatible) == 1) {
			dev->of_id_entry = id;
			return 0;
		}
		id++;
	}

	return 1;
}
EXPORT_SYMBOL(of_match);

/**
 * of_property_read_u32_array - Find and read an array of 32 bit integers
 * from a property.
 *
 * @np:		device node from which the property value is to be read.
 * @propname:	name of the property to be searched.
 * @out_value:	pointer to return value, modified only if return value is 0.
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
	struct property *prop = of_find_property(np, propname);
	const __be32 *val;

	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	if ((sz * sizeof(*out_values)) > prop->length)
		return -EOVERFLOW;

	val = prop->value;
	while (sz--)
		*out_values++ = be32_to_cpup(val++);
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_u32_array);

int of_property_write_u32_array(struct device_node *np,
				const char *propname, const u32 *values,
				size_t sz)
{
	struct property *prop = of_find_property(np, propname);
	__be32 *val;

	if (!prop)
		prop = of_new_property(np, propname, NULL, 0);
	if (!prop)
		return -ENOMEM;

	free(prop->value);

	prop->value = malloc(sizeof(__be32) * sz);
	if (!prop->value)
		return -ENOMEM;

	val = prop->value;

	while (sz--)
		*val++ = cpu_to_be32(*values++);
	return 0;
}

/**
 * of_parse_phandles_with_args - Find a node pointed by phandle in a list
 * @np:		pointer to a device tree node containing a list
 * @list_name:	property name that contains a list
 * @cells_name:	property name that specifies phandles' arguments count
 * @index:	index of a phandle to parse out
 * @out_node:	optional pointer to device_node struct pointer (will be filled)
 * @out_args:	optional pointer to arguments pointer (will be filled)
 *
 * This function is useful to parse lists of phandles and their arguments.
 * Returns 0 on success and fills out_node and out_args, on error returns
 * appropriate errno value.
 *
 * Example:
 *
 * phandle1: node1 {
 * 	#list-cells = <2>;
 * }
 *
 * phandle2: node2 {
 * 	#list-cells = <1>;
 * }
 *
 * node3 {
 * 	list = <&phandle1 1 2 &phandle2 3>;
 * }
 *
 * To get a device_node of the `node2' node you may call this:
 * of_parse_phandles_with_args(node3, "list", "#list-cells", 2, &node2, &args);
 */
int of_parse_phandles_with_args(struct device_node *np, const char *list_name,
				const char *cells_name, int index,
				struct device_node **out_node,
				const void **out_args)
{
	int ret = -EINVAL;
	const __be32 *list;
	const __be32 *list_end;
	int size;
	int cur_index = 0;
	struct device_node *node = NULL;
	const void *args = NULL;

	list = of_get_property(np, list_name, &size);
	if (!list) {
		ret = -ENOENT;
		goto err0;
	}
	list_end = list + size / sizeof(*list);

	while (list < list_end) {
		const __be32 *cells;
		phandle phandle;

		phandle = be32_to_cpup(list++);
		args = list;

		/* one cell hole in the list = <>; */
		if (!phandle)
			goto next;

		node = of_find_node_by_phandle(phandle);
		if (!node) {
			pr_debug("%s: could not find phandle %d\n",
				 np->full_name, phandle);
			goto err0;
		}

		cells = of_get_property(node, cells_name, &size);
		if (!cells || size != sizeof(*cells)) {
			pr_debug("%s: could not get %s for %s\n",
				 np->full_name, cells_name, node->full_name);
			goto err1;
		}

		list += be32_to_cpup(cells);
		if (list > list_end) {
			pr_debug("%s: insufficient arguments length\n",
				 np->full_name);
			goto err1;
		}
next:
		if (cur_index == index)
			break;

		node = NULL;
		args = NULL;
		cur_index++;
	}

	if (!node) {
		/*
		 * args w/o node indicates that the loop above has stopped at
		 * the 'hole' cell. Report this differently.
		 */
		if (args)
			ret = -EEXIST;
		else
			ret = -ENOENT;
		goto err0;
	}

	if (out_node)
		*out_node = node;
	if (out_args)
		*out_args = args;

	return 0;
err1:
err0:
	pr_debug("%s failed with status %d\n", __func__, ret);
	return ret;
}
EXPORT_SYMBOL(of_parse_phandles_with_args);

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
 *	of_find_node_by_path - Find a node matching a full OF path
 *	@root:	The root node of this tree
 *	@path:	The full path to match
 *
 *	Returns a node pointer with refcount incremented, use
 *	of_node_put() on it when done.
 */
struct device_node *of_find_node_by_path(struct device_node *root, const char *path)
{
	char *slash, *p, *freep;
	struct device_node *dn = root;

	if (*path != '/')
		return NULL;

	path++;

	freep = p = xstrdup(path);

	while (1) {
		if (!*p)
			goto out;

		slash = strchr(p, '/');
		if (slash)
			*slash = 0;

		dn = of_find_child_by_name(dn, p);
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
EXPORT_SYMBOL(of_find_node_by_path);

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
	struct property *prop = of_find_property(np, propname);
	if (!prop)
		return -EINVAL;
	if (!prop->value)
		return -ENODATA;
	if (strnlen(prop->value, prop->length) >= prop->length)
		return -EILSEQ;
	*out_string = prop->value;
	return 0;
}
EXPORT_SYMBOL_GPL(of_property_read_string);

struct device_node *of_get_root_node(void)
{
	return root_node;
}

int of_set_root_node(struct device_node *node)
{
	if (node && root_node)
		return -EBUSY;

	root_node = node;

	of_alias_scan();

	return 0;
}

static int of_node_disabled(struct device_node *node)
{
	struct property *p;

	p = of_find_property(node, "status");
	if (p) {
		if (!strcmp("disabled", p->value))
			return 1;
	}
	return 0;
}

void of_print_nodes(struct device_node *node, int indent)
{
	struct device_node *n;
	struct property *p;
	int i;

	if (!node)
		return;

	if (of_node_disabled(node))
		return;

	for (i = 0; i < indent; i++)
		printf("\t");

	printf("%s%s\n", node->name, node->name ? " {" : "{");

	list_for_each_entry(p, &node->properties, list) {
		for (i = 0; i < indent + 1; i++)
			printf("\t");
		printf("%s: ", p->name);
		of_print_property(p->value, p->length);
		printf("\n");
	}

	list_for_each_entry(n, &node->children, parent_list) {
		of_print_nodes(n, indent + 1);
	}

	for (i = 0; i < indent; i++)
		printf("\t");
	printf("};\n");
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
		node->name = xstrdup(name);
		node->full_name = asprintf("%s/%s", node->parent->full_name, name);
		list_add(&node->list, &parent->list);
	} else {
		node->name = xstrdup("");
		node->full_name = xstrdup("");
		INIT_LIST_HEAD(&node->list);
	}

	return node;
}

struct property *of_new_property(struct device_node *node, const char *name,
		const void *data, int len)
{
	struct property *prop;

	prop = xzalloc(sizeof(*prop));

	prop->name = strdup(name);
	prop->length = len;
	if (len) {
		prop->value = xzalloc(len);
		memcpy(prop->value, data, len);
	}

	list_add_tail(&prop->list, &node->properties);

	return prop;
}

void of_delete_property(struct property *pp)
{
	list_del(&pp->list);

	free(pp->name);
	free(pp->value);
	free(pp);
}

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
	struct property *pp;

	if (!np)
		return -ENOENT;

	pp = of_find_property(np, name);
	if (pp) {
		void *data;

		free(pp->value);
		data = xzalloc(len);
		memcpy(data, val, len);
		pp->value = data;
		pp->length = len;
	} else {
		if (!create)
			return -ENOENT;

		pp = of_new_property(np, name, val, len);
	}

	return 0;
}

static struct device_d *add_of_amba_device(struct device_node *node)
{
	struct amba_device *dev;
	char *name, *at;

	dev = xzalloc(sizeof(*dev));

	name = xstrdup(node->name);
	at = strchr(name, '@');
	if (at) {
		*at = 0;
		snprintf(dev->dev.name, MAX_DRIVER_NAME, "%s.%s", at + 1, name);
	} else {
		strncpy(dev->dev.name, node->name, MAX_DRIVER_NAME);
	}

	dev->dev.id = DEVICE_ID_SINGLE;
	memcpy(&dev->res, &node->resource[0], sizeof(struct resource));
	dev->dev.resource = node->resource;
	dev->dev.num_resources = 1;
	dev->dev.device_node = node;
	node->device = &dev->dev;

	of_property_read_u32(node, "arm,primecell-periphid", &dev->periphid);

	debug("register device 0x%08x\n", node->resource[0].start);

	amba_device_add(dev);

	free(name);

	return &dev->dev;
}

static struct device_d *add_of_platform_device(struct device_node *node)
{
	struct device_d *dev;
	char *name, *at;

	dev = xzalloc(sizeof(*dev));

	name = xstrdup(node->name);
	at = strchr(name, '@');
	if (at) {
		*at = 0;
		snprintf(dev->name, MAX_DRIVER_NAME, "%s.%s", at + 1, name);
	} else {
		strncpy(dev->name, node->name, MAX_DRIVER_NAME);
	}

	dev->id = DEVICE_ID_SINGLE;
	dev->resource = node->resource;
	dev->num_resources = 1;
	dev->device_node = node;
	node->device = dev;

	debug("register device 0x%08x\n", node->resource[0].start);

	platform_device_register(dev);

	free(name);

	return dev;
}

static struct device_d *add_of_device(struct device_node *node)
{
	const struct property *cp;

	if (of_node_disabled(node))
		return NULL;

	cp = of_get_property(node, "compatible", NULL);
	if (!cp)
		return NULL;

	if (IS_ENABLED(CONFIG_ARM_AMBA) &&
	    of_device_is_compatible(node, "arm,primecell") == 1)
		return add_of_amba_device(node);
	else
		return add_of_platform_device(node);
}
EXPORT_SYMBOL(add_of_device);

static u64 dt_mem_next_cell(int s, const __be32 **cellp)
{
	const __be32 *p = *cellp;

	*cellp = p + s;
	return of_read_number(p, s);
}

int of_add_memory(struct device_node *node, bool dump)
{
	int na, nc;
	const __be32 *reg, *endp;
	int len, r = 0, ret;
	static char str[6];
	const char *device_type;

	ret = of_property_read_string(node, "device_type", &device_type);
	if (ret)
		return -ENXIO;

	if (strcmp(device_type, "memory"))
		return -ENXIO;

	of_bus_count_cells(node, &na, &nc);

	reg = of_get_property(node, "reg", &len);
	if (!reg)
		return -EINVAL;

	endp = reg + (len / sizeof(__be32));

	while ((endp - reg) >= (na + nc)) {
		u64 base, size;

		base = dt_mem_next_cell(na, &reg);
		size = dt_mem_next_cell(nc, &reg);

		if (size == 0)
			continue;

		sprintf(str, "ram%d", r);

		barebox_add_memory_bank(str, base, size);

		if (dump)
			pr_info("%s: %s: 0x%llx@0x%llx\n", node->name, str, size, base);

		r++;
	}

	return 0;
}

static int add_of_device_resource(struct device_node *node)
{
	struct property *reg;
	u64 address, size;
	struct resource *res;
	struct device_d *dev;
	phandle phandle;
	int ret;

	ret = of_property_read_u32(node, "phandle", &phandle);
	if (!ret) {
		node->phandle = phandle;
		list_add_tail(&node->phandles, &phandle_list);
	}

	ret = of_add_memory(node, false);
	if (ret != -ENXIO)
		return ret;

	reg = of_find_property(node, "reg");
	if (!reg)
		return -ENODEV;

	address = of_translate_address(node, reg->value);
	if (address == OF_BAD_ADDR)
		return -EINVAL;

	size = be32_to_cpu(((u32 *)reg->value)[1]);

	/*
	 * A device may already be registered as platform_device.
	 * Instead of registering the same device again, just
	 * add this node to the existing device.
	 */
	for_each_device(dev) {
		if (!dev->resource)
			continue;
		if (dev->resource->start == address) {
			debug("connecting %s to %s\n", node->name, dev_name(dev));
			node->device = dev;
			dev->device_node = node;
			node->resource = dev->resource;
			return 0;
		}
	}

	res = xzalloc(sizeof(*res));
	res->start = address;
	res->end = address + size - 1;
	res->flags = IORESOURCE_MEM;

	node->resource = res;

	add_of_device(node);

	return 0;
}

void of_free(struct device_node *node)
{
	struct device_node *n, *nt;
	struct property *p, *pt;

	if (!node)
		return;

	list_for_each_entry_safe(p, pt, &node->properties, list) {
		list_del(&p->list);
		free(p->name);
		free(p->value);
		free(p);
	}

	list_for_each_entry_safe(n, nt, &node->children, parent_list) {
		of_free(n);
	}

	if (node->parent) {
		list_del(&node->parent_list);
		list_del(&node->list);
	}

	if (node->device)
		node->device->device_node = NULL;
	else
		free(node->resource);

	free(node->name);
	free(node->full_name);
	free(node);

	if (node == root_node)
		of_set_root_node(NULL);
}

static void __of_probe(struct device_node *node)
{
	struct device_node *n;

	if (node->device)
		return;

	add_of_device_resource(node);

	list_for_each_entry(n, &node->children, parent_list)
		__of_probe(n);
}

struct device_node *of_chosen;
const char *of_model;

const char *of_get_model(void)
{
	return of_model;
}

int of_probe(void)
{
	if(!root_node)
		return -ENODEV;

	of_chosen = of_find_node_by_path(root_node, "/chosen");
	of_property_read_string(root_node, "model", &of_model);

	__of_probe(root_node);

	return 0;
}

struct device_node *of_find_child_by_name(struct device_node *node, const char *name)
{
	struct device_node *_n;

	device_node_for_nach_child(node, _n)
		if (!strcmp(_n->name, name))
			return _n;

	return NULL;
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

		tmp = of_find_child_by_name(dn, p);
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

int of_device_is_stdout_path(struct device_d *dev)
{
	struct device_node *dn;
	const char *name;

	name = of_get_property(of_chosen, "linux,stdout-path", NULL);
	if (name == NULL)
		return 0;
	dn = of_find_node_by_path(root_node, name);

	if (!dn)
		return 0;

	if (dn == dev->device_node)
		return 1;

	return 0;
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

	chosen = of_find_node_by_path(root, "/chosen");
	if (!chosen)
		return -EINVAL;

	if (end) {
		of_write_number(buf, start, 2);
		of_set_property(chosen, "linux,initrd-start", buf, 8, 1);
		of_write_number(buf, end + 1, 2);
		of_set_property(chosen, "linux,initrd-end", buf, 8, 1);
	} else {
		struct property *pp;

		pp = of_find_property(chosen, "linux,initrd-start");
		if (pp)
			of_delete_property(pp);

		pp = of_find_property(chosen, "linux,initrd-end");
		if (pp)
			of_delete_property(pp);
	}

	return 0;
}

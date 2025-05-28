// SPDX-License-Identifier: GPL-2.0
#include <linux/libfdt.h>
#include <pbl.h>
#include <linux/printk.h>
#include <stdio.h>

static const __be32 *fdt_parse_reg(const __be32 *reg, uint32_t n,
				   uint64_t *val)
{
	int i;

	*val = 0;
	for (i = 0; i < n; i++)
		*val = (*val << 32) | fdt32_to_cpu(*reg++);

	return reg;
}

void fdt_find_mem(const void *fdt, unsigned long *membase, unsigned long *memsize)
{
	const __be32 *reg;
	int na, ns;
	uint64_t memsize64, membase64;
	int node, size;

	/* Make sure FDT blob is sane */
	if (fdt_check_header(fdt) != 0) {
		pr_err("Invalid device tree blob\n");
		goto err;
	}

	node = fdt_path_offset(fdt, "/");
	if (node < 0) {
		pr_err("Cannot find root node\n");
		goto err;
	}

	na = fdt_address_cells(fdt, node);
	if (na < 0) {
		pr_err("Cannot find #address-cells property");
		goto err;
	}

	ns = fdt_size_cells(fdt, node);
	if (ns < 0) {
		pr_err("Cannot find #size-cells property");
		goto err;
	}

	/* Find the memory range */
	node = fdt_node_offset_by_prop_value(fdt, -1, "device_type",
					     "memory", sizeof("memory"));
	if (node < 0) {
		pr_err("Cannot find memory node\n");
		goto err;
	}

	reg = fdt_getprop(fdt, node, "reg", &size);
	if (size < (na + ns) * sizeof(u32)) {
		pr_err("cannot get memory range\n");
		goto err;
	}

	/* get the memsize and truncate it to under 4G on 32 bit machines */
	reg = fdt_parse_reg(reg, na, &membase64);
	reg = fdt_parse_reg(reg, ns, &memsize64);

	*membase = membase64;
	*memsize = memsize64;

	return;
err:
	pr_err("No memory, cannot continue\n");
	while (1);
}

static int fdt_find_or_add_memory(void *fdt, int parentoffset, const char *name)
{
	int err;
	int node;

	node = fdt_subnode_offset(fdt, parentoffset, name);
	if (node != -FDT_ERR_NOTFOUND)
		return node;

	/* Create new memory node */
	node = fdt_add_subnode(fdt, parentoffset, name);
	if (node < 0)
		return node;
	err = fdt_setprop(fdt, node, "device_type", "memory", sizeof("memory"));
	if (err < 0)
		return err;

	return node;
}

int fdt_fixup_mem(void *fdt, unsigned long membase[], unsigned long memsize[],
		  size_t num)
{
	int node, root;
	int err;
	int i;

	err = fdt_check_header(fdt);
	if (err != 0) {
		pr_err("Invalid device tree blob: %s\n", fdt_strerror(err));
		return err;
	}

	root = fdt_path_offset(fdt, "/");
	if (root < 0) {
		pr_err("Cannot find root node: %s\n", fdt_strerror(root));
		return root;
	}

	/* Delete memory node without @address postfix */
	node = fdt_subnode_offset(fdt, root, "memory");
	if (node >= 0)
		fdt_del_node(fdt, node);

	for (i = 0; i < num; i++) {
		unsigned long base = membase[i];
		unsigned long size = memsize[i];
		char name[32];

		if (size == 0)
			continue;

		snprintf(name, sizeof(name), "memory@%lx", base);
		node = fdt_find_or_add_memory(fdt, root, name);
		if (!node) {
			pr_warn("%s: Failed to get node: %s\n",
				name, fdt_strerror(err));
			continue;
		}

		/* Add or rewrite the reg property */
		fdt_delprop(fdt, node, "reg");
		err = fdt_appendprop_addrrange(fdt, root, node, "reg",
					       base, size);
		if (err < 0) {
			pr_warn("%s: Failed to set reg property %lx %lx: %s\n",
				name, base, size, fdt_strerror(err));
			continue;
		}

		/* Remove status property to ensure the node is enabled */
		fdt_delprop(fdt, node, "status");
	}

	return err;
}

const void *fdt_device_get_match_data(const void *fdt, const char *nodepath,
				      const struct fdt_device_id ids[])
{
	int node, length;
	const char *list, *end;
	const struct fdt_device_id *id;

	node = fdt_path_offset(fdt, nodepath);
	if (node < 0)
		return NULL;

	list = fdt_getprop(fdt, node, "compatible", &length);
	if (!list)
		return NULL;

	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end)
			return NULL;

		for (id = ids; id->compatible; id++) {
			if (!strcasecmp(list, id->compatible))
				return id->data;
		}

		list += length;
	}

	return NULL;
}

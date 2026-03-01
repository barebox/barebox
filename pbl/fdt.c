// SPDX-License-Identifier: GPL-2.0
#include <compressed-dtb.h>
#include <linux/libfdt.h>
#include <pbl.h>
#include <linux/printk.h>
#include <stdio.h>
#include <uncompress.h>

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
		if (node < 0) {
			pr_warn("%s: Failed to get node: %s\n",
				name, fdt_strerror(node));
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

static int pbl_open_dtbz_into(const void *fdt, void *buf, int bufsize)
{
	const struct barebox_boarddata_compressed_dtb *compressed_dtb;
	int error;

	if (!fdt_blob_can_be_decompressed(fdt)) {
		pr_warn("DTB can't be decompressed\n");
		return -EINVAL;
	}

	compressed_dtb = fdt;

	if (IS_ENABLED(CONFIG_IMAGE_COMPRESSION_NONE)) {
		error = fdt_open_into(compressed_dtb->data, buf, bufsize);
		if (error) {
			pr_warn("Failed to open uncompressed DTB with %s\n",
				fdt_strerror(error));
			return -EINVAL;
		}
		return 0;
	}

	if (bufsize < compressed_dtb->datalen_uncompressed) {
		pr_warn("FDT buffer to small, min. %u bytes required\n",
			compressed_dtb->datalen_uncompressed);
		return -EINVAL;
	}

	/*
	 * No error handling required, since this will hang() if uncompress
	 * fails.
	 */
	pbl_dtbz_uncompress(buf, (void *)compressed_dtb->data,
			    compressed_dtb->datalen);

	if (!blob_is_fdt(buf)) {
		pr_warn("Failed to determine FDT type for uncompressed DTB\n");
		return -EINVAL;
	}

	/* Required to setup size data structures to allow runtime adaptions */
	error = fdt_open_into(buf, buf, bufsize);
	if (error) {
		pr_warn("Failed to open decompressed DTB with %s\n",
			fdt_strerror(error));
		return -EINVAL;
	}

	return 0;
}

int pbl_load_fdt(const void *fdt, void *dest, int destsize)
{
	if (destsize == 0 || !dest || !fdt) {
		pr_warn("Skip early FDT load, invalid input\n");
		return -EINVAL;
	}

	if (blob_is_fdt(fdt)) {
		int error;

		error = fdt_open_into(fdt, dest, destsize);
		if (error) {
			pr_warn("Failed to uncompressed DTB with %s\n",
				fdt_strerror(error));
			return -EINVAL;
		}
		return 0;
	} else if (blob_is_compressed_fdt(fdt)) {
		return pbl_open_dtbz_into(fdt, dest, destsize);
	}

	pr_warn("FDT detection failed\n");

	return -EINVAL;
}

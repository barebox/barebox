/*
 * dtb.c - flat devicetree functions
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <linux/err.h>

static inline uint32_t dt_struct_advance(struct fdt_header *f, uint32_t dt, int size)
{
	dt += size;
	dt = ALIGN(dt, 4);

	if (dt > f->off_dt_struct + f->size_dt_struct)
		return 0;

	return dt;
}

static inline char *dt_string(struct fdt_header *f, char *strstart, uint32_t ofs)
{
	if (ofs > f->size_dt_strings)
		return NULL;
	else
		return strstart + ofs;
}

/**
 * of_unflatten_dtb - unflatten a dtb binary blob
 * @root - node in which the fdt blob should be merged into or NULL
 * @infdt - the fdt blob to unflatten
 *
 * Parse a flat device tree binary blob and return a pointer to the
 * unflattened tree.
 */
struct device_node *of_unflatten_dtb(struct device_node *root, void *infdt)
{
	const void *nodep;	/* property node pointer */
	uint32_t tag;		/* tag */
	int  len;		/* length of the property */
	const struct fdt_property *fdt_prop;
	const char *pathp, *name;
	struct device_node *node = NULL;
	struct property *p;
	uint32_t dt_struct;
	struct fdt_node_header *fnh;
	void *dt_strings;
	struct fdt_header f;
	int ret, merge = 0;
	unsigned int maxlen;
	struct fdt_header *fdt = infdt;

	if (fdt->magic != cpu_to_fdt32(FDT_MAGIC)) {
		pr_err("bad magic: 0x%08x\n", fdt32_to_cpu(fdt->magic));
		return ERR_PTR(-EINVAL);
	}

	if (fdt->version != cpu_to_fdt32(17)) {
		pr_err("bad dt version: 0x%08x\n", fdt32_to_cpu(fdt->version));
		return ERR_PTR(-EINVAL);
	}

	f.totalsize = fdt32_to_cpu(fdt->totalsize);
	f.off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
	f.size_dt_struct = fdt32_to_cpu(fdt->size_dt_struct);
	f.off_dt_strings = fdt32_to_cpu(fdt->off_dt_strings);
	f.size_dt_strings = fdt32_to_cpu(fdt->size_dt_strings);

	if (f.off_dt_struct + f.size_dt_struct > f.totalsize) {
		pr_err("unflatten: dt size exceeds total size\n");
		return ERR_PTR(-ESPIPE);
	}

	if (f.off_dt_strings + f.size_dt_strings > f.totalsize) {
		pr_err("unflatten: string size exceeds total size\n");
		return ERR_PTR(-ESPIPE);
	}

	dt_struct = f.off_dt_struct;
	dt_strings = (void *)fdt + f.off_dt_strings;

	if (root) {
		pr_debug("unflatten: merging into existing tree\n");
		merge = 1;
	} else {
		root = of_new_node(NULL, NULL);
		if (!root)
			return ERR_PTR(-ENOMEM);
	}

	while (1) {
		tag = be32_to_cpu(*(uint32_t *)(infdt + dt_struct));

		switch (tag) {
		case FDT_BEGIN_NODE:
			fnh = infdt + dt_struct;
			pathp = name = fnh->name;
			maxlen = (unsigned long)fdt + f.off_dt_struct +
				f.size_dt_struct - (unsigned long)name;

			len = strnlen(name, maxlen + 1);
			if (len > maxlen) {
				ret = -ESPIPE;
				goto err;
			}

			dt_struct = dt_struct_advance(&f, dt_struct,
					sizeof(struct fdt_node_header) + len + 1);
			if (!dt_struct) {
				ret = -ESPIPE;
				goto err;
			}

			if (!node) {
				node = root;
			} else {
				if (merge)
					node = of_get_child_by_name(node,
								    pathp);
				if (!merge || !node)
					node = of_new_node(node, pathp);
			}

			break;

		case FDT_END_NODE:
			if (!node) {
				pr_err("unflatten: too many end nodes\n");
				ret = -EINVAL;
				goto err;
			}

			node = node->parent;

			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);
			if (!dt_struct) {
				ret = -ESPIPE;
				goto err;
			}

			break;

		case FDT_PROP:
			fdt_prop = infdt + dt_struct;
			len = fdt32_to_cpu(fdt_prop->len);
			nodep = fdt_prop->data;

			name = dt_string(&f, dt_strings, fdt32_to_cpu(fdt_prop->nameoff));
			if (!name) {
				ret = -ESPIPE;
				goto err;
			}

			dt_struct = dt_struct_advance(&f, dt_struct,
					sizeof(struct fdt_property) + len);
			if (!dt_struct) {
				ret = -ESPIPE;
				goto err;
			}

			p = NULL;
			if (merge)
				p = of_find_property(node, name, NULL);
			if (merge && p) {
				free(p->value);
				p->value = xzalloc(len);
				p->length = len;
				memcpy(p->value, nodep, len);
			} else {
				p = of_new_property(node, name, nodep, len);
				if (!strcmp(name, "phandle") && len == 4)
					node->phandle = be32_to_cpup(p->value);
			}

			break;

		case FDT_NOP:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);
			if (!dt_struct) {
				ret = -ESPIPE;
				goto err;
			}

			break;

		case FDT_END:
			return root;

		default:
			pr_err("unflatten: Unknown tag 0x%08X\n", tag);
			ret = -EINVAL;
			goto err;
		}
	}
err:
	of_delete_node(root);

	return ERR_PTR(ret);
}

struct fdt {
	void *dt;
	uint32_t dt_nextofs;
	uint32_t dt_size;
	char *strings;
	uint32_t str_nextofs;
	uint32_t str_size;
};

static inline uint32_t dt_next_ofs(uint32_t curofs, uint32_t len)
{
	return ALIGN(curofs + len, 4);
}

static int lstrcpy(char *dest, const char *src)
{
	int len = 0;
	int maxlen = 1023;

	while (*src) {
		*dest++ = *src++;
		len++;
		if (!maxlen)
			return -ENOSPC;
		maxlen--;
	}

	return len;
}

static void *memalign_realloc(void *orig, size_t oldsize, size_t newsize)
{
	int align;
	void *newbuf;

	/*
	 * ARM Linux uses a single 1MiB section (with 1MiB alignment)
	 * for mapping the devicetree, so we are not allowed to cross
	 * 1MiB boundaries. This got fixed in the Kernel since v3.8-rc5
	 */
	align = 1 << fls(newsize - 1);

	if (!orig)
		return memalign(align, newsize);

	newbuf = memalign(align, newsize);
	if (!newbuf) {
		free(orig);
		return NULL;
	}

	memcpy(newbuf, orig, oldsize);
	free(orig);
	memset(newbuf + oldsize, 0, newsize - oldsize);

	return newbuf;
}

static int fdt_ensure_space(struct fdt *fdt, int dtsize)
{
	/*
	 * We assume strings and names have a maximum length of 1024
	 * whereas properties can be longer. We allocate new memory
	 * if we have less than 1024 bytes (+ the property size left.
	 */
	if (fdt->str_size - fdt->str_nextofs < 1024) {
		fdt->strings = realloc(fdt->strings, fdt->str_size * 2);
		if (!fdt->strings)
			return -ENOMEM;
		fdt->str_size *= 2;
	}

	if (fdt->dt_size - fdt->dt_nextofs < 1024 + dtsize) {
		fdt->dt = memalign_realloc(fdt->dt, fdt->dt_size,
				fdt->dt_size * 2);
		if (!fdt->dt)
			return -ENOMEM;
		fdt->dt_size *= 2;
	}

	return 0;
}

static inline int dt_add_string(struct fdt *fdt, const char *str)
{
	uint32_t ret;
	int len;

	if (fdt_ensure_space(fdt, 0) < 0)
		return -ENOMEM;

	len = lstrcpy(fdt->strings + fdt->str_nextofs, str);
	if (len < 0)
		return -ENOSPC;

	ret = fdt->str_nextofs;

	fdt->str_nextofs += len + 1;

	return ret;
}

static int __of_flatten_dtb(struct fdt *fdt, struct device_node *node)
{
	struct property *p;
	struct device_node *n;
	int ret;
	unsigned int len;
	struct fdt_node_header *nh;

	if (fdt_ensure_space(fdt, 0) < 0)
		return -ENOMEM;

	nh = fdt->dt + fdt->dt_nextofs;
	nh->tag = cpu_to_fdt32(FDT_BEGIN_NODE);
	len = lstrcpy(nh->name, node->name);
	fdt->dt_nextofs = dt_next_ofs(fdt->dt_nextofs, 4 + len + 1);

	list_for_each_entry(p, &node->properties, list) {
		struct fdt_property *fp;

		if (fdt_ensure_space(fdt, p->length) < 0)
			return -ENOMEM;

		fp = fdt->dt + fdt->dt_nextofs;

		fp->tag = cpu_to_fdt32(FDT_PROP);
		fp->len = cpu_to_fdt32(p->length);
		fp->nameoff = cpu_to_fdt32(dt_add_string(fdt, p->name));
		memcpy(fp->data, p->value, p->length);
		fdt->dt_nextofs = dt_next_ofs(fdt->dt_nextofs,
				sizeof(struct fdt_property) + p->length);
	}

	list_for_each_entry(n, &node->children, parent_list) {
		ret = __of_flatten_dtb(fdt, n);
		if (ret)
			return ret;
	}

	nh = fdt->dt + fdt->dt_nextofs;
	nh->tag = cpu_to_fdt32(FDT_END_NODE);
	fdt->dt_nextofs = dt_next_ofs(fdt->dt_nextofs,
			sizeof(struct fdt_node_header));

	if (fdt_ensure_space(fdt, 0) < 0)
		return -ENOMEM;

	return 0;
}

/**
 * of_flatten_dtb - flatten a barebox internal devicetree to a dtb
 * @node - the root node of the tree to be unflattened
 */
void *of_flatten_dtb(struct device_node *node)
{
	int ret;
	struct fdt_header header = {};
	struct fdt fdt = {};
	uint32_t ofs;
	struct fdt_node_header *nh;

	header.magic = cpu_to_fdt32(FDT_MAGIC);
	header.version = cpu_to_fdt32(0x11);
	header.last_comp_version = cpu_to_fdt32(0x10);

	fdt.dt = xmemalign(SZ_64K, SZ_64K);
	fdt.dt_size = SZ_64K;

	fdt.strings = xzalloc(SZ_64K);
	fdt.str_size = SZ_64K;

	memset(fdt.dt, 0, SZ_64K);

	ofs = sizeof(struct fdt_header);

	header.off_mem_rsvmap = cpu_to_fdt32(ofs);
	ofs += sizeof(struct fdt_reserve_entry) * OF_MAX_RESERVE_MAP;

	fdt.dt_nextofs = ofs;

	ret = __of_flatten_dtb(&fdt, node);
	if (ret)
		goto out_free;
	nh = fdt.dt + fdt.dt_nextofs;
	nh->tag = cpu_to_fdt32(FDT_END);
	fdt.dt_nextofs = dt_next_ofs(fdt.dt_nextofs, sizeof(struct fdt_node_header));

	header.size_dt_strings = cpu_to_fdt32(fdt.str_nextofs);
	header.size_dt_struct = cpu_to_fdt32(fdt.dt_nextofs);

	header.off_dt_struct = cpu_to_fdt32(ofs);

	header.off_dt_strings = cpu_to_fdt32(fdt.dt_nextofs);

	if (fdt.dt_size - fdt.dt_nextofs < fdt.str_nextofs) {
		fdt.dt = memalign_realloc(fdt.dt, fdt.dt_size,
				fdt.dt_nextofs + fdt.str_nextofs);
		if (!fdt.dt)
			goto out_free;
	}

	memcpy(fdt.dt + fdt.dt_nextofs, fdt.strings, fdt.str_nextofs);

	header.totalsize = cpu_to_fdt32(fdt.dt_nextofs + fdt.str_nextofs);

	memcpy(fdt.dt, &header, sizeof(header));

	free(fdt.strings);

	return fdt.dt;

out_free:
	free(fdt.strings);
	free(fdt.dt);

	return NULL;
}

/*
 * The last entry is the zeroed sentinel, the one before is
 * reserved for the reservemap entry for the dtb itself.
 */
#define OF_MAX_FREE_RESERVE_MAP	(OF_MAX_RESERVE_MAP - 2)

static struct of_reserve_map of_reserve_map;

int of_add_reserve_entry(resource_size_t start, resource_size_t end)
{
	int e = of_reserve_map.num_entries;

	if (e >= OF_MAX_FREE_RESERVE_MAP)
		return -ENOSPC;

	of_reserve_map.start[e] = start;
	of_reserve_map.end[e] = end;
	of_reserve_map.num_entries++;

	return 0;
}

struct of_reserve_map *of_get_reserve_map(void)
{
	return &of_reserve_map;
}

void of_clean_reserve_map(void)
{
	of_reserve_map.num_entries = 0;
}

/**
 * fdt_add_reserve_map - Add reserve map entries to a devicetree binary
 * @__fdt: The devicetree blob
 *
 * This adds the reservemap entries previously colllected in
 * of_add_reserve_entry() to a devicetree binary blob. This also
 * adds the devicetree itself to the reserved list, so after calling
 * this function the tree should not be relocated anymore.
 */
void fdt_add_reserve_map(void *__fdt)
{
	struct fdt_header *fdt = __fdt;
	struct of_reserve_map *res = &of_reserve_map;
	struct fdt_reserve_entry *fdt_res =
		__fdt + be32_to_cpu(fdt->off_mem_rsvmap);
	int i;

	for (i = 0; i < res->num_entries; i++) {
		of_write_number(&fdt_res->address, res->start[i], 2);
		of_write_number(&fdt_res->size, res->end[i] - res->start[i] + 1,
				2);
		fdt_res++;
	}

	of_write_number(&fdt_res->address, (unsigned long)__fdt, 2);
	of_write_number(&fdt_res->size, be32_to_cpu(fdt->totalsize), 2);
	fdt_res++;

	of_write_number(&fdt_res->address, 0, 2);
	of_write_number(&fdt_res->size, 0, 2);
}

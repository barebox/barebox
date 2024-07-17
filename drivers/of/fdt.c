// SPDX-License-Identifier: GPL-2.0-only
/*
 * fdt.c - flat devicetree functions
 *
 * Copyright (c) 2013 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * based on Linux devicetree support
 */
#include <common.h>
#include <of.h>
#include <errno.h>
#include <malloc.h>
#include <init.h>
#include <memory.h>
#include <linux/sizes.h>
#include <linux/ctype.h>
#include <linux/log2.h>
#include <linux/overflow.h>
#include <linux/string_helpers.h>
#include <linux/err.h>

static inline bool __dt_ptr_ok(const struct fdt_header *fdt, const void *p,
				  unsigned elem_size, unsigned elem_align)
{
	if (!p || (const void *)fdt > p || !PTR_IS_ALIGNED(p, elem_align) ||
	     p + elem_size > (const void *)fdt + be32_to_cpu(fdt->totalsize)) {
		pr_err("unflatten: offset overflows or misaligns FDT\n");
		return false;
	}

	return true;
}
#define dt_ptr_ok(fdt, p) __dt_ptr_ok(fdt, p, sizeof(*(p)), __alignof__(*(p)))

static inline uint32_t dt_struct_advance(struct fdt_header *f, uint32_t dt, int size)
{
	dt += size;
	dt = ALIGN(dt, 4);

	if (dt > f->off_dt_struct + f->size_dt_struct)
		return 0;

	return dt;
}

static inline const char *dt_string(struct fdt_header *f, const char *strstart, uint32_t ofs)
{
	const char *str;

	if (ofs > f->size_dt_strings)
		return NULL;

	str = strstart + ofs;

	return string_is_terminated(str, f->size_dt_strings - ofs) ? str : NULL;
}

static int of_reservemap_num_entries(const struct fdt_header *fdt)
{
	/*
	 * FDT may violate spec mandated 8-byte alignment if unflattening it out of
	 * a FIT image property, so play it safe here.
	 */
	const struct fdt_reserve_entry_unaligned {
		fdt64_t address;
		fdt64_t size;
	} __packed *r;
	int n = 0;

	r = (void *)fdt + be32_to_cpu(fdt->off_mem_rsvmap);

	while (dt_ptr_ok(fdt, r) && n < OF_MAX_RESERVE_MAP) {
		if (!r->size)
			return n;
		n++;
		r++;
	}

	return n == OF_MAX_RESERVE_MAP ? -EINVAL : -ESPIPE;
}

/**
 * of_unflatten_reservemap - store /memreserve/ entries in unflattened tree
 * @root - The unflattened tree
 * @fdt - the flattened device tree blob
 *
 * This stores the memreserve entries from the dtb in a newly created
 * /memserve node in the unflattened device tree. The device tree
 * flatten code moves the entries back to the /memreserve/ area in the
 * flattened tree.
 *
 * Return: 0 for success or negative error code
 */
static int of_unflatten_reservemap(struct device_node *root,
				   const struct fdt_header *fdt)
{
	int n;
	struct property *p;
	struct device_node *memreserve;

	n = of_reservemap_num_entries(fdt);
	if (n <= 0)
		return n;

	memreserve = of_new_node(root, "memreserve");
	if (!memreserve)
		return -ENOMEM;

	p = of_new_property(memreserve, "reg",
			    (void *)fdt + be32_to_cpu(fdt->off_mem_rsvmap),
			    n * sizeof(struct fdt_reserve_entry));
	if (!p)
		return -ENOMEM;

	return 0;
}

static int fdt_parse_header(const struct fdt_header *fdt, size_t fdt_size,
			  struct fdt_header *out)
{
	if (fdt_size < sizeof(struct fdt_header))
		return -EINVAL;

	if (fdt->magic != cpu_to_fdt32(FDT_MAGIC)) {
		pr_err("bad magic: 0x%08x\n", fdt32_to_cpu(fdt->magic));
		return -EINVAL;
	}

	if (fdt->version != cpu_to_fdt32(17)) {
		pr_err("bad dt version: 0x%08x\n", fdt32_to_cpu(fdt->version));
		return -EINVAL;
	}

	out->totalsize = fdt32_to_cpu(fdt->totalsize);
	out->off_dt_struct = fdt32_to_cpu(fdt->off_dt_struct);
	out->size_dt_struct = fdt32_to_cpu(fdt->size_dt_struct);
	out->off_dt_strings = fdt32_to_cpu(fdt->off_dt_strings);
	out->size_dt_strings = fdt32_to_cpu(fdt->size_dt_strings);

	if (out->totalsize > fdt_size)
		return -EINVAL;

	if (size_add(out->off_dt_struct, out->size_dt_struct) > out->totalsize) {
		pr_err("unflatten: dt size exceeds total size\n");
		return -ESPIPE;
	}

	if (size_add(out->off_dt_strings, out->size_dt_strings) > out->totalsize) {
		pr_err("unflatten: string size exceeds total size\n");
		return -ESPIPE;
	}

	return 0;
}

/**
 * of_unflatten_dtb - unflatten a dtb binary blob
 * @infdt - the fdt blob to unflatten
 *
 * Parse a flat device tree binary blob and return a pointer to the
 * unflattened tree.
 */
static struct device_node *__of_unflatten_dtb(const void *infdt, int size,
					      bool constprops)
{
	const void *nodep;	/* property node pointer */
	uint32_t tag;		/* tag */
	int  len;		/* length of the property */
	const struct fdt_property *fdt_prop;
	const char *pathp, *name;
	struct device_node *root, *node = NULL;
	struct property *p;
	uint32_t dt_struct;
	const struct fdt_node_header *fnh;
	void *dt_strings;
	struct fdt_header f;
	int ret;
	unsigned int maxlen;
	const struct fdt_header *fdt = infdt;

	ret = fdt_parse_header(infdt, size, &f);
	if (ret < 0)
		return ERR_PTR(ret);

	dt_struct = f.off_dt_struct;
	dt_strings = (void *)fdt + f.off_dt_strings;

	root = of_new_node(NULL, NULL);
	if (!root)
		return ERR_PTR(-ENOMEM);

	ret = of_unflatten_reservemap(root, fdt);
	if (ret)
		goto err;

	while (1) {
		__be32 *tagp = (uint32_t *)(infdt + dt_struct);
		if (!dt_ptr_ok(infdt, tagp)) {
			ret = -ESPIPE;
			goto err;
		}

		tag = be32_to_cpu(*tagp);

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

			if (!node) {
				/* The root node must have an empty name */
				if (*pathp) {
					ret = -EINVAL;
					goto err;
				}
				node = root;
			} else {
				/* Only the root node may have an empty name */
				if (!*pathp) {
					ret = -EINVAL;
					goto err;
				}
				node = of_new_node(node, pathp);
			}

			dt_struct = dt_struct_advance(&f, dt_struct,
					sizeof(struct fdt_node_header) + len + 1);

			break;

		case FDT_END_NODE:
			if (!node) {
				pr_err("unflatten: too many end nodes\n");
				ret = -EINVAL;
				goto err;
			}

			node = node->parent;

			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);

			break;

		case FDT_PROP:
			fdt_prop = infdt + dt_struct;
			len = fdt32_to_cpu(fdt_prop->len);
			nodep = fdt_prop->data;

			name = dt_string(&f, dt_strings, fdt32_to_cpu(fdt_prop->nameoff));
			if (!name || !node) {
				ret = -ESPIPE;
				goto err;
			}

			if (constprops)
				p = of_new_property_const(node, name, nodep, len);
			else
				p = of_new_property(node, name, nodep, len);

			if (!strcmp(name, "phandle") && len == 4)
				node->phandle = be32_to_cpup(of_property_get_value(p));

			dt_struct = dt_struct_advance(&f, dt_struct,
					sizeof(struct fdt_property) + len);

			break;

		case FDT_NOP:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);

			break;

		case FDT_END:
			return root;

		default:
			pr_err("unflatten: Unknown tag 0x%08X\n", tag);
			ret = -EINVAL;
			goto err;
		}

		if (!dt_struct) {
			ret = -ESPIPE;
			goto err;
		}
	}
err:
	of_delete_node(root);

	return ERR_PTR(ret);
}

/**
 * of_unflatten_dtb - unflatten a dtb binary blob
 * @infdt - the fdt blob to unflatten
 *
 * Parse a flat device tree binary blob and return a pointer to the unflattened
 * tree. The tree must be freed after use with of_delete_node().
 */
struct device_node *of_unflatten_dtb(const void *infdt, int size)
{
	return __of_unflatten_dtb(infdt, size, false);
}

/**
 * of_unflatten_dtb_const - unflatten a dtb binary blob
 * @infdt - the fdt blob to unflatten
 *
 * Parse a flat device tree binary blob and return a pointer to the unflattened
 * tree. The tree must be freed after use with of_delete_node(). Unlike the
 * above version this function uses the property data directly from the input
 * flattened tree instead of copying the data, thus @infdt must be valid for the
 * whole lifetime of the returned tree. This is normally not what you want, so
 * use of_unflatten_dtb() instead.
 */
struct device_node *of_unflatten_dtb_const(const void *infdt, int size)
{
	return __of_unflatten_dtb(infdt, size, true);
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

	do {
		*dest++ = *src;
		len++;
		if (!maxlen)
			return -ENOSPC;
		maxlen--;
	} while (*src++);

	return len - 1;
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
	size_t new_size;
	void *previous;

	/*
	 * We assume strings and names have a maximum length of 1024
	 * whereas properties can be longer. We allocate new memory
	 * if we have less than 1024 bytes (+ the property size left.
	 */
	if (fdt->str_size - fdt->str_nextofs < 1024) {
		previous = fdt->strings;
		new_size = fdt->str_size * 2;

		fdt->strings = realloc(previous, new_size);
		if (!fdt->strings) {
			free(previous);
			return -ENOMEM;
		}

		fdt->str_size = new_size;
	}

	if (fdt->dt_size - fdt->dt_nextofs < 1024 + dtsize) {
		previous = fdt->dt;
		new_size = fdt->dt_size * 2;

		if (new_size <= dtsize)
			new_size = roundup_pow_of_two(fdt->dt_size + dtsize);

		fdt->dt = memalign_realloc(previous, fdt->dt_size, new_size);
		if (!fdt->dt) {
			free(previous);
			return -ENOMEM;
		}

		fdt->dt_size = new_size;
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

static int __of_flatten_dtb(struct fdt *fdt, struct device_node *node, int is_root)
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
		if (is_root && !strcmp(n->name, "memreserve"))
			continue;

		ret = __of_flatten_dtb(fdt, n, 0);
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
	uint32_t ofs, off_mem_rsvmap;
	struct fdt_node_header *nh;
	struct device_node *memreserve;
	int len;

	header.magic = cpu_to_fdt32(FDT_MAGIC);
	header.version = cpu_to_fdt32(0x11);
	header.last_comp_version = cpu_to_fdt32(0x10);

	fdt.dt = xmemalign(SZ_64K, SZ_64K);
	fdt.dt_size = SZ_64K;

	fdt.strings = xzalloc(SZ_64K);
	fdt.str_size = SZ_64K;

	memset(fdt.dt, 0, SZ_64K);

	ofs = sizeof(struct fdt_header);

	off_mem_rsvmap = ofs;
	header.off_mem_rsvmap = cpu_to_fdt32(off_mem_rsvmap);
	ofs += sizeof(struct fdt_reserve_entry) * OF_MAX_RESERVE_MAP;

	fdt.dt_nextofs = ofs;

	ret = __of_flatten_dtb(&fdt, node, 1);
	if (ret)
		goto out_free;

	memreserve = of_find_node_by_name_address(node, "memreserve");
	if (memreserve) {
		const void *entries = of_get_property(memreserve, "reg", &len);

		if (entries)
			memcpy(fdt.dt + off_mem_rsvmap, entries, len);
	}

	nh = fdt.dt + fdt.dt_nextofs;
	nh->tag = cpu_to_fdt32(FDT_END);
	fdt.dt_nextofs = dt_next_ofs(fdt.dt_nextofs, sizeof(struct fdt_node_header));

	header.off_dt_struct = cpu_to_fdt32(ofs);
	header.size_dt_struct = cpu_to_fdt32(fdt.dt_nextofs - ofs);

	header.off_dt_strings = cpu_to_fdt32(fdt.dt_nextofs);
	header.size_dt_strings = cpu_to_fdt32(fdt.str_nextofs);

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
 * This adds the reservemap entries previously collected in
 * of_add_reserve_entry() to a devicetree binary blob.
 */
void fdt_add_reserve_map(void *__fdt)
{
	struct fdt_header *fdt = __fdt;
	struct of_reserve_map *res = &of_reserve_map;
	struct fdt_reserve_entry *fdt_res =
		__fdt + be32_to_cpu(fdt->off_mem_rsvmap);
	int i, n;

	n = of_reservemap_num_entries(fdt);
	if (n < 0)
		return;

	if (n + res->num_entries + 2 > OF_MAX_FREE_RESERVE_MAP) {
		pr_err("Too many entries in reserve map\n");
		return;
	}

	fdt_res += n;

	for (i = 0; i < res->num_entries; i++) {
		of_write_number(&fdt_res->address, res->start[i], 2);
		of_write_number(&fdt_res->size, res->end[i] - res->start[i] + 1,
				2);
		fdt_res++;
	}

	of_write_number(&fdt_res->address, 0, 2);
	of_write_number(&fdt_res->size, 0, 2);
}

void fdt_print_reserve_map(const void *__fdt)
{
	const struct fdt_header *fdt = __fdt;
	const struct fdt_reserve_entry *fdt_res =
		__fdt + be32_to_cpu(fdt->off_mem_rsvmap);
	int n = 0;

	while (1) {
		uint64_t size = fdt64_to_cpu(fdt_res->size);
		uint64_t address = fdt64_to_cpu(fdt_res->address);

		if (!size)
			break;

		printf("/memreserve/ #%d: 0x%08llx - 0x%08llx\n", n, address, address + size - 1);

		n++;
		fdt_res++;
		if (n == OF_MAX_RESERVE_MAP)
			return;
	}
}

static int fdt_string_is_compatible(const char *haystack, int haystack_len,
				    const char *needle, int needle_len)
{
	const char *p;
	int index = 0;

	while (haystack_len >= needle_len) {
		if (memcmp(needle, haystack, needle_len + 1) == 0)
			return OF_DEVICE_COMPATIBLE_MAX_SCORE - (index << 2);

		p = memchr(haystack, '\0', haystack_len);
		if (!p)
			return 0;
		haystack_len -= (p - haystack) + 1;
		haystack = p + 1;
		index++;
	}

	return 0;
}

int fdt_machine_is_compatible(const struct fdt_header *fdt, size_t fdt_size, const char *compat)
{
	uint32_t tag;
	const struct fdt_property *fdt_prop;
	const char *name;
	uint32_t dt_struct;
	const struct fdt_node_header *fnh;
	const void *dt_strings;
	struct fdt_header f;
	int ret, len;
	int expect = FDT_BEGIN_NODE;
	int compat_len = strlen(compat);

	ret = fdt_parse_header(fdt, fdt_size, &f);
	if (ret < 0)
		return 0;

	dt_struct = f.off_dt_struct;
	dt_strings = (const void *)fdt + f.off_dt_strings;

	while (1) {
		const __be32 *tagp = (const void *)fdt + dt_struct;
		if (!dt_ptr_ok(fdt, tagp))
			return 0;

		tag = be32_to_cpu(*tagp);
		if (tag != FDT_NOP && tag != expect)
			return 0;

		switch (tag) {
		case FDT_BEGIN_NODE:
			fnh = (const void *)fdt + dt_struct;

			/* The root node must have an empty name */
			if (fnh->name[0] != '\0')
				return 0;

			dt_struct = dt_struct_advance(&f, dt_struct,
					sizeof(struct fdt_node_header) + 1);

			/*
			 * Quoting Device Tree Specification v0.4 §5.4.2:
			 *
			 *   [T]his process requires that all property definitions for
			 *   a particular node precede any subnode definitions for that
			 *   node. Although the structure would not be ambiguous if
			 *   properties and subnodes were intermingled, the code needed
			 *   to process a flat tree is simplified by this requirement.
			 *
			 * So let's make use of this simplification.
			 */
			expect = FDT_PROP;
			break;

		case FDT_PROP:
			fdt_prop = (const void *)fdt + dt_struct;
			len = fdt32_to_cpu(fdt_prop->len);

			name = dt_string(&f, dt_strings, fdt32_to_cpu(fdt_prop->nameoff));
			if (!name)
				return 0;

			if (strcmp(name, "compatible")) {
				dt_struct = dt_struct_advance(&f, dt_struct,
							      sizeof(struct fdt_property) + len);
				break;
			}

			return fdt_string_is_compatible(fdt_prop->data, len, compat, compat_len);

		case FDT_NOP:
			dt_struct = dt_struct_advance(&f, dt_struct, FDT_TAGSIZE);
			break;

		default:
			return 0;
		}

		if (!dt_struct)
			return 0;
	}

	return 0;
}

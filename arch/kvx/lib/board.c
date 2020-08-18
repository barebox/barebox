// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2019 Kalray Inc.
 */

#include <common.h>
#include <malloc.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>

static int find_memory_malloc(u32 na, u32 ns, u64 addr, u64 *membase,
			      u64 *memsize, const __be32 *reg)
{
	int i;
	u64 memsize64 = 0, membase64 = 0;

	for (i = 0; i < na; i++)
		membase64 = (membase64 << 32) | fdt32_to_cpu(*reg++);

	for (i = 0; i < ns; i++)
		memsize64 = (memsize64 << 32) | fdt32_to_cpu(*reg++);

	if (addr > membase64 && addr < (membase64 + memsize64)) {
		*membase = membase64;
		*memsize = memsize64;
		return 1;
	}

	return 0;
}

/**
 * of_find_mem - Find the first memory range (from fdt) in which an address is
 * contained
 * @fdt: fdt blob containing memory nodes
 * @addr: Address to search in available memories
 * @membase: Returned memory base address
 * @memsize: Returned memory size
 */
static void of_find_mem(void *fdt, u64 addr, u64 *membase, u64 *memsize)
{
	const __be32 *nap, *nsp, *reg;
	u32 na, ns, reg_size;
	int node, size, i, ret;

	/* Make sure FDT blob is sane */
	if (fdt_check_header(fdt) != 0) {
		pr_err("Invalid device tree blob\n");
		goto err;
	}

	/* Find the #address-cells and #size-cells properties */
	node = fdt_path_offset(fdt, "/");
	if (node < 0) {
		pr_err("Cannot find root node\n");
		goto err;
	}

	nap = fdt_getprop(fdt, node, "#address-cells", &size);
	if (!nap || (size != 4)) {
		pr_err("Cannot find #address-cells property");
		goto err;
	}
	na = fdt32_to_cpu(*nap);

	nsp = fdt_getprop(fdt, node, "#size-cells", &size);
	if (!nsp || (size != 4)) {
		pr_err("Cannot find #size-cells property");
		goto err;
	}
	ns = fdt32_to_cpu(*nap);

	node = -1;
	/* Iterate on the memory devices */
	while (1) {
		/* Find the memory node */
		node = fdt_node_offset_by_prop_value(fdt, node, "device_type",
						     "memory",
						     sizeof("memory"));
		if (node < 0) {
			pr_err("Cannot find memory node\n");
			goto err;
		}

		reg_size = na + ns * sizeof(u32);
		reg = fdt_getprop(fdt, node, "reg", &size);
		if (size < reg_size) {
			pr_err("cannot get memory range\n");
			goto err;
		}

		/* Iterate on reg content */
		for (i = 0; i < size; i += reg_size) {
			ret = find_memory_malloc(na, ns, addr, membase, memsize,
						 reg);
			if (ret)
				return;
			reg += na + ns;
		}
	}
err:
	pr_err("No memory, cannot continue\n");
	while (1);
}

/**
 * exclude_dtb_from_alloc - Find best zone to allocate without overwriting dtb
 *
 * @fdt: fdt blob
 * @alloc_start: start of allocation zone
 * @alloc_end: end of allocation zone
 */
static void exclude_dtb_from_alloc(void *fdt, u64 *alloc_start, u64 *alloc_end)
{
	const struct fdt_header *fdth = fdt;
	u64 fdt_start = (u64) fdt;
	u64 fdt_end = fdt_start + be32_to_cpu(fdth->totalsize);
	u64 start_size = 0, end_size = 0;

	/*
	 * If the device tree is inside the malloc zone, we must exclude it to
	 * avoid allocating memory over it while unflattening it
	 */
	if (fdt_end < *alloc_start || fdt_start > (*alloc_end))
		return;

	/* Compute the largest remaining chunk when removing the dtb */
	if (fdt_start >= *alloc_start)
		start_size = (fdt_start - *alloc_start);

	if (fdt_end <= *alloc_end)
		end_size = *alloc_end - fdt_end;

	/* Modify start/end to reflect the maximum area we found */
	if (start_size >= end_size)
		*alloc_end = fdt_start;
	else
		*alloc_start = fdt_end;
}

void __noreturn kvx_start_barebox(void)
{
	u64 memsize = 0, membase = 0;
	u64 barebox_text_end = (u64) &__end;
	u64 alloc_start, alloc_end;

	of_find_mem(boot_dtb, barebox_text_end, &membase, &memsize);

	alloc_start = barebox_text_end;
	alloc_end = (membase + memsize);
	exclude_dtb_from_alloc(boot_dtb, &alloc_start, &alloc_end);

	mem_malloc_init((void *) alloc_start, (void *) alloc_end);

	start_barebox();

	hang();
}
